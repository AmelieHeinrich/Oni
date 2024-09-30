// Copyright (c) 2015 ~ 2019 NVIDIA Corporation 
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

/// \file 
/// Header of the low-level interface of NVTT.
///
/// This contains functions for compressing to each of the formats NVTT
/// supports, as well as different ways of specifying the input and output.
/// For instance, the low-level API allows both compression input and output
/// to exist on the GPU, removing the need for any CPU-to-GPU or GPU-to-CPU
/// copies.
/// 
/// Here are some general notes on the low-level compression functions.
/// 
/// These functions often support "fast-mode" and "slow-mode" compression.
/// These switch between two compression algorithms: fast-mode algorithms
/// are faster but lower-quality, while slow-mode algorithms are slower
/// but higher-quality. Other functions support multiple quality levels.
/// 
/// Sometimes, the fast-mode or slow-mode algorithm isn't available on the GPU.
/// In that case, that compression will be done on the CPU instead. In this
/// case, slow-mode compression on the GPU may be faster than fast-mode
/// compression on the CPU.
/// 
/// To use GPU compression, there must be a GPU that supports CUDA.
/// See nvtt::isCudaSupported().

#ifndef nvtt_lowlevel_h
#define nvtt_lowlevel_h

#include <stddef.h>
#include <stdint.h>

#ifdef _DOXYGEN_
/// @brief Functions with this macro are accessible via the NVTT DLL.
#define NVTT_API
/// @brief These functions are accessible via the NVTT DLL, but there are
/// better replacements for them, and they may be removed in a future major
/// version.
#define NVTT_DEPRECATED_API
#endif

// Function linkage
#if NVTT_SHARED

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__ || defined __MINGW32__
#  ifdef NVTT_EXPORTS
#    define NVTT_API __declspec(dllexport)
#  else
#    define NVTT_API __declspec(dllimport)
#  endif
#endif

#if defined __GNUC__ >= 4
#  ifdef NVTT_EXPORTS
#    define NVTT_API __attribute__((visibility("default")))
#  endif
#endif

#endif // NVTT_SHARED

#if !defined NVTT_API
#  define NVTT_API
#endif

#if !defined NVTT_DEPRECATED_API
#  define NVTT_DEPRECATED_API [[deprecated]] NVTT_API
#endif

namespace nvtt
{
	/// Check if CUDA is supported by the run-time environment
	NVTT_API bool isCudaSupported();

	/// @brief Tells NVTT to always use an application-set device rather
	/// than selecting its own.
	/// 
	/// By default, NVTT functions such as nvtt::isCudaSupported() and
	/// nvtt::Context() can choose a device and call cudaSetDevice().
	/// Calling this function will prevent NVTT from calling cudaSetDevice(),
	/// and will make it use the currently set device instead.
	/// The application must then call cudaSetDevice() before calling NVTT
	/// functions, and whenever it wants to change the device subsequent
	/// NVTT functions will use.
	/// 
	/// For instance, this may be useful when managing devices on multi-GPU
	/// systems.
	NVTT_API void useCurrentDevice();

	struct TimingContext;

	/// Value type of the input images.
	/// The input buffer will use the same value type as the input images
	enum ValueType
	{
		UINT8, ///< 8-bit unsigned integer.
		SINT8, ///< 8-bit signed integer. Can be casted to float by dividing by 127, with the exception that both -128 and -127 represent -1.0f.
		FLOAT32, ///< IEEE 754 single-precision floating-point: 1 sign bit, 8 exponent bits, and 23 mantissa bits.
		FLOAT16  ///< IEEE 754 half-precision floating-point: 1 sign bit, 5 exponent bits, and 10 mantissa bits.
	};

	/// Name of channels for defining a swizzling
	enum ChannelOrder
	{
		Red = 0,
		Green = 1,
		Blue = 2,
		Alpha = 3,
		Zero = 4,
		One = 5
	};

