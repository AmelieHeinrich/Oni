// Copyright (c) 2021 NVIDIA Corporation <texturetools@nvidia.com>
// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
// Copyright (c) 2007-2009 NVIDIA Corporation -- Ignacio Castano <icastano@nvidia.com>

#ifndef NVTT_WRAPPER_H
#define NVTT_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

// Function linkage
#if NVTT_SHARED

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__ || defined __MINGW32__
#	ifdef NVTT_EXPORTS
#		define NVTT_API __declspec(dllexport)
#	else
#		define NVTT_API __declspec(dllimport)
#	endif
#endif

#if defined __GNUC__ >= 4
#	ifdef NVTT_EXPORTS
#		define NVTT_API __attribute__((visibility("default")))
#	endif
#endif

#endif // NVTT_SHARED

#if !defined NVTT_API
#	define NVTT_API
#endif

#ifdef __cplusplus
typedef struct nvtt::CPUInputBuffer NvttCPUInputBuffer;
typedef struct nvtt::GPUInputBuffer NvttGPUInputBuffer;
typedef struct nvtt::CompressionOptions NvttCompressionOptions;
typedef struct nvtt::OutputOptions NvttOutputOptions;
typedef struct nvtt::Context NvttContext;
typedef struct nvtt::Surface NvttSurface;
typedef struct nvtt::SurfaceSet NvttSurfaceSet;
typedef struct nvtt::CubeSurface NvttCubeSurface;
typedef struct nvtt::BatchList NvttBatchList;
typedef struct nvtt::TimingContext NvttTimingContext;
#else
typedef struct NvttCPUInputBuffer NvttCPUInputBuffer;
typedef struct NvttGPUInputBuffer NvttGPUInputBuffer;
typedef struct NvttCompressionOptions NvttCompressionOptions;
typedef struct NvttOutputOptions NvttOutputOptions;
typedef struct NvttContext NvttContext;
typedef struct NvttSurface NvttSurface;
typedef struct NvttSurfaceSet NvttSurfaceSet;
typedef struct NvttCubeSurface NvttCubeSurface;
typedef struct NvttBatchList NvttBatchList;
typedef struct NvttTimingContext NvttTimingContext;
#endif

#ifndef NVTT_VERSION
#	define NVTT_VERSION 30203
#endif

/// Proxy for C++'s bool type.
typedef enum
{
	NVTT_False,
	NVTT_True,
} NvttBoolean;

/// See nvtt::ValueType.
typedef enum
{
	NVTT_ValueType_UINT8,
	NVTT_ValueType_SINT8,
	NVTT_ValueType_FLOAT32,
} NvttValueType;

/// See nvtt::ChannelOrder.
typedef enum
{
	NVTT_ChannelOrder_Red = 0,
	NVTT_ChannelOrder_Green = 1,
	NVTT_ChannelOrder_Blue = 2,
	NVTT_ChannelOrder_Alpha = 3,
	NVTT_ChannelOrder_Zero = 4,
	NVTT_ChannelOrder_One = 5,
	NVTT_ChannelOrder_MaxEnum = 0x7FFFFFFF ///< Forces NvttChannelOrder to be a 32-bit enum; not valid to pass to functions.
} NvttChannelOrder;

/// See nvtt::RefImage.
typedef struct
{
	const void * data;
	int width;
	int height;
	int depth;
	int num_channels;
	NvttChannelOrder channel_swizzle[4];
	NvttBoolean channel_interleave;
} NvttRefImage;

/// See nvtt::Format.
typedef enum
{
	// No compression.
	NVTT_Format_RGB,
	NVTT_Format_RGBA = NVTT_Format_RGB,

	// DX9 formats.
	NVTT_Format_DXT1,
	NVTT_Format_DXT1a,
	NVTT_Format_DXT3,
	NVTT_Format_DXT5,
	NVTT_Format_DXT5n,
	
	// DX10 formats.
	NVTT_Format_BC1 = NVTT_Format_DXT1,
	NVTT_Format_BC1a = NVTT_Format_DXT1a,
	NVTT_Format_BC2 = NVTT_Format_DXT3,
	NVTT_Format_BC3 = NVTT_Format_DXT5,
	NVTT_Format_BC3n = NVTT_Format_DXT5n,
	NVTT_Format_BC4,
	NVTT_Format_BC4S,
	NVTT_Format_ATI2,
	NVTT_Format_BC5,
	NVTT_Format_BC5S,

	NVTT_Format_DXT1n,
    NVTT_Format_CTX1,

    NVTT_Format_BC6U,
	NVTT_Format_BC6S,

    NVTT_Format_BC7,

    NVTT_Format_BC3_RGBM,

	NVTT_Format_ASTC_LDR_4x4,
	NVTT_Format_ASTC_LDR_5x4,
	NVTT_Format_ASTC_LDR_5x5,
	NVTT_Format_ASTC_LDR_6x5,
	NVTT_Format_ASTC_LDR_6x6,
	NVTT_Format_ASTC_LDR_8x5,
	NVTT_Format_ASTC_LDR_8x6,
	NVTT_Format_ASTC_LDR_8x8,
	NVTT_Format_ASTC_LDR_10x5,
	NVTT_Format_ASTC_LDR_10x6,
	NVTT_Format_ASTC_LDR_10x8,
	NVTT_Format_ASTC_LDR_10x10,
	NVTT_Format_ASTC_LDR_12x10,
	NVTT_Format_ASTC_LDR_12x12,

	NVTT_Format_Count
} NvttFormat;

