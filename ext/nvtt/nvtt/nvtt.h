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
/// Header of the high-level interface of NVTT.

#ifndef NVTT_H
#define NVTT_H

#include "nvtt_lowlevel.h"

/// This library's version number, stored as 10000*fork + 100*major + minor.
/// This can also be read via nvtt::version().
#define NVTT_VERSION 30205

/// Prevents a class from being copied.
#define NVTT_FORBID_COPY(Class) \
	private: \
		Class(const Class &); \
		void operator=(const Class &); \
	public:

/// @brief Hides the members of the implementation of the class behind a `Private` structure.
/// @see nvtt::Surface::data()
#define NVTT_DECLARE_PIMPL(Class) \
	public: \
		struct Private; \
		Private & m;

/// Namespace for all public non-macro NVTT fields.
namespace nvtt
{
	/// Return the NVTT version number, as 10000*fork + 100*major + minor.
	NVTT_API unsigned int version();

	// Forward declarations.
	struct Surface;
	struct CubeSurface;
	struct TimingContext;
	struct BatchList;

	/// Compression options. This class describes the desired compression format and other compression settings.
	struct CompressionOptions
	{
		NVTT_FORBID_COPY(CompressionOptions)
		NVTT_DECLARE_PIMPL(CompressionOptions)

		/// Constructor. Sets compression options to the default values.
		NVTT_API CompressionOptions();

		/// Destructor.
		NVTT_API ~CompressionOptions();

		/// Set default compression options.
		NVTT_API void reset();

		/// Set desired compression format.
		NVTT_API void setFormat(Format format);

		/// Set compression quality settings.
		NVTT_API void setQuality(Quality quality);

		/// @brief Set the weights of each color channel used to measure compression error.
		/// 
		/// The choice for these values is subjective. In most cases uniform color weights
		/// (1.0, 1.0, 1.0) work very well. A popular choice is to use the NTSC luma encoding 
		/// weights (0.2126, 0.7152, 0.0722), but I think that blue contributes to our 
		/// perception more than 7%. A better choice in my opinion is (3, 4, 2).
		NVTT_API void setColorWeights(float red, float green, float blue, float alpha = 1.0f);

		/// @brief Describes an RGB/RGBA format using 32-bit masks per channel.
		/// 
		/// Note that this sets the number of bits per channel to 0.
		NVTT_API void setPixelFormat(unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask);

		/// @brief Describes an RGB/RGBA format using the number of bits per channel.
		/// 
		/// Note that this sets the masks per channel to 0.
		NVTT_API void setPixelFormat(unsigned char rsize, unsigned char gsize, unsigned char bsize, unsigned char asize);

		/// @brief Set pixel type.
		/// 
		/// These are used for Format_RGB: they indicate how the
		/// output should be interpreted, but do not have any influence over
		/// the input. They are ignored for other compression modes.
		NVTT_API void setPixelType(PixelType pixelType);

		/// Set pitch alignment in bytes.
		NVTT_API void setPitchAlignment(int pitchAlignment);

		/// @brief Set quantization options.
		/// 
		/// @warning Do not enable dithering unless you know what you are doing. Quantization 
		/// introduces errors. It's better to let the compressor quantize the result to 
		/// minimize the error, instead of quantizing the data before handling it to
		/// the compressor.
		NVTT_API void setQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold = 127);