	/// Use this structure to reference each of the input images
	struct RefImage
	{
		const void *data; ///< For CPUInputBuffer, this should point to host memory; for GPUInputBuffer, this should point to device memory.
		int width = 0; ///< Width of the image. This can be arbitrary, up to 65535.
		int height = 0; ///< Height of the image. This can be arbitrary, up to 65535.
		int depth = 1; ///< Z-dimension of the images, usually 1
		int num_channels = 4; ///< Number of channels the image has
		ChannelOrder channel_swizzle[4] = { Red,Green,Blue,Alpha }; ///< Channels order how the image is stored
		bool channel_interleave = true; ///< Whether the rgba channels are interleaved (r0, g0, b0, a0, r1, g1, b1, a1, ...)
	};

	/// @brief Structure containing all the input images from host memory.
	/// The image data is reordered by tiles.
	struct CPUInputBuffer
	{
		/// @brief Construct a CPUInputBuffer from 1 or more RefImage structs.
		/// 
		/// The input images should use the same value type.
		/// `images[i].data` should point to host memory here.
		/// `num_tiles` can be an array of numImages elements used to return the number of tiles of each input image after reordering.
		NVTT_API CPUInputBuffer(const RefImage* images, ValueType value_type, int numImages=1, int tile_w=4, int tile_h=4,
			float WeightR=1.0f, float WeightG=1.0f, float WeightB=1.0f, float WeightA=1.0f, nvtt::TimingContext *tc = nullptr, unsigned* num_tiles = nullptr);
		
		/// Destructor
		NVTT_API ~CPUInputBuffer();

		/// The total number of tiles of the input buffer
		NVTT_API int NumTiles() const;

		/// Tile Size
		NVTT_API void TileSize(int& tile_w, int& tile_h) const;

		/// Value type. The same as the input images used for creating this object
		NVTT_API ValueType Type() const;

		/// Get a pointer to the i-th tile. Mainly used internally.
		NVTT_API void* GetTile(int i, int& vw, int& vh) const;

		struct Private;
		Private *m;
	};

	/// @brief Structure containing all the input images from device memory.
	/// The image data is reordered by tiles.
	struct GPUInputBuffer
	{
		/// @brief Construct a GPUInputBuffer from 1 or more RefImage structs.
		/// 
		/// The input images should use the same value type.
		/// `images[i].data` should point to GPU global memory here (more specifically, a CUDA device pointer).
		/// `num_tiles` can be used to return the number of tiles of each input image after reordering.
		NVTT_API GPUInputBuffer(const RefImage* images, ValueType value_type, int numImages = 1, int tile_w = 4, int tile_h = 4,
			float WeightR = 1.0f, float WeightG = 1.0f, float WeightB = 1.0f, float WeightA = 1.0f, nvtt::TimingContext *tc = nullptr, unsigned* num_tiles = nullptr);

		/// @brief Construct a GPUInputBuffer from a CPUInputBuffer.
		/// 
		/// `begin`/`end` can be used to sepcify a range of tiles to copy from the CPUInputBuffer.
		/// `end = -1` means the end of the input buffer.
		NVTT_API GPUInputBuffer(const CPUInputBuffer& cpu_buf, int begin = 0, int end = -1, nvtt::TimingContext *tc = nullptr);
		
		/// Destructor
		NVTT_API ~GPUInputBuffer();

		/// The total number of tiles of the input buffer
		NVTT_API int NumTiles() const;

		/// Tile Size
		NVTT_API void TileSize(int& tile_w, int& tile_h) const;

		/// @brief Value type. The same as the input images of the CPUInputBuffer used for creating this object
		NVTT_API ValueType Type() const;

		struct Private;
		Private *m;
	};

	/// Supported block-compression formats, including compressor variants.
	// @@ I wish I had distinguished between "formats" and compressors.
	/// That is:
	/// - 'DXT1' is a format, 'DXT1a' and 'DXT1n' are DXT1 compressors.
	/// - 'DXT3' is a format, 'DXT3n' is a DXT3 compressor.
	///
	// @@ Having multiple enums for the same ids only creates confusion. Clean this up.
	enum Format
	{
		// No block-compression (linear).
		Format_RGB,  ///< Linear RGB format
		Format_RGBA = Format_RGB, ///< Linear RGBA format