/// See nvtt::PixelType.
typedef enum
{
	NVTT_PixelType_UnsignedNorm = 0,
	NVTT_PixelType_SignedNorm = 1,
	NVTT_PixelType_UnsignedInt = 2,
	NVTT_PixelType_SignedInt = 3,
	NVTT_PixelType_Float = 4,
	NVTT_PixelType_UnsignedFloat = 5,
	NVTT_PixelType_SharedExp = 6,
} NvttPixelType;

/// See nvtt::Quality.
typedef enum
{
	NVTT_Quality_Fastest,
	NVTT_Quality_Normal,
	NVTT_Quality_Production,
	NVTT_Quality_Highest,
} NvttQuality;

/// See nvtt::EncodeSettings_Version_1
#define NVTT_EncodeSettings_Version_1 1;

/// See nvtt::EncodeFlags.
typedef enum
{
	NVTT_EncodeFlags_None = 0,
	NVTT_EncodeFlags_UseGPU = 1 << 0,
	NVTT_EncodeFlags_OutputToGPUMem = 1 << 1,
	NVTT_EncodeFlags_Opaque = 1 << 2
} NvttEncodeFlags;

/// See nvtt::EncodeSettings.
/// Note that this supports C99's designated initialization; for instance,
/// here's how to create an EncodeSettings struct that says to compress to
/// ASTC 4x4 at Production quality on the GPU, writing to GPU memory, with
/// a timing context:
/// ```c
/// NvttTimingContext *tc = nvttCreateTimingContext(1);
/// assert(tc != NULL);
/// NvttEncodeSettings settings{
///		.sType = NVTT_EncodeSettings_Version_1,
///		.format = NVTT_Format_ASTC_LDR_4x4,
///		.quality = NVTT_Quality_Production,
///		.timing_context = tc,
///		.encode_flags = NVTT_EncodeFlags_UseGPU | NVTT_EncodeFlags_OutputToGPUMem };
/// ```
typedef struct
{
	uint32_t sType; ///< Must be set to NVTT_EncodeSettings_Version_1
	NvttFormat format;
	NvttQuality quality;
	NvttPixelType rgb_pixel_type;
	NvttTimingContext *timing_context;
	uint32_t encode_flags; ///< Bitmask of NvttEncodeFlags
} NvttEncodeSettings;

/// See nvtt::WrapMode.
typedef enum
{
	NVTT_WrapMode_Clamp,
	NVTT_WrapMode_Repeat,
	NVTT_WrapMode_Mirror,
} NvttWrapMode;

/// See nvtt::TextureType.
typedef enum
{
	NVTT_TextureType_2D,
	NVTT_TextureType_Cube,
	NVTT_TextureType_3D,
} NvttTextureType;

/// See nvtt::InputFormat.
typedef enum
{
	NVTT_InputFormat_BGRA_8UB,
	NVTT_InputFormat_BGRA_8SB,
	NVTT_InputFormat_RGBA_16F,
	NVTT_InputFormat_RGBA_32F,
	NVTT_InputFormat_R_32F,
} NvttInputFormat;

/// See nvtt::MipmapFilter.
typedef enum
{
	NVTT_MipmapFilter_Box,
	NVTT_MipmapFilter_Triangle,
	NVTT_MipmapFilter_Kaiser,
	NVTT_MipmapFilter_Mitchell,
	NVTT_MipmapFilter_Min,
	NVTT_MipmapFilter_Max,
} NvttMipmapFilter;

/// See nvtt::ResizeFilter.
typedef enum
{
	NVTT_ResizeFilter_Box,
	NVTT_ResizeFilter_Triangle,
	NVTT_ResizeFilter_Kaiser,
	NVTT_ResizeFilter_Mitchell,
	NVTT_ResizeFilter_Min,
	NVTT_ResizeFilter_Max,
} NvttResizeFilter;

/// See nvtt::RoundMode.
typedef enum
{
	NVTT_RoundMode_None,
	NVTT_RoundMode_ToNextPowerOfTwo,
	NVTT_RoundMode_ToNearestPowerOfTwo,
	NVTT_RoundMode_ToPreviousPowerOfTwo,
} NvttRoundMode;

/// See nvtt::AlphaMode.
typedef enum
{
	NVTT_AlphaMode_None,
	NVTT_AlphaMode_Transparency,
	NVTT_AlphaMode_Premultiplied,
} NvttAlphaMode;