		/// @brief Translates to a D3D format.
		///
		/// Returns 0 if no corresponding format could be found.
		/// 
		/// For Format_RGB, this looks at the pixel type and pixel format to
		/// determine the corresponding D3D format.
		/// For BC6, BC7, and ASTC, this returns a FourCC code: 'BC6H' for
		/// both unsigned and signed BC6, 'BC7L' for BC7, and 'ASTC' for all
		/// ASTC formats.
		NVTT_API unsigned int d3d9Format() const;
	};

	/// Wrap modes. Specifies how to handle coordinates outside the typical image range.
	enum WrapMode
	{
		/// Coordinates are clamped, moving them to the closest coordinate
		/// inside the image.
		WrapMode_Clamp,
		/// The image is treated as if it repeats on both axes, mod each
		/// dimension. For instance, for a 4x4 image, (5, -2) wraps to (1, 2).
		WrapMode_Repeat,
		/// Coordinates are treated as if they reflect every time they pass
		/// through the center of an edge texel. For instance, for a 10x10
		/// image, (8, 0), (10, 0), (26, 0), and (28, 0) all mirror to (8, 0).
		WrapMode_Mirror,
	};

	/// Texture types. Specifies the dimensionality of a texture.
	enum TextureType
	{
		TextureType_2D,
		TextureType_Cube,
		TextureType_3D,
	};

	/// @brief Input formats. Used when creating an nvtt::Surface from an RGB/RGBA array.
	enum InputFormat
	{
		InputFormat_BGRA_8UB,   ///< [0, 255] 8 bit uint
		InputFormat_BGRA_8SB,	///< [-127, 127] 8 bit int
		InputFormat_RGBA_16F,   ///< 16 bit floating point.
		InputFormat_RGBA_32F,   ///< 32 bit floating point.
		InputFormat_R_32F,      ///< Single channel 32 bit floating point.
	};

	/// @brief Mipmap downsampling filters. Each of these can be customized using
	/// `filterWidth` and `params` when calling Surface::buildNextMipmap().
	enum MipmapFilter
	{
		/// Box filter is quite good and very fast. It has some special paths
		/// for downsampling by exactly a factor of 2.
		/// `filterWidth` defaults to 0.5; `box(x)` is equal to 1 when
		/// `|x| < filterWidth` and 0 otherwise.
		MipmapFilter_Box,       
		/// Triangle filter blurs the results too much, but that might be what you want.
		/// `filterWidth` defaults to 1.0; `triangle(x)` is equal to
		/// `filterWidth - |x|` when `|x| < filterWidth` and 0 otherwise.
		MipmapFilter_Triangle,
		/// Kaiser-windowed Sinc filter is the best downsampling filter, and
		/// close to a mathematically ideal windowing filter. If the window
		/// size is too large, it can introduce ringing.
		/// 
		/// `filterWidth` controls the width of the Kaiser window. Larger
		/// values take longer to compute and include more oscillations of the
		/// sinc filter.
		/// 
		/// `param[0]` (default: 4.0f) sets `alpha`, the sharpness of the
		/// Kaiser window. Higher values make the main lobe wider, but reduce
		/// sideband energy.
		/// 
		/// `param[1]` (default: 1.0f) controls the frequency of the sinc
		/// filter. Higher values include higher frequencies.
		/// 
		/// @see https://en.wikipedia.org/wiki/Kaiser_window
		MipmapFilter_Kaiser,
		/// Mitchell & Netravali's two parameter cubic filter.
		/// @see nvtt::MipmapFilter::MipmapFilter_Mitchell
		///
		/// `filterWidth` (default: 2.0f) can truncate the filter, but should
		/// usually be left at the default.
		/// 
		/// `param[0]` (default: 1/3) sets B.
		/// 
		/// `param[1]` (default: 2/3) sets C.
		/// 
		/// @see "Reconstruction Filters in Computer Graphics", SIGGRAPH 1988
		/// 
		/// @see https://en.wikipedia.org/wiki/Mitchell%E2%80%93Netravali_filters
		MipmapFilter_Mitchell,
		/// Takes the minimum over all input texels that contribute to each
		/// output texel.
		///
		/// This is especially useful for generating mipmaps for parallax
		/// occlusion mapping, or for structures like hierarchical Z-buffers.
		///
		/// Specifically, this acts as if for an X x Y x Z image, the texel
		/// at (i, j, k) covers the open box
		/// (i/X, (i+1)/X) x (j/Y, (j+1)/Y) x (k/Z, (k+1)/Z).
		///
		/// Then for each output texel, the set of contributing texels is the
		/// set of input texels whose boxes intersect the output texel's box.
		MipmapFilter_Min,
		/// Like MipmapFilter_Min, but takes the maximum over all contributing
		/// texels instead of the minimum.
		MipmapFilter_Max,
	};

	/// Texture resizing filters.
	enum ResizeFilter
	{
		/// Box filter. Fast, but produces nearest-neighbor artifacts when
		/// upsampling.
		/// @see nvtt::MipmapFilter::MipmapFilter_Box
		ResizeFilter_Box,
		/// Triangle (tent) filter. It can blur the results too much, but that
		/// might be what you want.
		/// @see nvtt::MipmapFilter::MipmapFilter_Triangle
		ResizeFilter_Triangle,
		/// Kaiser-windowed Sinc filter.
		/// @see nvtt::MipmapFilter::MipmapFilter_Kaiser
		ResizeFilter_Kaiser,
		/// Mitchell & Netravali's two parameter cubic filter.
		/// @see nvtt::MipmapFilter::MipmapFilter_Mitchell
		ResizeFilter_Mitchell,
		/// Takes the minimum over all contributing texels.
		/// @see nvtt::MipmapFilter::MipmapFilter_Min
		ResizeFilter_Min,
		/// Takes the maximum over all contributing texels.
		/// @see nvtt::MipmapFilter::MipmapFilter_Max
		ResizeFilter_Max
	};

	/// @brief Extents rounding mode.
	/// 
	/// Determines how to round sizes to different sets when shrinking an image.
	/// 
	/// For each of the PowerOfTwo modes, `maxExtent` is first rounded to the
	/// previous power of two.
	/// 
	/// Then all extents are scaled and truncated without changing the aspect
	/// ratio, using `s = max((s * maxExtent) / m, 1)`, where `m` is the
	/// maximum width, height, or depth.
	/// 
	/// If the texture is a cube map, the width and height are then averaged
	/// to make the resulting texture square.
	/// 
	/// Finally, extents are rounded to a set of possible sizes depending on
	/// this enum.
	/// 
	/// @see getTargetExtent()
	enum RoundMode
	{
		RoundMode_None, ///< Each extent is left as-is.
		RoundMode_ToNextPowerOfTwo, ///< Each extent is rounded up to the next power of two.
		RoundMode_ToNearestPowerOfTwo, ///< Each extent is rounded either up or down to the nearest power of two.
		RoundMode_ToPreviousPowerOfTwo, ///< Each element is rounded down to the next power of two.
	};

	/// Alpha mode.
	enum AlphaMode
	{
		/// This image has no alpha.
		/// The alpha channel will be ignored in some forms of compression.
		AlphaMode_None,
		/// Alpha represents opacity; for instance, (r, g, b, 0.5) is a
		/// 50% opaque (r, g, b) color.
		AlphaMode_Transparency,
		/// Colors are stored using premultiplied alpha: (a*r, a*g, a*b, a) is
		/// an (r, g, b) color with an opacity of a.
		/// This is mostly for tracking purposes; compressors only distinguish
		/// between AlphaMode_None and AlphaMode_Transparency. 
		AlphaMode_Premultiplied,
	};

	/// @brief Inheritable interface for outputting data.
	/// 
	/// For instance, one can inherit from this to write to a stream, a buffer
	/// in memory, or a custom data structure.
	struct OutputHandler
	{
		/// Destructor.
		virtual ~OutputHandler() {}

		/// Indicate the start of a new compressed image that's part of the final texture.
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) = 0;

		/// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
		virtual bool writeData(const void * data, int size) = 0;

		/// Indicate the end of the compressed image. (New in NVTT 2.1)
		virtual void endImage() = 0;
	};

	/// @brief Error codes.
	///
	/// @see nvtt::ErrorHandler
	/// @see errorString()
	enum Error
	{
		Error_None, ///< No error.
		Error_Unknown = Error_None, ///< Alias used before NVTT 3.2 for Error_None.
		Error_InvalidInput, ///< The input to the function was invalid (for instance, a negative size).
		Error_UnsupportedFeature, ///< Unsupported feature.
		Error_CudaError, ///< CUDA reported an error during an operation.
		Error_FileOpen, ///< I/O error attempting to open the given file.
		Error_FileWrite, ///< I/O error attempting to write to the given file.
		Error_UnsupportedOutputFormat, ///< The chosen container does not support the requested format (for instance, attempting to store BC7 data in a DDS file without the DX10 header.)
		Error_Messaging, ///< Internal error while invoking the message callback. @since NVTT 3.2
		Error_OutOfHostMemory, ///< Out of host memory (allocating a CPU-side buffer failed). @since NVTT 3.2
		Error_OutOfDeviceMemory, ///< Out of device memory (allocating a GPU-side buffer failed). @since NVTT 3.2
		Error_OutputWrite, ///< OutputOptions::writeData() returned false. @since NVTT 3.2
		Error_Count
	};

	/// Return string for the given error code.
	NVTT_API const char* errorString(Error e);

	/// Inheritable interface for handling errors.
	struct ErrorHandler
	{
		/// Destructor.
		virtual ~ErrorHandler() {}

		/// Called to signal an error.
		virtual void error(Error e) = 0;
	};

	/// @brief Message severity.
	/// @since NVTT 3.2
	enum Severity
	{
		/// An informative message, such as statistics or current computation
		/// progress.
		Severity_Info,
		/// A warning. For instance, an app may get a warning if it tries to
		/// enable CUDA acceleration, but no CUDA driver is available (which
		/// requires NVTT to use its CPU fallbacks).
		Severity_Warning,
		/// An error. For instance, an API may have been called incorrectly,
		/// or CUDA may have run out of memory. The `Error` field of the
		/// message callback will be set to an appropriate value.
		Severity_Error,
		Severity_Count
	};

	/// A MessageCallback is a function that takes a message `Severity`,
	/// an error enumeration (only nonzero for error messages),
	/// a null-terminated description of the message (usually with useful
	/// information for debugging; no newline at end), and a custom
	/// user pointer set when calling `nvtt::SetMessageCallback()`.
	/// @since NVTT 3.2
	typedef void (*MessageCallback)(Severity severity, Error error, const char* message, const void* userData);

	/// Sets the current thread's NVTT `MessageCallback`.
	/// NVTT calls this function whenever it encounters an error or performance
	/// warning, or has useful information. Here's an example showing how to
	/// use a message callback to detect and print errors:
	/// ```cpp
	/// thread_local bool caughtError = false;
	/// void customCallback(nvtt::Severity severity, nvtt::Error error,
	///                     const char* message, const void* userData)
	/// {
	///     if (severity == nvtt::Severity_Error)
	///     {
	///         printf("ERROR: %s\n", message);
	///         caughtError = true;
	///     }
	/// }
	/// 
	/// // Example processing function; returns true on success
	/// bool exampleProcessing(...)
	/// {
	///     nvtt::setMessageCallback(customCallback, nullptr);
	///     GPUInputBuffer input(...);
	///     nvtt_encode_bc7(input, ...);
	///     return !caughtError;
	/// }
	/// ```
	/// Since each thread has its own callback pointer, a custom callback
	/// should be set whenever an app creates a new thread. If no callback has
	/// been set, the default callback prints the severity and the message,
	/// followed by a newline, to stdout. If a callback has been set, setting
	/// the callback to nullptr will make NVTT switch to the default callback.
	/// Returns true if setting the message callback succeeded.
	/// @since NVTT 3.2
	NVTT_API bool setMessageCallback(MessageCallback callback, const void* userData);

	/// @brief Container type for encoded data.
	///
	/// @note For DDS containers, NVTT stores some additional data in the
	/// `reserved[]` fields to allow consumers to detect writer versions.
	/// - `reserved[7]` is the FourCC code "UVER", and `reserved[8]` stores a
	/// version number that can be set by the user.
	/// - `reserved[9]` is the FourCC code "NVTT", and `reserved[10]` is the
	/// NVTT writer version (which isn't necessarily the same as `nvtt::version()`).
	/// 
	/// @note For DDS containers, NVTT also extends the `dwFlags` field with two
	/// more flags.
	/// - `DDPF_SRGB` (`0x40000000U`) indicates that the texture uses an sRGB
	/// transfer function. Note that most readers will ignore this and instead
	/// guess the transfer function from the format.
	/// - `DDPF_NORMAL` (`0x80000000U`) indicates that the texture is a normal map.
	enum Container
	{
		Container_DDS, ///< DDS without the DX10 header extension. Compatible with legacy readers, but doesn't support BC6 or BC7.
		Container_DDS10, ///< DDS without the DX10 header. Supports BC6 and BC7, but may be unreadable by legacy readers.
		// Container_KTX,   // Khronos Texture: http://www.khronos.org/opengles/sdk/tools/KTX/
		// Container_VTF,   // Valve Texture Format: http://developer.valvesoftware.com/wiki/Valve_Texture_Format
	};

	/// @brief Output Options.
	/// 
	/// This class holds pointers to the interfaces that are used to report
	/// the output of the compressor to the app, as well as the container type
	/// and options specific to the container.
	struct OutputOptions
	{
		NVTT_FORBID_COPY(OutputOptions)
		NVTT_DECLARE_PIMPL(OutputOptions)

		NVTT_API OutputOptions();
		NVTT_API ~OutputOptions();

		/// Set default options.
		NVTT_API void reset();

		/// Set output file name. This uses the same character encoding as the
		/// `filename` argument of `fopen()`.
		NVTT_API void setFileName(const char * fileName);

		/// Set output file handle.
		NVTT_API void setFileHandle(void * fp);

		/// Set output handler.
		NVTT_API void setOutputHandler(OutputHandler * outputHandler);

		/// Set error handler.
		NVTT_API void setErrorHandler(ErrorHandler * errorHandler);

		/// Set output header. Defaults to true.
		NVTT_API void setOutputHeader(bool outputHeader);

		/// Set container. Defaults to Container_DDS.
		NVTT_API void setContainer(Container container);

		/// Set user version.
		///
		/// @see #Container
		NVTT_API void setUserVersion(int version);

		/// Set the sRGB flag, indicating whether this file stores data with
		/// an sRGB transfer function (true) or a linear transfer function
		/// (false). Defaults to false.
		/// 
		/// @see #Container
		NVTT_API void setSrgbFlag(bool b);
	};

	/// Compression context.
	struct Context
	{
		NVTT_FORBID_COPY(Context)
		NVTT_DECLARE_PIMPL(Context)

		/// @brief Create a compression context.
		/// @param enableCuda If true, enables CUDA acceleration (same as calling `enableCudaAcceleration()`).
		NVTT_API Context(bool enableCuda = true);
		NVTT_API ~Context();

		/// Enable CUDA acceleration; initializes CUDA if not already initialized.
		NVTT_API void enableCudaAcceleration(bool enable);
		/// Check if CUDA acceleration is enabled
		NVTT_API bool isCudaAccelerationEnabled() const;

		/// @brief Write the #Container's header to the output.
		/// @param mipmapCount The number of mipmaps in the container.
		/// @since NVTT 2.1
		NVTT_API bool outputHeader(const Surface & img, int mipmapCount, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
		/// @brief Compress the Surface and write the compressed data to the output.
		/// @param img The Surface.
		/// @param face Ignored.
		/// @param mipmap Ignored.
		/// @param compressionOptions Specifies the compression format.
		/// @param outputOptions Specifies how to output the data.
		/// @since NVTT 2.1
		NVTT_API bool compress(const Surface & img, int face, int mipmap, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
		/// @brief Returns the total compressed size of mips 0...`mipmapCount-1`,
		/// without compressing the image.
		/// 
		/// Note that this does not include the container header, and mips are
		/// assumed to be tightly packed.
		/// 
		/// For instance, call this with `mipmapCount = img.countMipmaps()` and
		/// add the size of the DDS header to get the size of a DDS file with
		/// a surface and a full mip chain.
		/// 
		/// @param img The Surface.
		/// @param mipmapCount The number of mips (each with size `max(1, s/2)` of the previous) in the mip chain to be compressed.
		/// @param compressionOptions Specifies the compression format.
		/// @since NVTT 2.1
		NVTT_API int estimateSize(const Surface & img, int mipmapCount, const CompressionOptions & compressionOptions) const;

		/// @brief Quantize a Surface to the number of bits per channel of the given format.
		/// 
		/// This shouldn't be called unless you're confident you want to do
		/// this. Compressors quantize automatically, and calling this will
		/// cause compression to minimize error with respect to the quantized
		/// values, rather than the original image.
		///  
		/// @see nvtt::Surface::quantize()
		/// @see nvtt::Surface::binarize()
		/// @since NVTT 3.0
		NVTT_API void quantize(Surface & tex, const CompressionOptions & compressionOptions) const;

		/// @brief Write the #Container's header to the output.
		/// @param cube The CubeSurface to read sizes from.
		/// @param mipmapCount The number of mipmaps in the container.
		/// @since NVTT 2.1
		NVTT_API bool outputHeader(const CubeSurface & cube, int mipmapCount, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
		/// @brief Compress the CubeSurface and write the compressed data to the output.
		/// @param cube The CubeSurface.
		/// @param mipmap Ignored.
		/// @param compressionOptions Specifies the compression format.
		/// @param outputOptions Specifies how to output the data.
		/// @since NVTT 2.1
		NVTT_API bool compress(const CubeSurface & cube, int mipmap, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
		/// @brief Returns the total compressed size of mips 0...`mipmapCount-1`,
		/// without compressing the image.
		/// 
		/// Note that this does not include the container header, and mips are
		/// assumed to be tightly packed.
		/// 
		/// For instance, call this with `mipmapCount = cube.countMipmaps()` and
		/// add the size of the DDS header to get the size of a DDS file with
		/// a CubeSurface and a full mip chain.
		/// 
		/// @param cube The CubeSurface.
		/// @param mipmapCount The number of mips (each with size `max(1, s/2)` of the previous) in the mip chain to be compressed.
		/// @param compressionOptions Specifies the compression format.
		/// @since NVTT 2.1
		NVTT_API int estimateSize(const CubeSurface & cube, int mipmapCount, const CompressionOptions & compressionOptions) const;

		/// @brief Write the #Container's header to the output.
		/// @param mipmapCount The number of mipmaps in the container.
		/// @since NVTT 2.1
		NVTT_API bool outputHeader(TextureType type, int w, int h, int d, int mipmapCount, bool isNormalMap, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
		/// @brief Compress raw data and write the compressed data to the output.
		/// 
		/// Note that this only supports CPU compression. For GPU support and
		/// more options, please see nvtt_lowlevel.h.
		/// @param face Ignored.
		/// @param mipmap Ignored.
		/// @param rgba Color data. Assumed to be non-interleaved; i.e. the value of channel c, pixel (x, y, z) is `rgba[((c*d + z)*h + y)*w + x]`.
		/// @param compressionOptions Specifies the compression format.
		/// @param outputOptions Specifies how to output the data.
		/// @since NVTT 2.1
		NVTT_API bool compress(int w, int h, int d, int face, int mipmap, const float * rgba, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
		/// @brief Returns the total compressed size of mips 0...`mipmapCount-1`,
		/// without compressing the image.
		/// 
		/// Note that this does not include the container header, and mips are
		/// assumed to be tightly packed.
		/// 
		/// For instance, call this with `mipmapCount = nvtt::countMipmaps()` and
		/// add the size of the DDS header to get the size of a DDS file with
		/// a CubeSurface and a full mip chain.
		/// 
		/// @param w Width.
		/// @param h Height.
		/// @param d Depth.
		/// @param mipmapCount The number of mips (each with size `max(1, s/2)` of the previous) in the mip chain to be compressed.
		/// @param compressionOptions Specifies the compression format.
		/// @since NVTT 2.1
		NVTT_API int estimateSize(int w, int h, int d, int mipmapCount, const CompressionOptions & compressionOptions) const;

		/// @brief Batch processing compression interface.
		/// 
		/// Compresses multiple inputs in a row, improving performance.
		/// Outputs data to each item's `OutputHandler`.
		/// 
		/// @see BatchList
		/// @since NVTT 3.0
		NVTT_API bool compress(const BatchList & lst, const CompressionOptions & compressionOptions) const;

		/// @brief Enables performance measurement. May introduce additional synchronization.
		/// @see TimingContext
		/// @since NVTT 3.0
		NVTT_API void enableTiming(bool enable, int detailLevel = 1);
		/// @brief Returns a TimingContext containing recorded compression performance information.
		/// @since NVTT 3.0
		NVTT_API TimingContext* getTimingContext();

	};

	/// @brief Specifies a normal transformation, used to store 3D (x, y, z) normals in 2D (x, y).
	/// 
	/// We define these in terms of their 2D -> 3D reconstructions, since their
	/// transformations are the inverse of the reconstructions. Most require
	/// z >= 0.0f.
	/// 
	/// @see Surface::transformNormals()
	/// @see Surface::reconstructNormals()
	/// @since NVTT 2.1
	enum NormalTransform {
		/// Reconstructs the z component using `z = sqrt(1 - x^2 + y^2)`.
		NormalTransform_Orthographic,
		/// Stereographic projection (like looking from the bottom of the sphere
		/// of normals and projecting points onto a plane at z = 1).
		/// Reconstructed using `d = 2/(1 + min(x^2 + y^2, 1)); return (x*d, y*d, d-1)`.
		NormalTransform_Stereographic,
		/// Reconstructed using `normalize(x, y, 1 - min(x^2 + y^2, 1))`.
		NormalTransform_Paraboloid,
		/// Reconstructed using `normalize(x, y, (1-x^2)(1-y^2))`.
		NormalTransform_Quartic
		//NormalTransform_DualParaboloid,
	};

	/// @brief Tone mapping functions.
	/// @since NVTT 2.1
	enum ToneMapper {
		/// Colors inside [0,1)^3 are preserved; colors outside are tone mapped
		/// using (r', g', b') = (r, g, b)/max(r, g, b). This clamps colors to
		/// the RGB cube, but preserves hue. It is not invertible.
		ToneMapper_Linear,
		/// Applies a Reinhard operator to each channel: c' = c / (c + 1).
		ToneMapper_Reinhard,
		/// Name for backwards compatibility. @see #ToneMapper_Reinhard
		ToneMapper_Reindhart = ToneMapper_Reinhard,
		/// Applies an exponential tone mapper to each channel: c' = 1 - 2^(-c).
		ToneMapper_Halo,
		/// Same as #ToneMapper_Linear.
		ToneMapper_Lightmap,
	};

	/// @brief A surface is one level of a 2D or 3D texture.
	/// 
	/// A surface has four channels numbered 0-3, also referred to as the
	/// red, green, blue, and alpha channels.
	/// 
	/// Surfaces store some additional properties, such as their width, height,
	/// depth, wrap mode, alpha mode, and whether they represent a normal map.
	/// 
	/// Surfaces can have CPU and GPU data. If a surface has GPU data, it
	/// always has CPU data. When the surface is GPU-enabled (using ToGPU()),
	/// image processing will be CUDA-accelerated and work on this GPU data.
	/// Calling ToCPU() will copy the GPU data to the CPU and destroy the GPU
	/// data buffer. `gpuData() != nullptr` can be used to determine if a
	/// surface has GPU data.
	/// 
	/// To directly access CPU data, use data() or channel(). To get a pointer
	/// to the GPU data buffer, use gpuData() (for a const CUDA pointer) or
	/// gpuDataMutable() (for a CUDA pointer to modifiable data).
	/// 
	/// Texture data is stored non-interleaved; that is, all channel 0's data
	/// is stored first, followed by channel 1's, and so on.
	/// 
	/// @note Performance note: Surfaces use reference-counted pointers to
	/// image data. This means that multiple Surfaces can reference the same
	/// data. This is handled automatically by NVTT's image processing routines.
	/// For instance, after the following piece of code,
	/// ```cpp
	/// nvtt::Surface s1;
	/// s1.setImage(...);
	/// nvtt::Surface s2 = s1;
	/// ```
	/// surfaces `s1` and `s2` will have the same `data()` pointer. Cloning the
	/// underlying data is handled automatically: for instance, after
	/// ```cpp
	/// s2.toSrgb()
	/// ```
	/// `s2` will have a new `data()` pointer, and `s1` will be unchanged.
	/// This usually means that when writing custom image processing routines,
	/// you'll want to call Surface::clone() before modifying the Surface's
	/// data. Using Nsight Systems is one way to detect when additional
	/// allocations occur.
	/// 
	/// @since NVTT 2.1
	// @@ It would be nice to add support for texture borders for correct resizing of tiled textures and constrained DXT compression.
	struct Surface
	{
		/// @brief Creates an empty surface. All data will be null until a
		/// setImage() function is called.
		NVTT_API Surface();
		/// Copy constructor.
		NVTT_API Surface(const Surface & img);
		/// Destructor.
		NVTT_API ~Surface();

		/// Assignment operator.
		NVTT_API void operator=(const Surface & img);
		/// Creates a deep copy of this Surface, with its own internal data. (New in NVTT 3.1)
		NVTT_API Surface clone() const;

		/// @brief Set the Surface's wrap mode. See #WrapMode for details.
		NVTT_API void setWrapMode(WrapMode mode);
		/// @brief Set the Surface's alpha mode. See #AlphaMode for details.
		NVTT_API void setAlphaMode(AlphaMode alphaMode);
		/// @brief Set whether the Surface represents a normal map. This can
		/// be accessed using isNormalMap(), and e.g. affects whether DDS
		/// files are written with the normal map flag.
		NVTT_API void setNormalMap(bool isNormalMap);

		/// @brief Returns if the surface is null (i.e. refers to nothing, such
		/// as if it was just created using Surface()).
		NVTT_API bool isNull() const;
		/// Returns the width (X size) of the surface.
		NVTT_API int width() const;
		/// Returns the height (Y size) of the surface.
		NVTT_API int height() const;
		/// Returns the depth (Z size) of the surface. 1 for 2D surfaces.
		NVTT_API int depth() const;
		/// @brief Returns the dimensionality of the surface. This is set for
		/// instance by setImage().
		NVTT_API TextureType type() const;
		/// @brief Returns the wrap mode of the surface. See setWrapMode()
		NVTT_API WrapMode wrapMode() const;
		/// @brief Returns the alpha mode of the surface. See setAlphaMode()
		NVTT_API AlphaMode alphaMode() const;
		/// @brief Returns whether the image represents a normal map. See setNormalMap()
		NVTT_API bool isNormalMap() const;
		/// @brief Returns the number of mipmaps in a full mipmap chain.
		/// Each mip is half the size of the previous, rounding down, until and
		/// including a 1x1 mip.
		///
		/// For instance, a 8x5 surface has mipmaps of size 8x5 (mip 0),
		/// 4x2 (mip 1), 2x1 (mip 2), and 1x1 (mip 3), so countMipmaps()
		/// returns 4. A 7x3 surface has mipmaps of size 7x3, 3x1, and 1x1, so
		/// countMipmaps() returns 3.
		NVTT_API int countMipmaps() const;
		/// @brief Returns the number of mipmaps in a mipmap chain, stopping
		/// when canMakeNextMipmap() returns false.
		/// 
		/// That is, it stops when a 1x1x1 mip is reached if `min_size == 1`
		/// (in which case it is the same as countMipmaps()), or stops when
		/// the width and height are less than `min_size` and the depth is 1.
		/// 
		/// @see canMakeNextMipmap().
		NVTT_API int countMipmaps(int min_size) const;
		/// @brief Returns the approximate fraction (0 to 1) of the image with
		/// an alpha value greater than `alphaRef`.
		///
		/// This function uses 8 x 8 subsampling together with
		/// linear interpolation.
		/// 
		/// @note `alphaRef` is clamped to the range [1/256, 255/256].
		NVTT_API float alphaTestCoverage(float alphaRef = 0.5, int alpha_channel = 3) const;
		/// @brief Computes the average of a channel, possibly with alpha or
		/// with a gamma transfer function.
		/// 
		/// If `alpha_channel` is -1, this function computes
		/// 
		/// `(sum(c[i]^gamma, i=0...numPixels)/numPixels)^(1/gamma)`
		/// 
		/// where `c` is the channel's data.
		/// 
		/// Otherwise, this computes
		/// 
		/// `(sum((c[i]^gamma) * a[i], i=0...numPixels)/sum(a[i], i=0...numPixels))^(1/gamma)`
		/// 
		/// where `a` is the alpha channel's data.
		/// 
		/// @param channel The channel to average.
		/// @param alpha_channel If not equal to -1, weights each texel's value
		/// after gamma correction by this channel.
		/// @param gamma Gamma for the transfer function to apply. A value of
		/// 2.2f roughly corresponds to averaging an sRGB image in linear space,
		/// then converting back to sRGB.
		NVTT_API float average(int channel, int alpha_channel = -1, float gamma = 2.2f) const;

		/// @brief Returns a `const` pointer to the surface's CPU data.
		/// 
		/// Data is stored in [c, z, y, x] order; that is, all channel 0's data
		/// comes first, followed by all channel 1's data, and so on.
		/// More specifically, the value of channel c of the texel at (x, y, z)
		/// is at index
		/// 
		/// `((c * depth() + z) * height() + y) * width() + x`.
		/// 
		/// @note If the image has GPU data (see ToGPU()), this performs a
		/// GPU-to-CPU copy.
		NVTT_API const float * data() const;
		/// @brief Returns a `const` pointer to channel i's CPU data.
		/// 
		/// Data is stored in [z, y, x] order; that is, all channel 0's data
		/// comes first, followed by all channel 1's data, and so on.
		/// More specifically, the value of channel c of the texel at (x, y, z)
		/// is at index
		/// 
		/// `(z * height() + y) * width() + x`
		/// 
		/// of channel(c).
		/// 
		/// @note If the image has GPU data (see ToGPU()), this performs a
		/// GPU-to-CPU copy.
		NVTT_API const float * channel(int i) const;

		/// @brief Returns a pointer that can be used to modify the surface's CPU data. See data() const.
		NVTT_API float * data();
		/// @brief Returns a pointer that can be used to modify channel i's CPU data. See channel(int) const.
		NVTT_API float * channel(int i);

		/// @brief Stores a histogram of channel values between `rangeMin` and
		/// `rangeMax` into `binPtr[0...binCount-1]`.
		///
		/// `binPtr` must be an array of at least `binCount` integers.
		/// This function does not clear `binPtr`'s values, in case we want to
		/// accumulate multiple histograms.
		/// 
		/// Each texel's value is linearly mapped to a bin, using floor
		/// rounding. Values below `rangeMin` are clamped to bin 0, values
		/// above `rangeMax` are clamped to bin `binCount-1`. Then the bin's
		/// value is incremented.
		NVTT_API void histogram(int channel, float rangeMin, float rangeMax, int binCount, int * binPtr, TimingContext *tc = 0) const;
		/// @brief Sets `*rangeMin` and `*rangeMax` to the range of values in
		/// the channel, possibly using alpha testing.
		///
		/// If `alpha_channel` is -1, this sets `*rangeMin` to the smallest
		/// value in the entire channel, and `*rangeMax` to the largest value
		/// in the entire channel. Otherwise, this only includes texels for
		/// which the alpha value is greater than `alpha_ref`.
		/// 
		/// If the image is null or if an alpha channel is selected and all
		/// texels fail the alpha test, this sets `*rangeMin` to `FLT_MAX` and
		/// `*rangeMax` to `FLT_MIN`, i.e. one will have `*rangeMin > *rangeMax`.
		NVTT_API void range(int channel, float * rangeMin, float * rangeMax, int alpha_channel = -1, float alpha_ref = 0.f, TimingContext *tc = 0) const;

		/// @brief Loads texture data from a file.
		///
		/// @returns `true` if loading succeeded and `false` if it failed.
		///
		/// @param fileName Path to the file to load. This uses the same
		/// character encoding as the `filename` argument of `fopen()`.
		/// @param hasAlpha If this is non-null, then `*hasAlpha` will be set
		/// to whether the reader reported that the file included
		/// an alpha channel.
		/// @param expectSigned If this is true, then some forms of unsigned
		/// data will be converted to signed using the mapping x |-> 2x-1.
		NVTT_API bool load(const char * fileName, bool * hasAlpha = 0, bool expectSigned = false, TimingContext *tc = 0);
		/// @brief Variant of load() that reads from memory instead of a file.
		/// 
		/// @returns `true` if loading succeeded and `false` if it failed.
		///
		/// @param data Pointer to the start of the file's data in memory.
		/// @param sizeInBytes Length of the file's data.
		/// @param hasAlpha If this is non-null, then `*hasAlpha` will be set
		/// to whether the reader reported that the file included
		/// an alpha channel.
		/// @param expectSigned If this is true, then some forms of unsigned
		/// data will be converted to signed using the mapping x |-> 2x-1.
		NVTT_API bool loadFromMemory(const void * data, unsigned long long sizeInBytes, bool * hasAlpha = 0, bool expectSigned = false, TimingContext *tc = 0);
		/// @brief Saves texture data to file.
		///
		/// @returns `true` if saving succeeded and `false` if it failed.
		/// 
		/// @param fileName Path to the file to load. This uses the same
		/// character encoding as the `filename` argument of `fopen()`.
		/// @param hasAlpha If true, then TGA images will be saved with an alpha channel.
		/// @param hdr If true, then this will attempt to use a writer that supports
		/// an HDR format before attempting to use an LDR format writer.
		NVTT_API bool save(const char * fileName, bool hasAlpha = false, bool hdr = false, TimingContext *tc = 0) const;
		/// @brief Sets this surface to a new w x h x d uninitialized image.
		///
		/// Surfaces are not GPU-enabled by default. The surface's texture type
		/// will be set to TextureType_2D if `d == 1`, and TextureType_3D
		/// otherwise.
		/// @returns `true`
		NVTT_API bool setImage(int w, int h, int d, TimingContext *tc = 0);
		/// @brief Sets this surface given uncompressed input data.
		/// 
		/// The type of values in `data` should match `format`.
		/// 
		/// If `unsignedToSigned` is true, #InputFormat_BGRA_8UB unsigned input
		/// will be converted to signed values between -1 and 1, mapping 0 to
		/// -1, and 1...255 linearly to -1...1.
		/// 
		/// Returns whether setting the image succeeded.
		/// 
		/// @see setImage(int, int, int, TimingContext*)
		NVTT_API bool setImage(InputFormat format, int w, int h, int d, const void * data, bool unsignedToSigned = false, TimingContext *tc = 0);
		/// @brief Sets this surface given uncompressed input data, with different pointers for each channel.
		/// 
		/// The type of values in `data` should match `format`.
		/// 
		/// Returns whether setting the image succeeded.
		/// 
		/// @see setImage(int, int, int, TimingContext*)
		NVTT_API bool setImage(InputFormat format, int w, int h, int d, const void * r, const void * g, const void * b, const void * a, TimingContext *tc = 0);
		/// @brief Set 2D surface values from an encoded data source. Same as setImage3D() with `d=1`.
		NVTT_API bool setImage2D(Format format, int w, int h, const void * data, TimingContext *tc = 0);
		/// @brief Set surface values from an encoded data source.
		///
		/// For instance, this can be used to decompress BC1-BC7 or ASTC data.
		/// 
		/// Returns whether setting the image succeeded.
		NVTT_API bool setImage3D(Format format, int w, int h, int d, const void * data, TimingContext *tc = 0);

		/// Resizes this surface to have size w x h x d using a given filter.
		NVTT_API void resize(int w, int h, int d, ResizeFilter filter, TimingContext *tc = 0);
		/// Resizes this surface using customizable filter parameters.
		///
		/// @see #ResizeFilter for filter-specific parameters.
		NVTT_API void resize(int w, int h, int d, ResizeFilter filter, float filterWidth, const float * params = 0, TimingContext *tc = 0);
		/// @brief Resizes this surface so that its largest side has length `maxExtent`, subject to a rounding mode.
		/// @see getTargetExtent()
		NVTT_API void resize(int maxExtent, RoundMode mode, ResizeFilter filter, TimingContext *tc = 0);
		/// @brief Resizes this surface so that its largest side has length `maxExtent`, subject to a rounding mode, using customizable filter parameters.
		/// @see getTargetExtent()
		NVTT_API void resize(int maxExtent, RoundMode mode, ResizeFilter filter, float filterWidth, const float * params = 0, TimingContext *tc = 0);
		/// @brief Resizes this surface so that its longest side has length `maxExtent` and the result is square or cubical.
		///
		/// For 2D surfaces, the size is determined using getTargetExtent(),
		/// then using the minimum of the width and height. For 3D surfaces,
		/// the size is similarly determined using getTargetExtent(), then
		/// using the minimum of the width, height, or depth.
		NVTT_API void resize_make_square(int maxExtent, RoundMode roundMode, ResizeFilter filter, TimingContext *tc = 0);

		/// @brief Resizes this surface to create the next mip in a mipmap chain.
		///
		/// Returns false iff the next mip would have been smaller than
		/// `min_size` (signaling the end of the mipmap chain).
		NVTT_API bool buildNextMipmap(MipmapFilter filter, int min_size = 1, TimingContext *tc = 0);
		/// @brief Version of buildNextMipmap(MipmapFilter, int, TimingContext*) with customizable parameters.
		///
		/// @see #MipmapFilter for the effects of different parameters.
		NVTT_API bool buildNextMipmap(MipmapFilter filter, float filterWidth, const float * params = 0, int min_size = 1, TimingContext *tc = 0);
		/// @brief Replaces this surface with a surface the size of the next
		/// mip in a mip chain (half the width and height), but with each
		/// channel cleared to a constant value.
		/// 
		/// @param color_components is an array of the value for each channel;
		/// it must be at least as long as the number of channels in the surface.
		NVTT_API bool buildNextMipmapSolidColor(const float * const color_components, TimingContext *tc = 0);
		/// Crops or expands this surface from the (0,0,0) corner, with any new values cleared to 0.
		NVTT_API void canvasSize(int w, int h, int d, TimingContext *tc = 0);
		/// @brief Returns whether a the surface would have a next mip in a mip
		/// chain with minimum size `min_size.
		/// 
		/// That is, it returns false if this surface has size 1x1x1, or if
		/// the width and height are less than `min_size` and the depth is 1.
		NVTT_API bool canMakeNextMipmap(int min_size = 1);

		/// @brief Raises channels 0...2 to the power `gamma`.
		///
		/// `gamma=2.2` approximates sRGB-to-linear conversion.
		/// 
		/// @see toGamma()
		/// @see toLinearFromSrgb()
		NVTT_API void toLinear(float gamma, TimingContext *tc = 0);
		/// @brief Raises channels 0...2 to the power `1/gamma`. 
		///
		/// `gamma=2.2` approximates linear-to-sRGB conversion.
		/// 
		/// @see toLinear()
		/// @see toSrgb()
		NVTT_API void toGamma(float gamma, TimingContext *tc = 0);
		/// @brief Raises the given channel to the power `gamma`.
		/// @see toLinear(float, TimingContext*)
		NVTT_API void toLinear(int channel, float gamma, TimingContext *tc = 0);
		/// @brief Raises the given channel to the power `1/gamma`.
		/// @see toGamma(float, TimingContext*)
		NVTT_API void toGamma(int channel, float gamma, TimingContext *tc = 0);
		/// @brief Applies the linear-to-sRGB transfer function to channels 0...2.
		/// 
		/// This transfer function replaces each value `x` with
		/// ```text
		/// if x is NaN or x <= 0.0f, 0.0f
		/// if x <= 0.0031308f, 12.92f * x
		/// if x <  1.0f, powf(x, 1.0f/2.4f) * 1.055f - 0.055f
		/// otherwise, 1.0f
		/// ```
		/// 
		/// @see toLinearFromSrgb()
		NVTT_API void toSrgb(TimingContext *tc = 0);
		/// @brief Applies the linear-to-sRGB transfer function to channels 0...2, but does not clamp output to [0,1].
		///
		/// The motivation for this function is that it can approximately
		/// preserve HDR values through sRGB conversion and back; that is,
		/// s.toSrgbUnclamped().toLinearFromSrgbUnclamped() is close to s.
		///
		/// This transfer function replaces each value `x` with
		/// ```text
		/// if x is NaN or x <= 0.0f, x
		/// if x <= 0.0031308f, 12.92f * x
		/// otherwise, powf(x, 1.0f/2.4f) * 1.055f - 0.055f
		/// ```
		///
		/// @since NVTT 3.2
		/// @see toLinearFromSrgbUnclamped()
		NVTT_API void toSrgbUnclamped(TimingContext *tc = 0);
		/// @brief Applies the sRGB-to-linear transfer function to channels 0...2.
		///
		/// This transfer function replaces each value `x` with
		/// ```text
		/// if x < 0.0f, 0.0f
		/// if x < 0.04045f, x / 12.92f
		/// if x < 1.0f, powf((x + 0.055f)/1.055f, 2.4f)
		/// otherwise, 1.0f
		/// ```
		///
		/// @see toSrgb()
		NVTT_API void toLinearFromSrgb(TimingContext *tc = 0);
		/// @brief Applies the sRGB-to-linear transfer function to channels 0...2, but does not clamp output to [0,1].
		///
		/// The motivation for this function is that it can approximately
		/// preserve HDR values through sRGB conversion and back; that is,
		/// s.toSrgbUnclamped().toLinearFromSrgbUnclamped() is close to s.
		///
		/// This transfer function replaces each value `x` with
		/// ```text
		/// if x is NaN or x <= 0.0f, x
		/// if x < 0.04045f, x / 12.92f
		/// otherwise, powf((x + 0.055f)/1.055f, 2.4f)
		/// ```
		///
		/// @since NVTT 3.2
		/// @see toSrgbUnclamped()
		NVTT_API void toLinearFromSrgbUnclamped(TimingContext *tc = 0);
		/// @brief Converts colors in channels 0...2 from linear to a
		/// piecewise linear sRGB approximation.
		/// 
		/// This transfer function replaces each value `x` with
		/// ```text
		/// if x < 0,    0.0f
		/// if x < 1/16, 4.0f * x
		/// if x < 1/8,  2.0f * x + 0.125f
		/// if x < 1/2,         x + 0.25f
		/// if x < 1,    0.5f * x + 0.5f
		/// otherwise, 1.0f
		/// ```
		/// 
		/// @see Alex Vlachos, Post Processing in The Orange Box, GDC 2008
		/// @see toLinearFromXenonSrgb()
		NVTT_API void toXenonSrgb(TimingContext *tc = 0);
		/// @brief Converts colors in channels 0...2 from the Xenon sRGB
		/// piecewise linear sRGB approximation to linear.
		/// 
		/// This transfer function replaces each value `x` with
		/// ```text
		/// if x < 0,    0.0f
		/// if x < 1/4,  x/4.0f
		/// if x < 3/8, (x - 0.125f) / 2.0f
		/// if x < 3/4,  x - 0.25f
		/// if x < 1,   (x - 0.5f) / 0.5f
		/// otherwise, 1.0f
		/// ```
		/// 
		/// @since NVTT 3.2
		/// @see Alex Vlachos, Post Processing in The Orange Box, GDC 2008
		/// @see toXenonSrgb()
		NVTT_API void toLinearFromXenonSrgb(TimingContext *tc = 0);
		/// @brief Applies a 4x4 affine transformation to the values in channels 0...3.
		///
		/// `w0`...`w3` are the columns of the matrix. `offset` is added after
		/// the matrix-vector multiplication.
		/// 
		/// In other words, all (r, g, b, a) values are replaced with
		/// ```text
		/// (r)   (w0[0], w1[0], w2[0], w3[0]) (r)   (offset[0])
		/// (g) = (w0[1], w1[1], w2[1], w3[1]) (g) + (offset[1])
		/// (b)   (w0[2], w1[2], w2[2], w3[2]) (b)   (offset[2])
		/// (a)   (w0[3], w1[3], w2[3], w3[3]) (a)   (offset[3])
		/// ```
		NVTT_API void transform(const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4], TimingContext *tc = 0);
		/// @brief Swizzles the channels of the surface.
		///
		/// Each argument specifies where the corresponding channel should come
		/// from. For instance, setting r to 2 would mean that the red (0)
		/// channel would be set to the current 2nd channel.
		/// 
		/// In addition, the special values 4, 5, and 6 represent setting the
		/// channel to a constant value of 1.0f, 0.0f, or -1.0f, respectively.
		NVTT_API void swizzle(int r, int g, int b, int a, TimingContext *tc = 0);
		/// @brief Applies a scale and bias to the given channel. Each value x is replaced by x * scale + bias.
		NVTT_API void scaleBias(int channel, float scale, float bias, TimingContext *tc = 0);
		/// @brief Clamps all values in the channel to the range [low, high].
		NVTT_API void clamp(int channel, float low = 0.0f, float high = 1.0f, TimingContext *tc = 0);
		/// @brief Interpolates all texels between their current color and a constant color `(r, g, b, a)`.
		///
		/// `t` is the value used for linearly interpolating between the
		/// surface's current colors and the constant color. For instance, a
		/// value of `t=0` has no effect to the surface's colors, and a value
		/// of `t=1` replaces the surface's colors entirely with (r, g, b, a).
		NVTT_API void blend(float r, float g, float b, float a, float t, TimingContext *tc = 0);
		/// @brief Converts to premultiplied alpha, replacing `(r, g, b, a)` with `(ar, ag, ab, a)`.
		NVTT_API void premultiplyAlpha(TimingContext *tc = 0);
		/// @brief Converts from premultiplied to unpremultiplied alpha, with special handling around zero alpha values.
		///
		/// When `abs(a) >= epsilon`, the result is the same as dividing the
		/// RGB channels by the alpha channel. Otherwise, this function divides
		/// the RGB channels by `epsilon * sign(a)`, since the result of
		/// unpremultiplying a fully transparent color is undefined.
		NVTT_API void demultiplyAlpha(float epsilon = 1e-12f, TimingContext *tc = 0);
		/// @brief Sets channels 0...3 to the result of converting to grayscale, with customizable channel weights.
		///
		/// For instance, this can be used to give green a higher weight than
		/// red or blue when computing luminance. This function will normalize
		/// the different scales so they sum to 1, so e.g. (2, 4, 1, 0) are
		/// valid scales. The greyscale value is then computed using
		/// ```c
		/// grey = r * redScale + g * greenScale + b * blueScale + a * alphaScale
		/// ```
		/// and then all channels (including alpha) are set to `grey`.
		NVTT_API void toGreyScale(float redScale, float greenScale, float blueScale, float alphaScale, TimingContext *tc = 0);
		/// @brief Sets all texels on the border of the surface to a solid color.
		NVTT_API void setBorder(float r, float g, float b, float a, TimingContext *tc = 0);
		/// @brief Sets all texels in the surface to a solid color.
		NVTT_API void fill(float r, float g, float b, float a, TimingContext *tc = 0);
		/// @brief Attempts to scale the alpha channel so that a fraction
		/// `coverage` (between 0 and 1) of the surface has an alpha greater
		/// than `alphaRef`.
		/// 
		/// @see alphaTestCoverage() for the method used to determine what fraction passes the alpha test.
		/// @see Ignacio Casta&ntilde;o, "Computing Alpha Mipmaps" (2010)
		NVTT_API void scaleAlphaToCoverage(float coverage, float alphaRef = 0.5f, int alpha_channel = 3, TimingContext *tc = 0);
		/// @brief Produces an LDR Red, Green, Blue, Magnitude encoding of the HDR RGB channels.
		///
		/// See fromRGBM() for the storage method. This uses an iterative
		/// compression approach to reduce the error with regard to decoding. 
		NVTT_API void toRGBM(float range = 1.0f, float threshold = 0.25f, TimingContext *tc = 0);
		/// @brief Produces HDR `(r, g, b, 1)` values from an LDR `(red, green, blue, magnitude)` storage method.
		///
		/// HDR values are reconstructed as follows: First, the magnitude `M`
		/// is reconstructed from the alpha channel using
		/// `M = a * (range - threshold) + threshold`. Then the red, green,
		/// and blue channels are multiplied by `M`.
		NVTT_API void fromRGBM(float range = 1.0f, float threshold = 0.25f, TimingContext *tc = 0);
		/// @brief Stores luminance-only values in a two-channel way. Maybe consider BC4 compression instead.
		///
		/// Luminance `L` is computed by averaging the red, green, and blue
		/// values, while `M` stores the max of these values and `threshold`.
		/// The red, green, and blue channels then store `L/M`, and the alpha
		/// channel stores `(M - threshold)/(1 - threshold)`. 
		NVTT_API void toLM(float range = 1.0f, float threshold = 0.0f, TimingContext *tc = 0);
		/// @brief Produces a shared-exponent Red, Green, Blue, Exponent encoding of the HDR RGB channels, such as R9G9B9E5.
		/// 
		/// `mantissaBits` and `exponentBits` must be in the range 1...31.
		///
		/// See fromRGBE() for the storage method. This uses an iterative
		/// compression approach to reduce the error with regard to decoding.
		NVTT_API void toRGBE(int mantissaBits, int exponentBits, TimingContext *tc = 0);
		/// @brief Produces HDR `(r, g, b, 1)` values from an LDR `(red, green, blue, exponent)` storage method.
		///
		/// HDR values are reconstructed as follows:
		/// R, G, B, and E are first converted from UNORM floats to integers by
		/// multiplying RGB by `(1 << mantissaBits) - 1` and
		/// E by `(1 << exponentBits) - 1`. E stores a scaling factor as a
		/// power of 2, which is reconstructed using
		/// `scale = 2^(E - ((1 << (exponentBits - 1)) - 1) - mantissaBits)`.
		/// R, G, and B are then multiplied by `scale`.
		/// 
		/// `mantissaBits` and `exponentBits` must be in the range 1...31.
		NVTT_API void fromRGBE(int mantissaBits, int exponentBits, TimingContext *tc = 0);
		/// @brief Converts from `(r, g, b, -)` colors to `(Co, Cg, 1, Y)` colors.
		///
		/// This is useful for formats that use chroma subsampling.
		/// 
		/// Y is in the range [0, 1], while Co and Cg are in the range [-1, 1].
		/// 
		/// The RGB-to-YCoCg formula used is
		/// ```c
		/// Y  = (2g + r + b)/4
		/// Co = r - b
		/// Cg = (2g - r - b)/2
		/// ```
		NVTT_API void toYCoCg(TimingContext *tc = 0);
		/// @brief Stores per-block YCoCg scaling information for potentially better 4-channel compression of YCoCg data.
		///
		/// For each 4x4 block, this computes the maximum absolute Co and Cg
		/// values, stores the result in the blue channel, and multiplies the
		/// Co and Cg channels (0 and 1) by its reciprocal. The original Co
		/// and Cg values can then be reconstructed by multiplying by the
		/// blue channel.
		/// 
		/// The scaling information is quantized to the given number of bits.
		/// 
		/// `threshold` is ignored.
		/// 
		/// @note This assumes that your texture compression format uses 4x4
		/// blocks. This is true for all BC1-BC7 formats, but ASTC can use
		/// other block sizes.
		NVTT_API void blockScaleCoCg(int bits = 5, float threshold = 0.0f, TimingContext *tc = 0);
		/// @brief Converts from `(Co, Cg, scale, Y)` colors to `(r, g, b, 1)` colors.
		///
		/// This is useful for formats that use chroma subsampling.
		/// 
		/// Y is in the range [0, 1], while Co and Cg are in the range [-1, 1].
		/// Co and Cg are multiplied by channel 2 (scale) to reverse the
		/// effects of optionally calling blockScaleCoCg().
		/// 
		/// The YCoCg-to-RGB formula used is
		/// ```c
		/// r = Y + Co - Cg
		/// g = Y + Cg
		/// b = Y - Co - Cg
		/// ```
		NVTT_API void fromYCoCg(TimingContext *tc = 0);
		/// @brief Converts from RGB colors to a (U, V, W, L) color space, much like RGBM.
		///
		/// All values are clamped to [0, 1]. Then a luminance-like value
		/// `L` is computed from RGB using
		/// 
		/// `L = max(sqrtf(R^2 + G^2 + B^2), 1e-6f)`.
		/// 
		/// This then stores the value `(R/L, G/L, B/L, L/sqrt(3))`.
		NVTT_API void toLUVW(float range = 1.0f, TimingContext *tc = 0);
		/// @brief Converts from toLUVW()'s color space to RGB colors.
		///
		/// This is the same as fromRGBM(range * sqrt(3)).
		NVTT_API void fromLUVW(float range = 1.0f, TimingContext *tc = 0);
		/// @brief Replaces all colors by their absolute value.
		NVTT_API void abs(int channel, TimingContext *tc = 0);
		/// @brief Convolves a channel with a kernel.
		///
		/// This uses a 2D kernelSize x kernelSize kernel, with values in
		/// `kernelData` specified in row-major order. The behavior around
		/// image borders is determined by the image's wrap mode.
		NVTT_API void convolve(int channel, int kernelSize, float * kernelData, TimingContext *tc = 0);
		/// @brief Replaces all values with their log with the given base.
		NVTT_API void toLogScale(int channel, float base, TimingContext *tc = 0);
		/// @brief Inverts toLogScale() by replacing all values `x` with `base^x`.
		NVTT_API void fromLogScale(int channel, float base, TimingContext *tc = 0);
		/// @brief Draws borders of a given color around each w x h tile
		/// contained within the surface, starting from the (0, 0) corner.
		/// 
		/// In case the surface size is not divisible by the tile size, borders
		/// are not drawn for tiles crossing the surface boundary.
		NVTT_API void setAtlasBorder(int w, int h, float r, float g, float b, float a, TimingContext *tc = 0);
		/// @brief Applies an HDR-to-LDR tone mapper.
		/// @see #ToneMapper for definitions of the tone mappers.
		NVTT_API void toneMap(ToneMapper tm, float * parameters, TimingContext *tc = 0);

		/// @brief Sets values in the given channel to either 1 or 0 depending on if they're greater than the threshold, with optional dithering.
		/// @param dither If true, uses Floyd-Steinberg dithering on the CPU. Not supported for 3D surfaces.
		NVTT_API void binarize(int channel, float threshold, bool dither, TimingContext *tc = 0);
		/// @brief Quantizes this channel to a particular number of bits, with optional dithering.
		/// @param channel The index of the channel to quantize.
		/// @param bits The number of bits to quantize to, yielding `2^bits` possible values.
		/// Must be nonnegative, and must not be 0 if exactEndPoints is true.
		/// @param exactEndPoints If true, the set of quantized values will be
		/// `0, 1/(2^bits-1), ..., 1`, rather than `0, 1/(2^bits), ..., (2^bits-1)/(2^bits)`.
		/// @param dither If true, uses Floyd-Steinberg dithering on the CPU. Not supported for 3D surfaces.
		NVTT_API void quantize(int channel, int bits, bool exactEndPoints, bool dither, TimingContext *tc = 0);

		/// @brief Sets the RGB channels to a normal map generated by interpreting the alpha channel as a heightmap,
		/// using a blend of four small-scale to large-scale Sobel kernels.
		/// 
		/// This uses a 9x9 kernel which is a weighted sum of a 3x3 (small),
		/// 5x5 (medium), 7x7 (big), and 9x9 (large) differentiation kernels.
		/// Each of the weights can be greater than 1, or even negative.
		/// However, the kernel will be normalized so that its elements sum to
		/// 1, so scaling should be done on the alpha channel beforehand.
		/// The smallest kernel focuses on the highest-frequency details, and
		/// larger kernels attenuate higher frequencies.
		/// 
		/// The source alpha channel, which is used as a height map to
		/// differentiate, is copied to the output.
		/// 
		/// The output RGB channels will be in the ranges [-1, 1], [-1, 1], and [0, 1].
		/// 
		/// @see convolve()
		NVTT_API void toNormalMap(float sm, float medium, float big, float large, TimingContext *tc = 0);
		/// @brief Renormalizes the elements of a signed normal map, replacing `(r, g, b)` with `normalize(r, g, b)`.
		///
		/// This function is safe to call even for zero vectors.
		NVTT_API void normalizeNormalMap(TimingContext *tc = 0);
		/// @brief Applies a 3D->2D normal transformation, setting the z (blue) channel to 0.
		///
		/// @see #NormalTransform for definitions of each of the normal transformations.
		NVTT_API void transformNormals(NormalTransform xform, TimingContext *tc = 0);
		/// @brief Reconstructs 3D normals from 2D transformed normals.
		///
		/// @see #NormalTransform for definitions of each of the normal transformations.
		NVTT_API void reconstructNormals(NormalTransform xform, TimingContext *tc = 0);
		/// @brief Sets the z (blue) channel to x^2 + y^2.
		///
		/// If the x and y channels represent slopes, instead of normals, then
		/// this represents a CLEAN map. The important thing about this is that
		/// it can be mipmapped, and the difference between the sum of the 
		/// square of the first and second mipmapped channels and the third
		/// mipmapped channel can be used to determine how rough the normal
		/// map is in a given area.
		/// 
		/// This is a lower-memory and lower-bandwidth version of LEAN mapping,
		/// but it has the drawback that it can only represent
		/// isotropic roughness.
		/// 
		/// @see Olano and Baker, "LEAN Mapping", https://www.csee.umbc.edu/~olano/papers/lean/
		/// @see Hill, "Specular Showdown in the Wild West", http://blog.selfshadow.com/2011/07/22/specular-showdown/
		/// @see http://gaim.umbc.edu/2011/07/24/shiny-and-clean/
		/// @see http://gaim.umbc.edu/2011/07/26/on-error/
		NVTT_API void toCleanNormalMap(TimingContext *tc = 0);
		/// @brief Packs signed normals in [-1, 1] to an unsigned range [0, 1], using (r, g, b, a) |-> (r/2+1/2, g/2+1/2, b/2+1/2, a).
		NVTT_API void packNormals(float scale = 0.5f, float bias = 0.5f, TimingContext *tc = 0);
		/// @brief Expands packed normals in [0, 1] to signed normals in [-1, 1] using (r, g, b, a) |-> (2r-1, 2g-1, 2b-1, a).
		NVTT_API void expandNormals(float scale = 2.0f, float bias = -1.0f, TimingContext *tc = 0);
		/// @brief Unimplemented. This would equivalent to mipmapping a normal map and then measuring how much
		/// mipmapping shortens the normals. Currently returns a null surface.
		/// @see Hill, "Specular Showdown in the Wild West", http://blog.selfshadow.com/2011/07/22/specular-showdown/
		NVTT_API Surface createToksvigMap(float power, TimingContext *tc = 0) const;
		/// @brief Unimplemented. Currently returns a null surface.
		/// @see toCleanNormalMap()
		NVTT_API Surface createCleanMap(TimingContext *tc = 0) const;

		/// @brief Flips the surface along the X axis.
		NVTT_API void flipX(TimingContext *tc = 0);
		/// @brief Flips the surface along the Y axis.
		NVTT_API void flipY(TimingContext *tc = 0);
		/// @brief Flips the surface along the Z axis.
		NVTT_API void flipZ(TimingContext *tc = 0);
		/// @brief Creates a new surface from the range of pixels from x = x0 to x1, y = y0 to y1, and z = z0 to z1.
		/// 
		/// If any of the parameters are out of bounds or creation fails, returns a null surface.
		/// 
		/// A valid surface created will have size (x1 - x0 + 1) x (y1 - y0 + 1) x (z1 - z0 + 1).
		NVTT_API Surface createSubImage(int x0, int x1, int y0, int y1, int z0, int z1, TimingContext *tc = 0) const;

		/// @brief Copies channel `srcChannel` from `srcImage` to `srcChannel` of this surface.
		///
		/// Returns whether the operation succeeded (for instance, it can fail
		/// if the surfaces have different sizes).
		/// 
		/// `srcChannel` must be in the range [0, 3].
		/// 
		/// The surfaces need not have the same GPU mode.
		NVTT_API bool copyChannel(const Surface & srcImage, int srcChannel, TimingContext *tc = 0);
		/// @brief Copies channel `srcChannel` from `srcImage` to `dstChannel` of this surface.
		///
		/// Returns whether the operation succeeded (for instance, it can fail
		/// if the surfaces have different sizes).
		/// 
		/// Both `srcChannel` and `dstChannel` must be in the range [0, 3].
		/// 
		/// The surfaces need not have the same GPU mode.
		NVTT_API bool copyChannel(const Surface & srcImage, int srcChannel, int dstChannel, TimingContext *tc = 0);
		/// @brief Add channel `srcChannel` of `img`, multiplied by `scale`, to `dstChannel` of this surface.
		///
		/// Returns whether the operation succeeded (for instance, it can fail
		/// if the surfaces have different sizes).
		/// 
		/// Both `srcChannel` and `dstChannel` must be in the range [0, 3].
		/// 
		/// The surfaces need not have the same GPU mode.
		NVTT_API bool addChannel(const Surface & img, int srcChannel, int dstChannel, float scale, TimingContext *tc = 0);
		/// @brief Copies all channels of a rectangle from `src` to this surface.
		/// 
		/// More specifically, this copies the rectangle
		/// 
		/// `[xsrc, xsrc + xsize - 1] x [ysrc, ysrc + ysize - 1] x [zsrc, zsrc + zsize - 1]`
		/// 
		/// to the rectangle
		/// 
		/// `[xdst, xdst + xsize - 1] x [ydst, ydst + ysize - 1] x [zdst, zdst + zsize - 1]`.
		///
		/// Returns whether the operation succeeded (for instance, invalid
		/// parameters return false).
		NVTT_API bool copy(const Surface & src, int xsrc, int ysrc, int zsrc, int xsize, int ysize, int zsize, int xdst, int ydst, int zdst, TimingContext *tc = 0);

		/// @brief Makes succeeding operations work on the GPU buffer.
		/// 
		/// This also copies the surface's CPU data to a new or recreated GPU buffer.
		/// 
		/// @since NVTT 3.0
		NVTT_API void ToGPU(TimingContext *tc = 0, bool performCopy = true);
		/// @brief Makes succeeding operations work on the CPU buffer.
		/// 
		/// This copies the surface's GPU buffer to the CPU buffer, then destroys the GPU buffer.
		/// 
		/// @since NVTT 3.0
		NVTT_API void ToCPU(TimingContext *tc = 0);
		/// @brief Get a CUDA pointer to const image data on the GPU, using the same layout as data().
		/// If GPU data does not exist, returns `nullptr`.
		NVTT_API const float * gpuData() const;
		/// @brief Get a CUDA pointer to non-const image data on the GPU, using the same layout as data().
		/// If GPU data does not exist, returns `nullptr`.
		/// This can be used to modify NVTT Surface data on the GPU, outside of
		/// the functions NVTT provides.
		/// @since NVTT 3.2
		NVTT_API float * gpuDataMutable();

		//private:
		void detach();

		struct Private;
		Private * m;
	};

	/// @brief Surface-set struct for convenience of handling multi-level texture files such as DDS, currently only supporting reading.
	/// @since NVTT 3.0	
	struct SurfaceSet
	{
		NVTT_FORBID_COPY(SurfaceSet)
		NVTT_DECLARE_PIMPL(SurfaceSet)

		/// Constructor
		NVTT_API SurfaceSet();
		/// Destructor
		NVTT_API ~SurfaceSet();

		/// Texture type: 2D, 3D, or Cube
		NVTT_API TextureType GetTextureType();
		/// Number of faces
		NVTT_API int GetFaceCount();
		/// Number of mip-map levels
		NVTT_API int GetMipmapCount();
		/// Image width (level 0)
		NVTT_API int GetWidth();
		/// Image height (level 0)
		NVTT_API int GetHeight();
		/// Image depth (level 0)
		NVTT_API int GetDepth();

		/// Get a surface at specified face and mip level
		NVTT_API Surface GetSurface(int faceId, int mipId, bool expectSigned = false);

		/// Get a surface at specified face and mip level
		NVTT_API void GetSurface(int faceId, int mipId, Surface& surface, bool expectSigned = false);

		/// Release data
		NVTT_API void reset();

		/// @brief Load a surface set from a DDS file.
		/// @returns `true` if loading succeeded and `false` if it failed.
		///
		/// @param fileName Path to the file to load. This uses the same
		/// character encoding as the `filename` argument of `fopen()`.
		/// @param forcenormal If true, builds a normal map from the red and
		/// green components of BC5U data, or the alpha and green components of
		/// BC3 data.
		NVTT_API bool loadDDS(const char * fileName, bool forcenormal = false);


		/// @brief Load a surface set from an in-memory DDS file.
		/// @returns `true` if loading succeeded and `false` if it failed.
		///
		/// @param data Pointer to the start of the file's data in memory.
		/// @param sizeInBytes Length of the file's data.
		/// @param forcenormal If true, builds a normal map from the red and
		/// green components of BC5U data, or the alpha and green components of
		/// BC3 data.
		NVTT_API bool loadDDSFromMemory(const void * data, unsigned long long sizeInBytes, bool forcenormal = false);

		/// Save an image at specified face and mip level (for decompression)
		NVTT_API bool saveImage(const char* fileName, int faceId, int mipId);

	};

	/// @brief Specifies how to fold or unfold a cube map from or to a 2D texture.
	/// @since NVTT 2.1
	enum CubeLayout {
		/// Unfolds into a 3*edgeLength (width) x 4*edgeLength texture, laid
		/// out as follows:
		/// ```text
		///  2
		/// 140
		///  3
		///  5
		/// ```
		/// Face 5 is rotated 180 degrees.
		CubeLayout_VerticalCross,
		/// Unfolds into a 4*edgeLength (width) x 3*edgeLength texture, laid
		/// out as follows:
		/// ```text
		///  2
		/// 1405
		///  3
		/// ```
		/// Face 5 is rotated 180 degrees.
		CubeLayout_HorizontalCross,
		/// Writes each face in order into a column layout, like this:
		/// ```text
		/// 0
		/// 1
		/// 2
		/// 3
		/// 4
		/// 5
		/// ```
		CubeLayout_Column,
		/// Writes each face in order into a row layout, like this:
		/// ```text
		/// 012345
		/// ```
		CubeLayout_Row,
		/// Same as CubeLayout_VerticalCross.
		CubeLayout_LatitudeLongitude
	};

	/// @brief Use EdgeFixup_None if unsure; this affects how certain cube surface processing algorithms work.
	enum EdgeFixup {
		EdgeFixup_None, ///< No effect.
		EdgeFixup_Stretch, ///< Slightly stretches and shifts the coordinate systems cosinePowerFilter() and fastResample() use.
		EdgeFixup_Warp, ///< Applies a cubic warp to each face's coordinate system in cosinePowerFilter() and fastResample(), warping texels closer to edges more.
		EdgeFixup_Average, ///< Currently unimplemented.
	};

	/// @brief A CubeSurface is one level of a cube map texture.
	/// 
	/// CubeSurfaces are either null, or contain six square Surfaces numbered
	/// 0 through 5, all with the same size (referred to as the edge length).
	/// By convention, these are the +x, -x, +y, -y, +z, and -z faces, in that
	/// order, of a cube map in a right-handed coordinate system.
	/// 
	/// These objects are reference-counted like nvtt::Surface.
	/// 
	/// @since NVTT 2.1
	struct CubeSurface
	{
		/// Creates a null CubeSurface.
		NVTT_API CubeSurface();
		/// Copy constructor.
		NVTT_API CubeSurface(const CubeSurface & img);
		/// Destructor.
		NVTT_API ~CubeSurface();
		/// Assignment operator.
		NVTT_API void operator=(const CubeSurface & img);

		/// @brief Returns if this CubeSurface is null (i.e. has no underlying
		/// data, or all faces have size 0x0).
		NVTT_API bool isNull() const;
		/// Returns the edge length of any of the faces.
		NVTT_API int edgeLength() const;
		/// @brief Returns the number of mips that would be in a full mipmap
		/// chain starting with this CubeSurface.
		/// 
		/// For instance, a full mip chain for a cube map with 10x10 faces
		/// would consist of cube maps with sizes 10x10, 5x5, 2x2, and 1x1, and
		/// this function would return 4.
		NVTT_API int countMipmaps() const;

		/// @brief Load a cube map from a DDS file.
		///
		/// @param fileName Path to the file to load. This uses the same
		/// character encoding as the `filename` argument of `fopen()`.
		/// @param mipmap The mip to read. If `mipmap` is negative, then this
		/// reads the `abs(mipmap)`'th smallest mipmap.
		/// 
		/// If the DDS file has multiple array elements, this always loads the first one.
		/// 
		/// Returns whether the operation succeeded (for instance, this can
		/// fail if the file does not exist or if the mipmap was out of range).
		NVTT_API bool load(const char * fileName, int mipmap);
		/// @brief Load a cube map from a DDS file in memory.
		///
		/// @param data Pointer to the start of the file's data in memory.
		/// @param sizeInBytes Length of the file's data.
		/// @param mipmap The mip to read. If `mipmap` is negative, then this
		/// reads the `abs(mipmap)`'th smallest mipmap.
		///
		/// If the DDS file has multiple array elements, this always loads the first one.
		///
		/// Returns whether the operation succeeded (for instance, this can
		/// fail if the file does not exist or if the mipmap was out of range).
		NVTT_API bool loadFromMemory(const void * data, unsigned long long sizeInBytes, int mipmap);
		/// @brief Save a cube map to a DDS file in memory.
		/// 
		/// Unimplemented - iterate over faces and save them instead.
		NVTT_API bool save(const char * fileName) const;

		/// @brief Get a non-const reference to the surface for the given face.
		///
		/// `face` must be in the range [0, 5].
		NVTT_API Surface & face(int face);
		/// @brief Get a const reference to the surface for the given face.
		/// 
		/// `face` must be in the range [0, 5].
		NVTT_API const Surface & face(int face) const;

		/// @brief Sets this CubeSurface from a 2D unfolded surface in `img`.
		///
		/// @see #CubeLayout for allowed folding layouts.
		NVTT_API void fold(const Surface & img, CubeLayout layout);
		/// @brief Creates a Surface containing an unfolded/flattened representation of the cube surface.
		/// 
		/// @see #CubeLayout for allowed folding layouts.
		NVTT_API Surface unfold(CubeLayout layout) const;

		// @@ Angular extent filtering.

		// @@ Add resizing methods.

		// @@ Add edge fixup methods.

		/// @brief Computes an average value for the given channel over the entire sphere.
		/// 
		/// This takes solid angles into account when producing an average per
		/// steradian, so texels near face edges are weighted less than texels
		/// near face edges.
		/// 
		/// No gamma correction is performed, unlike nvtt::Surface::average().
		NVTT_API float average(int channel) const;
		/// @brief Sets `*minimum_ptr` and `*maximum_ptr` to the minimum and
		/// maximum values in the given channel.
		/// 
		/// If all faces have size 0 x 0 (in which case the cube surface isNull()),
		/// this will set `*minimum_ptr` to `FLT_MAX` and `*maximum_ptr` to 0.0f.
		NVTT_API void range(int channel, float * minimum_ptr, float * maximum_ptr) const;
		/// Clamps values in the given channel to the range [low, high].
		NVTT_API void clamp(int channel, float low = 0.0f, float high = 1.0f);


		/// Unimplemented; returns a null CubeSurface.
		NVTT_API CubeSurface irradianceFilter(int size, EdgeFixup fixupMethod) const;
		/// @brief Spherically convolves this CubeSurface with a
		/// `max(0.0f, cos(theta))^cosinePower` kernel, returning a CubeSurface
		/// with faces with dimension size x size.
		///
		/// This is useful for generating prefiltered cube maps, as this
		/// corresponds to the cosine power used in the Phong reflection model
		/// (with energy conservation).
		/// 
		/// This handles how each cube map texel can have a different solid angle.
		/// It also only considers texels for which the value of the kernel
		/// (without normalization) is at least 0.001.
		NVTT_API CubeSurface cosinePowerFilter(int size, float cosinePower, EdgeFixup fixupMethod) const;
		/// Produces a resized version of this CubeSurface using nearest-neighbor sampling.
		NVTT_API CubeSurface fastResample(int size, EdgeFixup fixupMethod) const;

		/// @brief Raises channels 0...2 to the power `gamma`.
		/// 
		/// Using a `gamma` of 2.2 approximates sRGB-to-linear conversion.
		///
		/// @see nvtt::Surface::toLinear()
		NVTT_API void toLinear(float gamma);
		/// @brief Raises channels 0...2 to the power `1/gamma`.
		/// 
		/// Using a `gamma` of 2.2 approximates linear-to-sRGB conversion.
		///
		/// @see nvtt::Surface::toGamma()
		NVTT_API void toGamma(float gamma);

		//private:
		void detach();

		struct Private;
		Private * m;
	};

	/// @brief Structure defining a list of inputs to be compressed.
	/// 
	/// Inputs will be combined for parallel GPU processing.
	/// Better performance expected comparing to the Surface-only API, which compresses each image one by one, especially when the images are small.
	/// 
	/// @since NVTT 3.0
	struct BatchList
	{
		NVTT_FORBID_COPY(BatchList)
		NVTT_DECLARE_PIMPL(BatchList)

		/// Creates an empty BatchList.
		NVTT_API BatchList();
		/// Destructor.
		NVTT_API ~BatchList();
		/// Clears the list of inputs in this BatchList.
		NVTT_API void Clear();
		/// Adds a pointer to the surface, its face and mipmap index, and a pointer to the output method to the input list.
		NVTT_API void Append(const Surface* pImg, int face, int mipmap, const OutputOptions* outputOptions);

		/// Returns the size of the input list.
		NVTT_API unsigned GetSize() const;
		/// Gets the `i`th item in the input list.
		NVTT_API void GetItem(unsigned i, const Surface*& pImg, int& face, int& mipmap, const OutputOptions*& outputOptions) const;

	};


	/// Image comparison and error measurement functions. (New in NVTT 2.1)
	NVTT_API float rmsError(const Surface & reference, const Surface & img, TimingContext *tc = 0);
	/// Image comparison and error measurement functions. (New in NVTT 2.1)
	NVTT_API float rmsAlphaError(const Surface & reference, const Surface & img, TimingContext *tc = 0);
	/// Image comparison and error measurement functions. (New in NVTT 2.1)
	NVTT_API float cieLabError(const Surface & reference, const Surface & img, TimingContext *tc = 0);
	/// Image comparison and error measurement functions. (New in NVTT 2.1)
	NVTT_API float angularError(const Surface & reference, const Surface & img, TimingContext *tc = 0);
	/// Image comparison and error measurement functions. (New in NVTT 2.1)
	NVTT_API Surface diff(const Surface & reference, const Surface & img, float scale, TimingContext *tc = 0);

	/// Image comparison and error measurement functions. (New in NVTT 2.1)
	NVTT_API float rmsToneMappedError(const Surface & reference, const Surface & img, float exposure, TimingContext *tc = 0);

	/// Generate histogram from surface
	NVTT_API Surface histogram(const Surface & img, int width, int height, TimingContext *tc = 0);

	/// Generate histogram from surface
	NVTT_API Surface histogram(const Surface & img, float minRange, float maxRange, int width, int height, TimingContext *tc = 0);

	/// Geting the target extent for round-mode and texture-type. (New in NVTT 3.0)
	NVTT_API void getTargetExtent(int * width, int * height, int * depth, int maxExtent, RoundMode roundMode, TextureType textureType, TimingContext *tc = 0);

	/// Calculate the count of mipmaps given width, height, depth. (New in NVTT 3.0)
	NVTT_API int countMipmaps(int w, int h, int d, TimingContext *tc = 0);

	/// @brief A TimingContext is a way to collect timing data from a number of
	/// functions, and report how much time each function took.
	/// 
	/// Since NVTT 3.0, many functions will take an optional TimingContext
	/// pointer. If the `detailLevel` of the TimingContext is high enough,
	/// the function will record the function name and how much CPU time the
	/// function took, synchronizing with the CPU if necessary. One can then
	/// get individual records using GetRecord(), or print all statistics
	/// using PrintRecords().
	/// 
	/// @since NVTT 3.0
	struct TimingContext
	{
		/// @brief Creates a TimingContext with the given `detailLevel`.
		/// 
		/// Functions will only collect timing data if their detail level
		/// (usually 2 or 3) is less than or equal to the TimingContext's
		/// `detailLevel`.
		NVTT_API TimingContext(int detailLevel = 1);
		/// Destructor.
		NVTT_API ~TimingContext();

		/// @brief Sets this TimingContext's detail level.
		/// 
		/// Functions will only collect timing data if their detail level
		/// (usually 2 or 3) is less than or equal to the TimingContext's
		/// `detailLevel`.
		NVTT_API void SetDetailLevel(int detailLevel);

		/// Returns the number of timing records stored.
		NVTT_API int GetRecordCount();
		/// @brief Returns the description and length in seconds of the `i`th record.
		/// 
		/// @deprecated GetRecord(int, char*, double&) doesn't check the length
		/// of its description argument before copying an arbitrary-length
		/// string into it. Please use GetRecord(int, char*, size_t, double&)
		/// instead.
		/// 
		/// The description is strcopied directly into `description`, which must be long enough.
		NVTT_DEPRECATED_API void GetRecord(int i, char* description, double& seconds);
		/// @brief Returns the description and length in seconds of the `i`th record.
		///
		/// `outDescriptionSize` is the size in bytes of the buffer pointed to
		/// by `outDescription`. GetRecord() truncates (if necessary) the
		/// record's description to `outDescriptionSize-1` bytes, and always
		/// writes a null terminator.
		///
		/// Returns the number of bytes written,
		/// not including the null terminator.
		/// If `outDescription` is null, returns the number of bytes in the
		/// record's description.
		/// If `i` is out of bounds, returns 0.
		NVTT_API size_t GetRecord(int i, char* outDescription, size_t outDescriptionSize, double& outSeconds);

		/// @brief Prints all records including their levels of nesting.
		/// 
		/// NVTT keeps track of how deeply timers are nested, so this will
		/// print out each description and length with an indentation
		/// corresponding to its nesting depth.
		NVTT_API void PrintRecords();

		struct Private;
		Private * m;
	};
}


#endif