		// DX9 formats.
		Format_DXT1,    ///< DX9 - DXT1 format
		Format_DXT1a,   ///< DX9 - DXT1 with binary alpha.
		Format_DXT3,    ///< DX9 - DXT3 format
		Format_DXT5,    ///< DX9 - DXT5 format
		Format_DXT5n,   ///< DX9 - DXT5 normal format. Stores a normal (x, y, z) as (R, G, B, A) = (1, y, 0, x).

		// DX10 formats.
		Format_BC1 = Format_DXT1, ///< DX10 - BC1 (DXT1) format
		Format_BC1a = Format_DXT1a, ///< DX10 - BC1 (DXT1) format
		Format_BC2 = Format_DXT3, ///< DX10 - BC2 (DXT3) format
		Format_BC3 = Format_DXT5, ///< DX10 - BC3 (DXT5) format
		Format_BC3n = Format_DXT5n, ///< DX10 - BC3 (DXT5) normal format for improved compression, storing a normal (x, y, z) as (1, y, 0, x).
		Format_BC4,     ///< DX10 - BC4U (ATI1) format (one channel, unsigned)
		Format_BC4S,     ///< DX10 - BC4S format (one channel, signed)
		Format_ATI2,     ///< DX10 - ATI2 format, similar to BC5U, channel order GR instead of RG
		Format_BC5,     ///< DX10 - BC5U format (two channels, unsigned)
		Format_BC5S,     ///< DX10 - BC5S format (two channels, signed)

		Format_DXT1n,   ///< Not supported.
		Format_CTX1,    ///< Not supported.

		Format_BC6U,     ///< DX10 - BC6 format (three-channel HDR, unsigned)
		Format_BC6S,     ///< DX10 - BC6 format (three-channel HDR, signed)

		Format_BC7,     ///< DX10 - BC7 format (four channels, UNORM)

		//Format_BC5_Luma,    // Two DXT alpha blocks encoding a single float.
		/// DX10 - BC3(DXT5) - using a magnitude encoding to approximate
		/// three-channel HDR data in four UNORM channels. The input should be
		/// in the range [0,1], and this should give more accurate values
		/// closer to 0. On most devices, consider using BC6 instead.
		/// 
		/// To decompress this format, decompress it like a standard BC3 texture,
		/// then compute `(R, G, B)` from `(r, g, b, m)` using `fromRGBM()` with
		/// `range = 1` and `threshold = 0.25`:
		/// 
		/// `M = m * 0.75 + 0.25`;
		/// 
		/// `(R, G, B) = (r, g, b) * M`
		/// 
		/// The idea is that since BC3 uses separate compression for the RGB
		/// and alpha blocks, the RGB and M signals can be independent.
		/// Additionally, the compressor can account for the RGB compression
		/// error.
		/// This will print warnings if any of the computed m values were
		/// greater than 1.0.
		Format_BC3_RGBM,

		// 14 ASTC LDR Formats 
		// Added by Fei Yang
		Format_ASTC_LDR_4x4, ///< ASTC - LDR - format, tile size 4x4
		Format_ASTC_LDR_5x4, ///< ASTC - LDR - format, tile size 5x4
		Format_ASTC_LDR_5x5, ///< ASTC - LDR - format, tile size 5x5
		Format_ASTC_LDR_6x5, ///< ASTC - LDR - format, tile size 6x5
		Format_ASTC_LDR_6x6, ///< ASTC - LDR - format, tile size 6x6
		Format_ASTC_LDR_8x5, ///< ASTC - LDR - format, tile size 8x5
		Format_ASTC_LDR_8x6, ///< ASTC - LDR - format, tile size 8x6
		Format_ASTC_LDR_8x8, ///< ASTC - LDR - format, tile size 8x8
		Format_ASTC_LDR_10x5, ///< ASTC - LDR - format, tile size 10x5
		Format_ASTC_LDR_10x6, ///< ASTC - LDR - format, tile size 10x6
		Format_ASTC_LDR_10x8, ///< ASTC - LDR - format, tile size 10x8
		Format_ASTC_LDR_10x10, ///< ASTC - LDR - format, tile size 10x10
		Format_ASTC_LDR_12x10, ///< ASTC - LDR - format, tile size 12x10
		Format_ASTC_LDR_12x12, ///< ASTC - LDR - format, tile size 12x12