/// See nvtt::Error.
typedef enum
{
	NVTT_Error_None,
	NVTT_Error_Unknown = NVTT_Error_None,
	NVTT_Error_InvalidInput,
	NVTT_Error_UnsupportedFeature,
	NVTT_Error_CudaError,
	NVTT_Error_FileOpen,
	NVTT_Error_FileWrite,
    NVTT_Error_UnsupportedOutputFormat,
	NVTT_Error_Messaging,
	NVTT_Error_OutOfHostMemory,
	NVTT_Error_OutOfDeviceMemory,
	NVTT_Error_OutputWrite,
	NVTT_Error_Count
} NvttError;

/// See nvtt::Severity.
typedef enum
{
	NVTT_Severity_Info,
	NVTT_Severity_Warning,
	NVTT_Severity_Error,
	NVTT_Severity_Count
} NvttSeverity;

/// See nvtt::Container.
typedef enum
{
	NVTT_Container_DDS,
	NVTT_Container_DDS10,
} NvttContainer;

/// See nvtt::NormalTransform.
typedef enum
{
	NVTT_NormalTransform_Orthographic,
	NVTT_NormalTransform_Stereographic,
	NVTT_NormalTransform_Paraboloid,
	NVTT_NormalTransform_Quartic
} NvttNormalTransform;

/// See nvtt::ToneMapper.
typedef enum
{
	NVTT_ToneMapper_Linear,
	NVTT_ToneMapper_Reinhard,
	NVTT_ToneMapper_Reindhart = NVTT_ToneMapper_Reinhard,
	NVTT_ToneMapper_Halo,
	NVTT_ToneMapper_Lightmap,
} NvttToneMapper;

/// See nvtt::CubeLayout.
typedef enum
{
	NVTT_CubeLayout_VerticalCross,
	NVTT_CubeLayout_HorizontalCross,
	NVTT_CubeLayout_Column,
	NVTT_CubeLayout_Row,
	NVTT_CubeLayout_LatitudeLongitude
} NvttCubeLayout;

/// See nvtt::EdgeFixup.
typedef enum EdgeFixup {
	NVTT_EdgeFixup_None,
	NVTT_EdgeFixup_Stretch,
	NVTT_EdgeFixup_Warp,
	NVTT_EdgeFixup_Average,
} NvttEdgeFixup;