		Format_Count,

		/// Placeholder in structs to produce errors if a format is not
		/// explicitly set, since format 0 is Format_RGB.
		Format_Unset = 255
	};

	/// @brief Pixel value types.
	/// 
	/// These are used for Format_RGB: they indicate how the output should be
	/// interpreted, but do not have any influence over the input. They are
	/// ignored for other compression modes.
	enum PixelType
	{
		PixelType_UnsignedNorm = 0, ///< Used to indicate a DXGI_..._UNORM format.
		PixelType_SignedNorm = 1,   ///< Not supported yet.
		PixelType_UnsignedInt = 2,  ///< Not supported yet.
		PixelType_SignedInt = 3,    ///< Not supported yet.
		PixelType_Float = 4, ///< Used to indicate a DXGI_..._FLOAT format.
		PixelType_UnsignedFloat = 5, ///< Used to indicate a DXGI_..._UF16 format. Unused.
		PixelType_SharedExp = 6,    ///< Shared exponent. Only supported for DXGI_FORMAT_R9G9B9E5_SHAREDEXP.
	};

	/// @brief Quality modes.
	///
	/// These can be used to trade off speed of compression for lower error,
	/// and often selects the specific compression algorithm that will be used.
	/// Here's a table showing which (format, quality) combinations support
	/// CUDA acceleration:
	/// 
	/// Quality    | BC1 | BC1a | BC2 | BC3 | BC3n | RGBM | BC4 | BC5 | BC6 | BC7       | ASTC |
	/// -----------|-----|------|-----|-----|------|------|-----|-----|-----|-----------|------|
	/// Fastest    | Yes | No   | No  | No  | No   | No   | Yes | Yes | Yes | Yes       | Yes  |
	/// Normal     | Yes | Yes  | Yes | Yes | Yes  | No   | Yes | Yes | Yes | Yes       | Yes  |
	/// Production | Yes | Yes  | Yes | Yes | Yes  | No   | No  | No  | No  | No (slow) | Yes  |
	/// Highest    | Yes | Yes  | Yes | Yes | Yes  | No   | No  | No  | No  | No (slow) | Yes  |
	/// 
	/// See the documentation of the different compression functions in
	/// nvtt_lowlevel.h for more information.
	enum Quality
	{
		Quality_Fastest,
		Quality_Normal,
		Quality_Production,
		Quality_Highest,
	};

	/// Declares that EncodeSettings uses the structure layout used in
	/// NVTT 3.2.
	static const uint32_t EncodeSettings_Version_1 = 1;

	/// Encode flags for EncodeSettings.
	enum EncodeFlags : uint32_t
	{
		EncodeFlags_None = 0,
		/// Compress on the GPU if CUDA is available, instead of the CPU.
		EncodeFlags_UseGPU = 1 << 0,
		/// The `output` pointer is a CUDA device pointer (e.g. returned by
		/// `cudaMalloc()`), instead of a pointer to system memory (e.g.
		/// returned by `malloc()`).
		EncodeFlags_OutputToGPUMem = 1 << 1,
		/// Specifies that all pixels have an alpha component equal to 1.0f.
		/// If set, this makes compression slightly faster. It's valid to
		/// leave it unset even if the image is opaque.
		EncodeFlags_Opaque = 1 << 2
	};