#ifdef __cplusplus
extern "C" {
#endif

// Callbacks
typedef void (* nvttErrorHandler)(NvttError e); ///< See nvtt::ErrorHandler::error()
typedef void (* nvttBeginImageHandler)(int size, int width, int height, int depth, int face, int miplevel); ///< See nvtt::OutputHandler::beginImage()
typedef NvttBoolean (* nvttOutputHandler)(const void * data, int size); ///< See nvtt::OutputHandler::writeData()
typedef void (* nvttEndImageHandler)(); ///< See nvtt::OutputHandler::endImage()
typedef void (* nvttMessageCallback)(NvttSeverity severity, NvttError error, const char * message, const void * userData); ///< See nvtt::MessageCallback.


/// Low-level API global functions.
NVTT_API NvttBoolean nvttIsCudaSupported();
NVTT_API void nvttUseCurrentDevice();


/// CPUInputBuffer class. See nvtt::CPUInputBuffer.
NVTT_API NvttCPUInputBuffer * nvttCreateCPUInputBuffer(const NvttRefImage * images, NvttValueType value_type, int numImages, int tile_w, int tile_h, float WeightR, float WeightG, float WeightB, float WeightA, NvttTimingContext * tc, unsigned * num_tiles);
NVTT_API void nvttDestroyCPUInputBuffer(NvttCPUInputBuffer * input);

NVTT_API int nvttCPUInputBufferNumTiles(const NvttCPUInputBuffer * input);
NVTT_API void nvttCPUInputBufferTileSize(const NvttCPUInputBuffer * input, int * tile_w, int * tile_h);
NVTT_API NvttValueType nvttCPUInputBufferType(const NvttCPUInputBuffer * input);


/// GPUInputBuffer class. See nvtt::GPUInputBuffer.
NVTT_API NvttGPUInputBuffer * nvttCreateGPUInputBuffer(const NvttRefImage * images, NvttValueType value_type, int numImages, int tile_w, int tile_h, float WeightR, float WeightG, float WeightB, float WeightA, NvttTimingContext * tc, unsigned * num_tiles);
NVTT_API void nvttDestroyGPUInputBuffer(NvttGPUInputBuffer * input);

NVTT_API int nvttGPUInputBufferNumTiles(const NvttGPUInputBuffer * input);
NVTT_API void nvttGPUInputBufferTileSize(const NvttGPUInputBuffer * input, int * tile_w, int * tile_h);
NVTT_API NvttValueType nvttGPUInputBufferType(const NvttGPUInputBuffer * input);


/// Low-level API global compression functions. These are named after their
/// corresponding low-level C++ API equivalents.
NVTT_API NvttBoolean nvttEncodeCPU(const NvttCPUInputBuffer * input, void * output, const NvttEncodeSettings * settings);
NVTT_API NvttBoolean nvttEncodeGPU(const NvttGPUInputBuffer * input, void * output, const NvttEncodeSettings * settings);
NVTT_API void nvttEncodeBC1CPU(const NvttCPUInputBuffer * input, NvttBoolean fast_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC1GPU(const NvttGPUInputBuffer * input, NvttBoolean fast_mode, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC1ACPU(const NvttCPUInputBuffer * input, NvttBoolean fast_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC1AGPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC2CPU(const NvttCPUInputBuffer * input, NvttBoolean fast_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC2GPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC3CPU(const NvttCPUInputBuffer * input, NvttBoolean fast_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC3GPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC3NCPU(const NvttCPUInputBuffer * input, int qualityLevel, void * output, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC3RGBMCPU(const NvttCPUInputBuffer * input, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC4CPU(const NvttCPUInputBuffer * input, NvttBoolean slow_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC4GPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC4SCPU(const NvttCPUInputBuffer * input, NvttBoolean slow_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC4SGPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeATI2CPU(const NvttCPUInputBuffer * input, NvttBoolean slow_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeATI2GPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC5CPU(const NvttCPUInputBuffer * input, NvttBoolean slow_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC5GPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC5SCPU(const NvttCPUInputBuffer * input, NvttBoolean slow_mode, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC5SGPU(const NvttGPUInputBuffer * input, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC6HCPU(const NvttCPUInputBuffer * input, NvttBoolean slow_mode, NvttBoolean is_signed, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC6HGPU(const NvttGPUInputBuffer * input, NvttBoolean is_signed, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC7CPU(const NvttCPUInputBuffer * input, NvttBoolean slow_mode, NvttBoolean imageHasAlpha, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeBC7GPU(const NvttGPUInputBuffer * input, NvttBoolean imageHasAlpha, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeASTCCPU(const NvttCPUInputBuffer * input, int qualityLevel, NvttBoolean imageHasAlpha, void * output, NvttBoolean useGpu, NvttBoolean to_device_mem, NvttTimingContext * tc);
NVTT_API void nvttEncodeASTCGPU(const NvttGPUInputBuffer * input, int qualityLevel, NvttBoolean imageHasAlpha, void * output, NvttBoolean to_device_mem, NvttTimingContext * tc);


// High-level API.

/// CompressionOptions class. See nvtt::CompressionOptions.
NVTT_API NvttCompressionOptions * nvttCreateCompressionOptions();
NVTT_API void nvttDestroyCompressionOptions(NvttCompressionOptions * compressionOptions);

NVTT_API void nvttResetCompressionOptions(NvttCompressionOptions * compressionOptions);
NVTT_API void nvttSetCompressionOptionsFormat(NvttCompressionOptions * compressionOptions, NvttFormat format);
NVTT_API void nvttSetCompressionOptionsQuality(NvttCompressionOptions * compressionOptions, NvttQuality quality);
NVTT_API void nvttSetCompressionOptionsColorWeights(NvttCompressionOptions * compressionOptions, float red, float green, float blue, float alpha);
NVTT_API void nvttSetCompressionOptionsPixelFormat(NvttCompressionOptions * compressionOptions, unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask);
NVTT_API void nvttSetCompressionOptionsPixelType(NvttCompressionOptions * compressionOptions, NvttPixelType pixelType);
NVTT_API void nvttSetCompressionOptionsPitchAlignment(NvttCompressionOptions * compressionOptions, int pitchAlignment);
NVTT_API void nvttSetCompressionOptionsQuantization(NvttCompressionOptions * compressionOptions, NvttBoolean colorDithering, NvttBoolean alphaDithering, NvttBoolean binaryAlpha, int alphaThreshold);
NVTT_API unsigned int nvttGetCompressionOptionsD3D9Format(const NvttCompressionOptions * compressionOptions);


/// OutputOptions class. See nvtt::OutputOptions.
NVTT_API NvttOutputOptions * nvttCreateOutputOptions();
NVTT_API void nvttDestroyOutputOptions(NvttOutputOptions * outputOptions);

NVTT_API void nvttResetOutputOptions(NvttOutputOptions * outputOptions);
NVTT_API void nvttSetOutputOptionsFileName(NvttOutputOptions * outputOptions, const char * fileName);
NVTT_API void nvttSetOutputOptionsFileHandle(NvttOutputOptions * outputOptions, void * fp);
NVTT_API void nvttSetOutputOptionsOutputHandler(NvttOutputOptions * outputOptions, nvttBeginImageHandler beginImageHandler, nvttOutputHandler outputHandler, nvttEndImageHandler endImageHandler);
NVTT_API void nvttSetOutputOptionsErrorHandler(NvttOutputOptions * outputOptions, nvttErrorHandler errorHandler);
NVTT_API void nvttSetOutputOptionsOutputHeader(NvttOutputOptions * outputOptions, NvttBoolean b);
NVTT_API void nvttSetOutputOptionsContainer(NvttOutputOptions * outputOptions, NvttContainer container);
NVTT_API void nvttSetOutputOptionsUserVersion(NvttOutputOptions * outputOptions, int version);
NVTT_API void nvttSetOutputOptionsSrgbFlag(NvttOutputOptions * outputOptions, NvttBoolean b);


/// Compression context class. See nvtt::Context.
NVTT_API NvttContext * nvttCreateContext();
NVTT_API void nvttDestroyContext(NvttContext * context);

NVTT_API void nvttSetContextCudaAcceleration(NvttContext * context, NvttBoolean enable);
NVTT_API NvttBoolean nvttContextIsCudaAccelerationEnabled(const NvttContext * context);
NVTT_API NvttBoolean nvttContextOutputHeader(const NvttContext * context, const NvttSurface * img, int mipmapCount, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions);
NVTT_API NvttBoolean nvttContextCompress(const NvttContext * context, const NvttSurface * img, int face, int mipmap, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions);
NVTT_API int nvttContextEstimateSize(const NvttContext * context, const NvttSurface * img, int mipmapCount, const NvttCompressionOptions * compressionOptions);
NVTT_API void nvttContextQuantize(const NvttContext * context, NvttSurface * tex, const NvttCompressionOptions * compressionOptions);
NVTT_API NvttBoolean nvttContextOutputHeaderCube(const NvttContext * context, const NvttCubeSurface * img, int mipmapCount, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions);
NVTT_API NvttBoolean nvttContextCompressCube(const NvttContext * context, const NvttCubeSurface * img, int mipmap, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions);
NVTT_API int nvttContextEstimateSizeCube(const NvttContext * context, const NvttCubeSurface * img, int mipmapCount, const NvttCompressionOptions * compressionOptions);
NVTT_API NvttBoolean nvttContextOutputHeaderData(const NvttContext * context, NvttTextureType type, int w, int h, int d, int mipmapCount, NvttBoolean isNormalMap, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions);
NVTT_API NvttBoolean nvttContextCompressData(const NvttContext * context, int w, int h, int d, int face, int mipmap, const float * rgba, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions);
NVTT_API int nvttContextEstimateSizeData(const NvttContext * context, int w, int h, int d, int mipmapCount, const NvttCompressionOptions * compressionOptions);
NVTT_API NvttBoolean nvttContextCompressBatch(const NvttContext * context, const NvttBatchList * lst, const NvttCompressionOptions * compressionOptions);
NVTT_API void nvttContextEnableTiming(NvttContext * context, NvttBoolean enable, int detailLevel);
NVTT_API NvttTimingContext * nvttContextGetTimingContext(NvttContext * context);


/// Surface class. See nvtt::Surface.
NVTT_API NvttSurface * nvttCreateSurface();
NVTT_API void nvttDestroySurface(NvttSurface * surface);
NVTT_API NvttSurface * nvttSurfaceClone(const NvttSurface * surface);

NVTT_API void nvttSetSurfaceWrapMode(NvttSurface * surface, NvttWrapMode mode);
NVTT_API void nvttSetSurfaceAlphaMode(NvttSurface * surface, NvttAlphaMode alphaMode);
NVTT_API void nvttSetSurfaceNormalMap(NvttSurface * surface, NvttBoolean isNormalMap);
/// Note that if `surface == NULL`, this prints an error, but returns NVTT_True (1) instead of NVTT_False (0).
NVTT_API NvttBoolean nvttSurfaceIsNull(const NvttSurface * surface);
NVTT_API int nvttSurfaceWidth(const NvttSurface * surface);
NVTT_API int nvttSurfaceHeight(const NvttSurface * surface);
NVTT_API int nvttSurfaceDepth(const NvttSurface * surface);
NVTT_API NvttTextureType nvttSurfaceType(const NvttSurface * surface);
NVTT_API NvttWrapMode nvttSurfaceWrapMode(const NvttSurface * surface);
NVTT_API NvttAlphaMode nvttSurfaceAlphaMode(const NvttSurface * surface);
NVTT_API NvttBoolean nvttSurfaceIsNormalMap(const NvttSurface * surface);
NVTT_API int nvttSurfaceCountMipmaps(const NvttSurface * surface, int min_size);
NVTT_API float nvttSurfaceAlphaTestCoverage(const NvttSurface * surface, float alphaRef, int alpha_channel);
NVTT_API float nvttSurfaceAverage(const NvttSurface * surface, int channel, int alpha_channel, float gamma);
NVTT_API float * nvttSurfaceData(NvttSurface * surface);
NVTT_API float * nvttSurfaceChannel(NvttSurface * surface, int i);
NVTT_API void nvttSurfaceHistogram(const NvttSurface * surface, int channel, float rangeMin, float rangeMax, int binCount, int * binPtr, NvttTimingContext * tc);
NVTT_API void nvttSurfaceRange(const NvttSurface * surface, int channel, float * rangeMin, float * rangeMax, int alpha_channel, float alpha_ref, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceLoad(NvttSurface * surface, const char * filename, NvttBoolean * hasAlpha, NvttBoolean expectSigned, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceLoadFromMemory(NvttSurface * surface, const void * data, unsigned long long sizeInBytes, NvttBoolean * hasAlpha, NvttBoolean expectSigned, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceSave(const NvttSurface * surface, const char * fileName, NvttBoolean hasAlpha, NvttBoolean hdr, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceSetImage(NvttSurface * surface, int w, int h, int d, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceSetImageData(NvttSurface * surface, NvttInputFormat format, int w, int h, int d, const void * data, NvttBoolean unsignedToSigned, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceSetImageRGBA(NvttSurface * surface, NvttInputFormat format, int w, int h, int d, const void * r, const void * g, const void * b, const void * a, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceSetImage2D(NvttSurface * surface, NvttFormat format, int w, int h, const void * data, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceSetImage3D(NvttSurface * surface, NvttFormat format, int w, int h, int d, const void * data, NvttTimingContext * tc);
NVTT_API void nvttSurfaceResize(NvttSurface * surface, int w, int h, int d, NvttResizeFilter filter, float filterWidth, const float * params, NvttTimingContext * tc);
NVTT_API void nvttSurfaceResizeMax(NvttSurface * surface, int maxExtent, NvttRoundMode mode, NvttResizeFilter filter, NvttTimingContext * tc);
NVTT_API void nvttSurfaceResizeMaxParams(NvttSurface * surface, int maxExtent, NvttRoundMode mode, NvttResizeFilter filter, float filterWidth, const float * params, NvttTimingContext * tc);
NVTT_API void nvttSurfaceResizeMakeSquare(NvttSurface * surface, int maxExtent, NvttRoundMode mode, NvttResizeFilter filter, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceBuildNextMipmap(NvttSurface * surface, NvttMipmapFilter filter, float filterWidth, const float * params, int min_size, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceBuildNextMipmapDefaults(NvttSurface* surface, NvttMipmapFilter filter, int min_size, NvttTimingContext* tc);
NVTT_API NvttBoolean nvttSurfaceBuildNextMipmapSolidColor(NvttSurface * surface, const float * const color_components, NvttTimingContext * tc);
NVTT_API void nvttSurfaceCanvasSize(NvttSurface * surface, int w, int h, int d, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceCanMakeNextMipmap(NvttSurface * surface, int min_size);
NVTT_API void nvttSurfaceToLinear(NvttSurface * surface, float gamma, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToGamma(NvttSurface * surface, float gamma, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToLinearChannel(NvttSurface * surface, int channel, float gamma, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToGammaChannel(NvttSurface * surface, int channel, float gamma, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToSrgb(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToSrgbUnclamped(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToLinearFromSrgb(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToLinearFromSrgbUnclamped(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToXenonSrgb(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToLinearFromXenonSrgb(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceTransform(NvttSurface * surface, const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4], NvttTimingContext * tc);
NVTT_API void nvttSurfaceSwizzle(NvttSurface * surface, int r, int g, int b, int a, NvttTimingContext * tc);
NVTT_API void nvttSurfaceScaleBias(NvttSurface * surface, int channel, float scale, float bias, NvttTimingContext * tc);
NVTT_API void nvttSurfaceClamp(NvttSurface * surface, int channel, float low, float high, NvttTimingContext * tc);
NVTT_API void nvttSurfaceBlend(NvttSurface * surface, float r, float g, float b, float a, float t, NvttTimingContext * tc);
NVTT_API void nvttSurfacePremultiplyAlpha(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceDemultiplyAlpha(NvttSurface * surface, float epsilon, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToGreyScale(NvttSurface * surface, float redScale, float greenScale, float blueScale, float alphaScale, NvttTimingContext * tc);
NVTT_API void nvttSurfaceSetBorder(NvttSurface * surface, float r, float g, float b, float a, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFill(NvttSurface * surface, float r, float g, float b, float a, NvttTimingContext * tc);
NVTT_API void nvttSurfaceScaleAlphaToCoverage(NvttSurface * surface, float coverage, float alphaRef, int alpha_channel, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToRGBM(NvttSurface * surface, float range, float threshold, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFromRGBM(NvttSurface * surface, float range, float threshold, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToLM(NvttSurface * surface, float range, float threshold, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToRGBE(NvttSurface * surface, int mantissaBits, int exponentBits, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFromRGBE(NvttSurface * surface, int mantissaBits, int exponentBits, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToYCoCg(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceBlockScaleCoCg(NvttSurface * surface, int bits, float threshold, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFromYCoCg(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToLUVW(NvttSurface * surface, float range, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFromLUVW(NvttSurface * surface, float range, NvttTimingContext * tc);
NVTT_API void nvttSurfaceAbs(NvttSurface * surface, int channel, NvttTimingContext * tc);
NVTT_API void nvttSurfaceConvolve(NvttSurface * surface, int channel, int kernelSize, float * kernelData, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToLogScale(NvttSurface * surface, int channel, float base, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFromLogScale(NvttSurface * surface, int channel, float base, NvttTimingContext * tc);
NVTT_API void nvttSurfaceSetAtlasBorder(NvttSurface * surface, int w, int h, float r, float g, float b, float a, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToneMap(NvttSurface * surface, NvttToneMapper tm, float * parameters, NvttTimingContext * tc);
NVTT_API void nvttSurfaceBinarize(NvttSurface * surface, int channel, float threshold, NvttBoolean dither, NvttTimingContext * tc);
NVTT_API void nvttSurfaceQuantize(NvttSurface * surface, int channel, int bits, NvttBoolean exactEndPoints, NvttBoolean dither, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToNormalMap(NvttSurface * surface, float sm, float medium, float big, float large, NvttTimingContext * tc);
NVTT_API void nvttSurfaceNormalizeNormalMap(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceTransformNormals(NvttSurface * surface, NvttNormalTransform xform, NvttTimingContext * tc);
NVTT_API void nvttSurfaceReconstructNormals(NvttSurface * surface, NvttNormalTransform xform, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToCleanNormalMap(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfacePackNormals(NvttSurface * surface, float scale, float bias, NvttTimingContext * tc);
NVTT_API void nvttSurfaceExpandNormals(NvttSurface * surface, float scale, float bias, NvttTimingContext * tc);
NVTT_API NvttSurface * nvttSurfaceCreateToksvigMap(const NvttSurface * surface, float power, NvttTimingContext * tc);
NVTT_API NvttSurface * nvttSurfaceCreateCleanMap(const NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFlipX(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFlipY(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API void nvttSurfaceFlipZ(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API NvttSurface * nvttSurfaceCreateSubImage(const NvttSurface * surface, int x0, int x1, int y0, int y1, int z0, int z1, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceCopyChannel(NvttSurface * surface, const NvttSurface * srcImage, int srcChannel, int dstChannel, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceAddChannel(NvttSurface * surface, const NvttSurface * srcImage, int srcChannel, int dstChannel, float scale, NvttTimingContext * tc);
NVTT_API NvttBoolean nvttSurfaceCopy(NvttSurface * surface, const NvttSurface * srcImage, int xsrc, int ysrc, int zsrc, int xsize, int ysize, int zsize, int xdst, int ydst, int zdst, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToGPU(NvttSurface * surface, NvttBoolean performCopy, NvttTimingContext * tc);
NVTT_API void nvttSurfaceToCPU(NvttSurface * surface, NvttTimingContext * tc);
NVTT_API const float * nvttSurfaceGPUData(const NvttSurface * surface);
NVTT_API float * nvttSurfaceGPUDataMutable(NvttSurface* surface);


/// SurfaceSet class. See nvtt::SurfaceSet.
NVTT_API NvttSurfaceSet * nvttCreateSurfaceSet();
NVTT_API void nvttDestroySurfaceSet(NvttSurfaceSet * surfaceSet);
NVTT_API void nvttResetSurfaceSet(NvttSurfaceSet * surfaceSet);

NVTT_API NvttTextureType nvttSurfaceSetGetTextureType(NvttSurfaceSet * surfaceSet);
NVTT_API int nvttSurfaceSetGetFaceCount(NvttSurfaceSet * surfaceSet);
NVTT_API int nvttSurfaceSetGetMipmapCount(NvttSurfaceSet * surfaceSet);
NVTT_API int nvttSurfaceSetGetWidth(NvttSurfaceSet * surfaceSet);
NVTT_API int nvttSurfaceSetGetHeight(NvttSurfaceSet * surfaceSet);
NVTT_API int nvttSurfaceSetGetDepth(NvttSurfaceSet * surfaceSet);
NVTT_API NvttSurface * nvttSurfaceSetGetSurface(NvttSurfaceSet * surfaceSet, int faceId, int mipId, NvttBoolean expectSigned);
NVTT_API NvttBoolean nvttSurfaceSetLoadDDS(NvttSurfaceSet * surfaceSet, const char * fileName, NvttBoolean forcenormal);
NVTT_API NvttBoolean nvttSurfaceSetLoadDDSFromMemory(NvttSurfaceSet * surfaceSet, const void * data, unsigned long long sizeInBytes, NvttBoolean forcenormal);
NVTT_API NvttBoolean nvttSurfaceSetSaveImage(NvttSurfaceSet * surfaceSet, const char * fileName, int faceId, int mipId);


/// CubeSurface class. See nvtt::CubeSurface.
NVTT_API NvttCubeSurface * nvttCreateCubeSurface();
NVTT_API void nvttDestroyCubeSurface(NvttCubeSurface * cubeSurface);

/// Note that if `cubeSurface == NULL`, this prints an error, but returns NVTT_True (1) instead of NVTT_False (0).
NVTT_API NvttBoolean nvttCubeSurfaceIsNull(const NvttCubeSurface * cubeSurface);
NVTT_API int nvttCubeSurfaceEdgeLength(const NvttCubeSurface * cubeSurface);
NVTT_API int nvttCubeSurfaceCountMipmaps(const NvttCubeSurface * cubeSurface);
NVTT_API NvttBoolean nvttCubeSurfaceLoad(NvttCubeSurface * cubeSurface, const char * fileName, int mipmap);
NVTT_API NvttBoolean nvttCubeSurfaceLoadFromMemory(NvttCubeSurface * cubeSurface, const void * data, unsigned long long sizeInBytes, int mipmap);
NVTT_API NvttBoolean nvttCubeSurfaceSave(NvttCubeSurface * cubeSurface, const char * fileName);
/// Note that unlike most C API functions returning an NvttSurface pointer, this
/// returns a pointer to an existing NvttSurface, rather than a new NvttSurface.
NVTT_API NvttSurface * nvttCubeSurfaceFace(NvttCubeSurface * cubeSurface, int face);
NVTT_API void nvttCubeSurfaceFold(NvttCubeSurface * cubeSurface, const NvttSurface * img, NvttCubeLayout layout);
NVTT_API NvttSurface * nvttCubeSurfaceUnfold(const NvttCubeSurface * cubeSurface, NvttCubeLayout layout);
NVTT_API float nvttCubeSurfaceAverage(NvttCubeSurface * cubeSurface, int channel);
NVTT_API void nvttCubeSurfaceRange(const NvttCubeSurface * cubeSurface, int channel, float * minimum_ptr, float * maximum_ptr);
NVTT_API void nvttCubeSurfaceClamp(NvttCubeSurface * cubeSurface, int channel, float low, float high);
NVTT_API NvttCubeSurface * nvttCubeSurfaceIrradianceFilter(const NvttCubeSurface * cubeSurface, int size, NvttEdgeFixup fixupMethod);
NVTT_API NvttCubeSurface * nvttCubeSurfaceCosinePowerFilter(const NvttCubeSurface * cubeSurface, int size, float cosinePower, NvttEdgeFixup fixupMethod);
NVTT_API NvttCubeSurface * nvttCubeSurfaceFastResample(const NvttCubeSurface * cubeSurface, int size, NvttEdgeFixup fixupMethod);
NVTT_API void nvttCubeSurfaceToLinear(NvttCubeSurface * cubeSurface, float gamma);
NVTT_API void nvttCubeSurfaceToGamma(NvttCubeSurface * cubeSurface, float gamma);


/// BatchList class. See nvtt::BatchList.
NVTT_API NvttBatchList * nvttCreateBatchList();
NVTT_API void nvttDestroyBatchList(NvttBatchList * batchList);

NVTT_API void nvttBatchListClear(NvttBatchList * batchList);
NVTT_API void nvttBatchListAppend(NvttBatchList * batchList, const NvttSurface * pImg, int face, int mipmap, const NvttOutputOptions * outputOptions);
NVTT_API unsigned nvttBatchListGetSize(const NvttBatchList * batchList);
NVTT_API void nvttBatchListGetItem(const NvttBatchList * batchList, unsigned i, const NvttSurface ** pImg, int * face, int * mipmap, const NvttOutputOptions ** outputOptions);


/// Global functions.
NVTT_API const char * nvttErrorString(NvttError e);
NVTT_API unsigned int nvttVersion();
NVTT_API NvttBoolean nvttSetMessageCallback(nvttMessageCallback callback, const void * userData);
NVTT_API float nvttRmsError(const NvttSurface * reference, const NvttSurface * img, NvttTimingContext * tc);
NVTT_API float nvttRmsAlphaError(const NvttSurface * reference, const NvttSurface * img, NvttTimingContext * tc);
NVTT_API float nvttRmsCIELabError(const NvttSurface * reference, const NvttSurface * img, NvttTimingContext * tc);
NVTT_API float nvttAngularError(const NvttSurface * reference, const NvttSurface * img, NvttTimingContext * tc);
NVTT_API NvttSurface * nvttDiff(const NvttSurface * reference, const NvttSurface * img, float scale, NvttTimingContext * tc);
NVTT_API float nvttRmsToneMappedError(const NvttSurface * reference, const NvttSurface * img, float exposure, NvttTimingContext * tc);
NVTT_API NvttSurface * nvttHistogram(const NvttSurface * img, int width, int height, NvttTimingContext * tc);
NVTT_API NvttSurface * nvttHistogramRange(const NvttSurface * img, float minRange, float maxRange, int width, int height, NvttTimingContext * tc);
NVTT_API void nvttGetTargetExtent(int * width, int * height, int * depth, int maxExtent, NvttRoundMode roundMode, NvttTextureType textureType, NvttTimingContext * tc);
NVTT_API int nvttCountMipmaps(int w, int h, int d, NvttTimingContext * tc);


/// TimingContext class. See nvtt::TimingContext.
NVTT_API NvttTimingContext * nvttCreateTimingContext(int detailLevel);
NVTT_API void nvttDestroyTimingContext(NvttTimingContext * timingContext);

NVTT_API void nvttTimingContextSetDetailLevel(NvttTimingContext * timingContext, int detailLevel);
NVTT_API int nvttTimingContextGetRecordCount(NvttTimingContext * timingContext);
NVTT_API void nvttTimingContextGetRecord(NvttTimingContext * timingContext, int i, char * description, double * seconds);
NVTT_API size_t nvttTimingContextGetRecordSafe(NvttTimingContext * timingContext, int i, char * outDescription, size_t outDescriptionSize, double * seconds);
NVTT_API void nvttTimingContextPrintRecords(NvttTimingContext * timingContext);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NVTT_WRAPPER_H