	/// Low-level settings including the compression format, where compressed
	/// data should be written, and how to encode the data.
	/// This can be used in the low-level compression API (@see nvtt_encode)
	/// for any format NVTT supports.
	///
	/// Most new apps will only need to use `format` and `SetOutputToGPUMem()`.
	/// EncodeSettings' functions provide concise ways to change individual
	/// properties. For instance, here's one way to configure it to encode
	/// data to ASTC 4x4 at Production quality, attach a timing context, and
	/// output data on the GPU:
	///
	/// ```cpp
	/// using namespace nvtt;
	/// TimingContext tc = TimingContext();
	/// const auto settings = EncodeSettings().SetFormat(Format_ASTC_LDR_4x4)
	/// 	.SetQuality(Quality_Production)
	/// 	.SetTimingContext(&tc)
	/// 	.SetOutputToGPUMem(true)
	/// ```
	///
	/// If your compiler supports C++20 aggregate initialization, it can also
	/// be used (and the C wrapper supports C99's designated initialization):
	/// ```cpp
	/// const EncodeSettings settings{
	///		.format = Format_ASTC_LDR_4x4,
	///		.quality = Quality_Production,
	///		.timing_context = &tc,
	///		.encode_flags = EncodeFlags_UseGPU | EncodeFlags_OutputToGPUMem };
	/// ```
	///
	/// @since NVTT 3.2
	struct EncodeSettings
	{
		/// The structure type and version. This is here to allow this struct
		/// to be identified from its first 32 bits, and to allow future NVTT
		/// versions to add new members to this struct without breaking
		/// backwards compatibility. It should not be changed from the default.
		uint32_t sType = EncodeSettings_Version_1;
		/// The desired compression format. @see Format
		Format format = Format_Unset;
		/// The desired compression quality. @see Quality
		Quality quality = Quality_Normal;
		/// When `format` is Format_RGB, this defines the type of the
		/// output data.
		PixelType rgb_pixel_type = (PixelType)0;
		/// @see nvtt::TimingContext.
		TimingContext *timing_context = nullptr;
		/// A bitmask of EncodeFlags. This can be set using bit operations,
		/// like `encode_flags = EncodeFlags_UseGPU | EncodeFlags_Opaque`,
		/// or using EncodeSettings' functions for individual flags.
		uint32_t encode_flags = EncodeFlags_UseGPU;

		/// Functions that set this object's fields and return this object.
		/// These allows setters to be chained.
		NVTT_API EncodeSettings& SetFormat(Format _format);
		NVTT_API EncodeSettings& SetQuality(Quality _quality);
		NVTT_API EncodeSettings& SetRGBPixelType(PixelType _rgb_pixel_type);
		NVTT_API EncodeSettings& SetTimingContext(TimingContext *_timing_context);
		/// @see EncodeFlags_UseGPU
		NVTT_API EncodeSettings& SetUseGPU(bool _use_gpu);
		/// @see EncodeFlags_OutputToGPUMem
		NVTT_API EncodeSettings& SetOutputToGPUMem(bool _to_device_mem);
		/// @see EncodeFlags_Opaque
		NVTT_API EncodeSettings& SetIsOpaque(bool _is_opaque);
	};

	/// Low-level interface for compressing to any of NVTT's formats from a
	/// CPUInputBuffer. If encoding succeeded, returns true. On failure, logs
	/// at least one error (@see nvtt::setMessageCallback()) and returns false.
	///
	/// Here's an example of how to encode data in CPU memory to BC7 format
	/// on the GPU, writing the output to CUDA device memory:
	///
	/// ```cpp
	/// // Given void* d_outputData, a pointer to where the compressed data
	/// // should be written on the GPU, and a CPUInputBuffer cpuInput:
	/// using namespace nvtt;
	/// const auto settings = EncodeSettings().SetFormat(Format_BC7)
	///                                       .SetOutputToGPUMem(true);
	/// if (nvtt_encode(cpuInput, d_outputData, settings))
	/// {
	///   // Encoding succeeded! Do something with the data here.
	/// }
	/// ```
	///
	/// @param input Constant reference to input in CPU memory.
	///
	/// @param output Pointer to output: CUDA device pointer if
	/// `settings.encode_flags` includes ENCODE_USE_GPU, and a pointer to CPU
	/// memory otherwise.
	///
	/// @param settings Encoder settings, including the compression format
	/// and quality.
	///
	/// @since NVTT 3.2
	NVTT_API bool nvtt_encode(const CPUInputBuffer& input, void* output, const EncodeSettings& settings);

	/// Low-level interface for compressing to any of NVTT's formats from a
	/// GPUInputBuffer. If encoding succeeded, returns true. On failure, logs
	/// at least one error (@see nvtt::setMessageCallback()) and returns false.
	/// 
	/// Here's an example of how to encode data in GPU memory to BC7 format
	/// on the GPU, writing the output to CUDA device memory:
	/// 
	/// ```cpp
	/// // Given void* d_outputData, a pointer to where the compressed data
	/// // should be written on the GPU, and a GPUInputBuffer gpuInput:
	/// using namespace nvtt;
	/// const auto settings = EncodeSettings().SetFormat(Format_BC7)
	///                                       .SetOutputToGPUMem(true);
	/// if (nvtt_encode(gpuInput, d_outputData, settings))
	/// {
	///   // Encoding succeeded! Do something with the data here.
	/// }
	/// ```
	/// 
	/// @param input Constant reference to input in GPU memory.
	///
	/// @param output Pointer to output: CUDA device pointer if
	/// `settings.encode_flags` includes ENCODE_USE_GPU, and a pointer to CPU
	/// memory otherwise.
	///
	/// @param settings Encoder settings, including the compression format
	/// and quality.
	///
	/// @note Clearing `settings` flag ENCODE_USE_GPU is ignored; data
	/// compressed using this function is currently always compressed on
	/// the GPU.
	/// 
	/// @since NVTT 3.2
	NVTT_API bool nvtt_encode(const GPUInputBuffer& input, void* output, const EncodeSettings& settings);

	/// NVTT 3's initial implementation of the low-level compression API below
	/// used a different function for each combination of the encoding format
	/// category and the input type. Because these functions have different
	/// signatures, we recommend using NVTT 3.2's nvtt_encode API above. The
	/// functions below are provided for backwards compatibility and will
	/// remain supported for all versions of NVTT 3.

	///////////////////////////// BC1 ///////////////////////////////

	/// Interface for compressing to BC1 format from CPUInputBuffer.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param fast_mode If true, uses a faster but lower-quality compressor;
	/// otherwise, uses a slower but higher-quality compressor. This applies
	/// to both CPU and GPU compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc1(const CPUInputBuffer& input, bool fast_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC1 format from GPUInputBuffer, always
	/// using GPU compression.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param fast_mode If true, uses a faster but lower-quality compressor;
	/// otherwise, uses a slower but higher-quality compressor. Compression
	/// always happens on the GPU, so CUDA must be available.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc1(const GPUInputBuffer& input, bool fast_mode, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC1a ///////////////////////////////

	/// Interface for compressing to BC1a format from CPUInputBuffer.
	/// No fast-mode algorithm for the GPU is available, so when `fast_mode`
	/// is true this ignores `useGPU` and compresses on the CPU. In this case,
	/// slow-mode GPU compression may be faster than fast-mode CPU compression.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param fast_mode If true, uses a faster but lower-quality compressor;
	/// otherwise, uses a slower but higher-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for fast-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `fast_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc1a(const CPUInputBuffer& input, bool fast_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC1a format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU slow-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc1a(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC2 ///////////////////////////////
	
	/// Interface for compressing to BC2 format from CPUInputBuffer.
	/// No fast-mode algorithm for the GPU is available, so when `fast_mode`
	/// is true this ignores `useGPU` and compresses on the CPU. In this case,
	/// slow-mode GPU compression may be faster than fast-mode CPU compression.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param fast_mode If true, uses a faster but lower-quality compressor;
	/// otherwise, uses a slower but higher-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for fast-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `fast_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc2(const CPUInputBuffer& input, bool fast_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC2 format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU slow-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc2(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC3 ///////////////////////////////

	/// Interface for compressing to BC3 format from CPUInputBuffer.
	/// No fast-mode algorithm for the GPU is available, so when `fast_mode`
	/// is true this ignores `useGPU` and compresses on the CPU. In this case,
	/// slow-mode GPU compression may be faster than fast-mode CPU compression.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param fast_mode If true, uses a faster but lower-quality compressor;
	/// otherwise, uses a slower but higher-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for fast-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `fast_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc3(const CPUInputBuffer& input, bool fast_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC3 format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU slow-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc3(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC3n ///////////////////////////////

	/// Interface for compressing to BC3n format from CPUInputBuffer.
	/// This method is currently CPU-only, but supports 3 quality levels
	/// - 0, 1, and 2.
	/// See nvtt::Format_BC3n.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param qualityLevel Higher quality levels produce less compression
	/// error, but take longer to compress. Can be 0, 1, or 2.
	/// 
	/// @param output Pointer to output in CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc3n(const CPUInputBuffer& input, int qualityLevel, void* output, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC3 - rgbm ///////////////////////////////

	/// Interface for compressing to BC3 - rgbm format from CPUInputBuffer.
	/// This method is currently CPU-only and has 1 quality level.
	/// See nvtt::Format_BC3_RGBM.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param output Pointer to output in CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc3_rgbm(const CPUInputBuffer& input, void* output, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC4U ///////////////////////////////

	/// Interface for compressing to BC4U format from CPUInputBuffer.
	/// No slow-mode algorithm for the GPU is available, so when `slow_mode`
	/// is true this ignores `useGPU` and compresses on the CPU.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param slow_mode If true, uses a slower but higher-quality compressor;
	/// otherwise, uses a faster but lower-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for slow-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc4(const CPUInputBuffer& input, bool slow_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC4U format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU fast-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc4(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC4S ///////////////////////////////

	/// Interface for compressing to BC4S format from CPUInputBuffer.
	/// No slow-mode algorithm for the GPU is available, so when `slow_mode`
	/// is true this ignores `useGPU` and compresses on the CPU.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param slow_mode If true, uses a slower but higher-quality compressor;
	/// otherwise, uses a faster but lower-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for slow-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc4s(const CPUInputBuffer& input, bool slow_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC4S format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU fast-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc4s(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// ATI2 ///////////////////////////////

	/// Interface for compressing to ATI2 format from CPUInputBuffer.
	/// No slow-mode algorithm for the GPU is available, so when `slow_mode`
	/// is true this ignores `useGPU` and compresses on the CPU.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param slow_mode If true, uses a slower but higher-quality compressor;
	/// otherwise, uses a faster but lower-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for slow-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_ati2(const CPUInputBuffer& input, bool slow_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to ATI2 format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU fast-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_ati2(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC5U ///////////////////////////////

	/// Interface for compressing to BC5U format from CPUInputBuffer.
	/// No slow-mode algorithm for the GPU is available, so when `slow_mode`
	/// is true this ignores `useGPU` and compresses on the CPU.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param slow_mode If true, uses a slower but higher-quality compressor;
	/// otherwise, uses a faster but lower-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for slow-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc5(const CPUInputBuffer& input, bool slow_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC5U format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU fast-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc5(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC5S ///////////////////////////////

	/// Interface for compressing to BC5S format from CPUInputBuffer.
	/// No slow-mode algorithm for the GPU is available, so when `slow_mode`
	/// is true this ignores `useGPU` and compresses on the CPU.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param slow_mode If true, uses a slower but higher-quality compressor;
	/// otherwise, uses a faster but lower-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for slow-mode compression.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc5s(const CPUInputBuffer& input, bool slow_mode, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC5S format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU fast-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc5s(const GPUInputBuffer& input, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC7 ///////////////////////////////

	/// Interface for compressing to BC7 format from CPUInputBuffer.
	/// No slow-mode algorithm for the GPU is available, so when `slow_mode`
	/// is true this ignores `useGPU` and compresses on the CPU. The slow-mode
	/// CPU compressor is particularly slow in this case (as it searches
	/// though a very large space of possibilities), so fast-mode compression
	/// is recommended.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param slow_mode If true, uses a slower but higher-quality compressor;
	/// otherwise, uses a faster but lower-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for slow-mode compression.
	/// 
	/// @param imageHasAlpha Specifies that some pixels in the image have an
	/// alpha value less than 1.0f. If false, this makes compression slightly
	/// faster. It's still valid to set it to true even if the image is opaque.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc7(const CPUInputBuffer& input, bool slow_mode, bool imageHasAlpha, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC7 format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU fast-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param imageHasAlpha Specifies that some pixels in the image have an
	/// alpha value less than 1.0f. If false, this makes compression slightly
	/// faster. It's still valid to set it to true even if the image is opaque.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc7(const GPUInputBuffer& input, bool imageHasAlpha, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// BC6H ///////////////////////////////

	/// Interface for compressing to BC6H format from CPUInputBuffer.
	/// No slow-mode algorithm for the GPU is available, so when `slow_mode`
	/// is true this ignores `useGPU` and compresses on the CPU.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param slow_mode If true, uses a slower but higher-quality compressor;
	/// otherwise, uses a faster but lower-quality compressor. Also overrides
	/// `useGPU` if true and uses the CPU for slow-mode compression.
	/// 
	/// @param is_signed If true, compresses to the BC6S format, instead of BC6U.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc6h(const CPUInputBuffer& input, bool slow_mode, bool is_signed, void* output, bool useGpu = false, bool to_device_mem = false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to BC6H format from GPUInputBuffer, always
	/// using GPU compression. This method has only one quality level,
	/// corresponding to CPU fast-mode.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param is_signed If true, compresses to the BC6S format, instead of BC6U.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_bc6h(const GPUInputBuffer& input, bool is_signed, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////

	///////////////////////////// ASTC ///////////////////////////////

	/// Interface for compressing to ASTC format from CPUInputBuffer.
	/// This supports 4 quality levels on both the CPU and GPU.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in CPU memory.
	/// 
	/// @param qualityLevel The quality level, 0, 1, 2, or 3. Higher quality
	/// levels produce less compression error, but take longer.
	/// 
	/// @param imageHasAlpha Specifies that some pixels in the image have an
	/// alpha value less than 1.0f. If false, this makes compression slightly
	/// faster. It's still valid to set it to true even if the image is opaque.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param useGpu Whether to run the compression algorithm on the GPU as
	/// opposed to the CPU. See note on `slow_mode`.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_astc(const CPUInputBuffer& input, int qualityLevel, bool imageHasAlpha, void* output, bool useGpu = false, bool to_device_mem= false, nvtt::TimingContext *tc = nullptr);

	/// Interface for compressing to ASTC format from GPUInputBuffer, always
	/// using GPU compression. This supports 4 quality levels.
	/// 
	/// @deprecated `nvtt_encode()` is more general and consistent.
	/// This function is now only a wrapper for an `nvtt_encode()` call.
	/// 
	/// @param input Constant reference to input in GPU memory.
	/// 
	/// @param qualityLevel The quality level, 0, 1, 2, or 3. Higher quality
	/// levels produce less compression error, but take longer.
	/// 
	/// @param imageHasAlpha Specifies that some pixels in the image have an
	/// alpha value less than 1.0f. If false, this makes compression slightly
	/// faster. It's still valid to set it to true even if the image is opaque.
	/// 
	/// @param output Pointer to output: CUDA device pointer if `to_device_mem`
	/// is true, and a pointer to CPU memory otherwise.
	/// 
	/// @param to_device_mem Specifies that `output` is a CUDA device pointer,
	/// rather than a pointer to CPU memory.
	/// 
	/// @param tc Timing context for recording performance information.
	NVTT_DEPRECATED_API void nvtt_encode_astc(const GPUInputBuffer& input, int qualityLevel, bool imageHasAlpha, void* output, bool to_device_mem = true, nvtt::TimingContext *tc = nullptr);

	//////////////////////////////////////////////////////////////////
}

#endif // nvtt_lowlevel_h
