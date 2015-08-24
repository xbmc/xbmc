/*
 *      Copyright (C) 2010-2014 Hendrik Leppkes
 *      http://www.1f0.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

// {FA40D6E9-4D38-4761-ADD2-71A9EC5FD32F}
DEFINE_GUID(IID_ILAVVideoSettings, 
0xfa40d6e9, 0x4d38, 0x4761, 0xad, 0xd2, 0x71, 0xa9, 0xec, 0x5f, 0xd3, 0x2f);

// {1CC2385F-36FA-41B1-9942-5024CE0235DC}
DEFINE_GUID(IID_ILAVVideoStatus,
0x1cc2385f, 0x36fa, 0x41b1, 0x99, 0x42, 0x50, 0x24, 0xce, 0x2, 0x35, 0xdc);


// Codecs supported in the LAV Video configuration
// Codecs not listed here cannot be turned off. You can request codecs to be added to this list, if you wish.
typedef enum LAVVideoCodec {
  Codec_H264,
  Codec_VC1,
  Codec_MPEG1,
  Codec_MPEG2,
  Codec_MPEG4,
  Codec_MSMPEG4,
  Codec_VP8,
  Codec_WMV3,
  Codec_WMV12,
  Codec_MJPEG,
  Codec_Theora,
  Codec_FLV1,
  Codec_VP6,
  Codec_SVQ,
  Codec_H261,
  Codec_H263,
  Codec_Indeo,
  Codec_TSCC,
  Codec_Fraps,
  Codec_HuffYUV,
  Codec_QTRle,
  Codec_DV,
  Codec_Bink,
  Codec_Smacker,
  Codec_RV12,
  Codec_RV34,
  Codec_Lagarith,
  Codec_Cinepak,
  Codec_Camstudio,
  Codec_QPEG,
  Codec_ZLIB,
  Codec_QTRpza,
  Codec_PNG,
  Codec_MSRLE,
  Codec_ProRes,
  Codec_UtVideo,
  Codec_Dirac,
  Codec_DNxHD,
  Codec_MSVideo1,
  Codec_8BPS,
  Codec_LOCO,
  Codec_ZMBV,
  Codec_VCR1,
  Codec_Snow,
  Codec_FFV1,
  Codec_v210,
  Codec_JPEG2000,
  Codec_VMNC,
  Codec_FLIC,
  Codec_G2M,
  Codec_ICOD,
  Codec_THP,
  Codec_HEVC,
  Codec_VP9,
  Codec_TrueMotion,
  Codec_VP7,

  Codec_VideoNB            // Number of entries (do not use when dynamically linking)
};

// Codecs with hardware acceleration
typedef enum LAVVideoHWCodec {
  HWCodec_H264  = Codec_H264,
  HWCodec_VC1   = Codec_VC1,
  HWCodec_MPEG2 = Codec_MPEG2,
  HWCodec_MPEG4 = Codec_MPEG4,
  HWCodec_MPEG2DVD,
  HWCodec_HEVC,

  HWCodec_NB    = HWCodec_HEVC + 1
};

// Flags for HW Resolution support
#define LAVHWResFlag_SD      0x0001
#define LAVHWResFlag_HD      0x0002
#define LAVHWResFlag_UHD     0x0004

// Type of hardware accelerations
typedef enum LAVHWAccel {
  HWAccel_None,
  HWAccel_CUDA,
  HWAccel_QuickSync,
  HWAccel_DXVA2,
  HWAccel_DXVA2CopyBack = HWAccel_DXVA2,
  HWAccel_DXVA2Native
};

// Deinterlace algorithms offered by the hardware decoders
typedef enum LAVHWDeintModes {
  HWDeintMode_Weave,
  HWDeintMode_BOB, // Deprecated
  HWDeintMode_Hardware
};

// Software deinterlacing algorithms
typedef enum LAVSWDeintModes {
  SWDeintMode_None,
  SWDeintMode_YADIF
};

// Deinterlacing processing mode
typedef enum LAVDeintMode {
  DeintMode_Auto,
  DeintMode_Aggressive,
  DeintMode_Force,
  DeintMode_Disable
};

// Type of deinterlacing to perform
// - FramePerField re-constructs one frame from every field, resulting in 50/60 fps.
// - FramePer2Field re-constructs one frame from every 2 fields, resulting in 25/30 fps.
// Note: Weave will always use FramePer2Field
typedef enum LAVDeintOutput {
  DeintOutput_FramePerField,
  DeintOutput_FramePer2Field
};

// Control the field order of the deinterlacer
typedef enum LAVDeintFieldOrder {
  DeintFieldOrder_Auto,
  DeintFieldOrder_TopFieldFirst,
  DeintFieldOrder_BottomFieldFirst,
};

// Supported output pixel formats
typedef enum LAVOutPixFmts {
  LAVOutPixFmt_None = -1,
  LAVOutPixFmt_YV12,            // 4:2:0, 8bit, planar
  LAVOutPixFmt_NV12,            // 4:2:0, 8bit, Y planar, U/V packed
  LAVOutPixFmt_YUY2,            // 4:2:2, 8bit, packed
  LAVOutPixFmt_UYVY,            // 4:2:2, 8bit, packed
  LAVOutPixFmt_AYUV,            // 4:4:4, 8bit, packed
  LAVOutPixFmt_P010,            // 4:2:0, 10bit, Y planar, U/V packed
  LAVOutPixFmt_P210,            // 4:2:2, 10bit, Y planar, U/V packed
  LAVOutPixFmt_Y410,            // 4:4:4, 10bit, packed
  LAVOutPixFmt_P016,            // 4:2:0, 16bit, Y planar, U/V packed
  LAVOutPixFmt_P216,            // 4:2:2, 16bit, Y planar, U/V packed
  LAVOutPixFmt_Y416,            // 4:4:4, 16bit, packed
  LAVOutPixFmt_RGB32,           // 32-bit RGB (BGRA)
  LAVOutPixFmt_RGB24,           // 24-bit RGB (BGR)

  LAVOutPixFmt_v210,            // 4:2:2, 10bit, packed
  LAVOutPixFmt_v410,            // 4:4:4, 10bit, packed

  LAVOutPixFmt_YV16,            // 4:2:2, 8-bit, planar
  LAVOutPixFmt_YV24,            // 4:4:4, 8-bit, planar

  LAVOutPixFmt_RGB48,           // 48-bit RGB (16-bit per pixel, BGR)

  LAVOutPixFmt_NB               // Number of formats
} LAVOutPixFmts;

typedef enum LAVDitherMode {
  LAVDither_Ordered,
  LAVDither_Random
} LAVDitherMode;

// LAV Video configuration interface
[uuid("FA40D6E9-4D38-4761-ADD2-71A9EC5FD32F")]
interface ILAVVideoSettings : public IUnknown
{
  // Switch to Runtime Config mode. This will reset all settings to default, and no changes to the settings will be saved
  // You can use this to programmatically configure LAV Video without interfering with the users settings in the registry.
  // Subsequent calls to this function will reset all settings back to defaults, even if the mode does not change.
  //
  // Note that calling this function during playback is not supported and may exhibit undocumented behaviour. 
  // For smooth operations, it must be called before LAV Video is connected to other filters.
  STDMETHOD(SetRuntimeConfig)(BOOL bRuntimeConfig) = 0;

  // Configure which codecs are enabled
  // If vCodec is invalid (possibly a version difference), Get will return FALSE, and Set E_FAIL.
  STDMETHOD_(BOOL,GetFormatConfiguration)(LAVVideoCodec vCodec) = 0;
  STDMETHOD(SetFormatConfiguration)(LAVVideoCodec vCodec, BOOL bEnabled) = 0;

  // Set the number of threads to use for Multi-Threaded decoding (where available)
  //  0 = Auto Detect (based on number of CPU cores)
  //  1 = 1 Thread -- No Multi-Threading
  // >1 = Multi-Threading with the specified number of threads
  STDMETHOD(SetNumThreads)(DWORD dwNum) = 0;

  // Get the number of threads to use for Multi-Threaded decoding (where available)
  //  0 = Auto Detect (based on number of CPU cores)
  //  1 = 1 Thread -- No Multi-Threading
  // >1 = Multi-Threading with the specified number of threads
  STDMETHOD_(DWORD,GetNumThreads)() = 0;

  // Set whether the aspect ratio encoded in the stream should be forwarded to the renderer,
  // or the aspect ratio specified by the source filter should be kept.
  // 0 = AR from the source filter
  // 1 = AR from the Stream
  // 2 = AR from stream if source is not reliable
  STDMETHOD(SetStreamAR)(DWORD bStreamAR) = 0;

  // Get whether the aspect ratio encoded in the stream should be forwarded to the renderer,
  // or the aspect ratio specified by the source filter should be kept.
  // 0 = AR from the source filter
  // 1 = AR from the Stream
  // 2 = AR from stream if source is not reliable
  STDMETHOD_(DWORD,GetStreamAR)() = 0;

  // Configure which pixel formats are enabled for output
  // If pixFmt is invalid, Get will return FALSE and Set E_FAIL
  STDMETHOD_(BOOL,GetPixelFormat)(LAVOutPixFmts pixFmt) = 0;
  STDMETHOD(SetPixelFormat)(LAVOutPixFmts pixFmt, BOOL bEnabled) = 0;

  // Set the RGB output range for the YUV->RGB conversion
  // 0 = Auto (same as input), 1 = Limited (16-235), 2 = Full (0-255)
  STDMETHOD(SetRGBOutputRange)(DWORD dwRange) = 0;

  // Get the RGB output range for the YUV->RGB conversion
  // 0 = Auto (same as input), 1 = Limited (16-235), 2 = Full (0-255)
  STDMETHOD_(DWORD,GetRGBOutputRange)() = 0;

  // Set the deinterlacing field order of the hardware decoder
  STDMETHOD(SetDeintFieldOrder)(LAVDeintFieldOrder fieldOrder) = 0;

  // get the deinterlacing field order of the hardware decoder
  STDMETHOD_(LAVDeintFieldOrder, GetDeintFieldOrder)() = 0;

  // DEPRECATED, use SetDeinterlacingMode
  STDMETHOD(SetDeintAggressive)(BOOL bAggressive) = 0;

  // DEPRECATED, use GetDeinterlacingMode
  STDMETHOD_(BOOL, GetDeintAggressive)() = 0;

  // DEPRECATED, use SetDeinterlacingMode
  STDMETHOD(SetDeintForce)(BOOL bForce) = 0;

  // DEPRECATED, use GetDeinterlacingMode
  STDMETHOD_(BOOL, GetDeintForce)() = 0;

  // Check if the specified HWAccel is supported
  // Note: This will usually only check the availability of the required libraries (ie. for NVIDIA if a recent enough NVIDIA driver is installed)
  // and not check actual hardware support
  // Returns: 0 = Unsupported, 1 = Supported, 2 = Currently running
  STDMETHOD_(DWORD,CheckHWAccelSupport)(LAVHWAccel hwAccel) = 0;

  // Set which HW Accel method is used
  // See LAVHWAccel for options.
  STDMETHOD(SetHWAccel)(LAVHWAccel hwAccel) = 0;

  // Get which HW Accel method is active
  STDMETHOD_(LAVHWAccel, GetHWAccel)() = 0;

  // Set which codecs should use HW Acceleration
  STDMETHOD(SetHWAccelCodec)(LAVVideoHWCodec hwAccelCodec, BOOL bEnabled) = 0;

  // Get which codecs should use HW Acceleration
  STDMETHOD_(BOOL, GetHWAccelCodec)(LAVVideoHWCodec hwAccelCodec) = 0;

  // Set the deinterlacing mode used by the hardware decoder
  STDMETHOD(SetHWAccelDeintMode)(LAVHWDeintModes deintMode) = 0;

  // Get the deinterlacing mode used by the hardware decoder
  STDMETHOD_(LAVHWDeintModes, GetHWAccelDeintMode)() = 0;

  // Set the deinterlacing output for the hardware decoder
  STDMETHOD(SetHWAccelDeintOutput)(LAVDeintOutput deintOutput) = 0;

  // Get the deinterlacing output for the hardware decoder
  STDMETHOD_(LAVDeintOutput, GetHWAccelDeintOutput)() = 0;

  // Set whether the hardware decoder should force high-quality deinterlacing
  // Note: this option is not supported on all decoder implementations and/or all operating systems
  STDMETHOD(SetHWAccelDeintHQ)(BOOL bHQ) = 0;

  // Get whether the hardware decoder should force high-quality deinterlacing
  // Note: this option is not supported on all decoder implementations and/or all operating systems
  STDMETHOD_(BOOL, GetHWAccelDeintHQ)() = 0;

  // Set the software deinterlacing mode used
  STDMETHOD(SetSWDeintMode)(LAVSWDeintModes deintMode) = 0;

  // Get the software deinterlacing mode used
  STDMETHOD_(LAVSWDeintModes, GetSWDeintMode)() = 0;

  // Set the software deinterlacing output
  STDMETHOD(SetSWDeintOutput)(LAVDeintOutput deintOutput) = 0;

  // Get the software deinterlacing output
  STDMETHOD_(LAVDeintOutput, GetSWDeintOutput)() = 0;

  // DEPRECATED, use SetDeinterlacingMode
  STDMETHOD(SetDeintTreatAsProgressive)(BOOL bEnabled) = 0;

  // DEPRECATED, use GetDeinterlacingMode
  STDMETHOD_(BOOL, GetDeintTreatAsProgressive)() = 0;

  // Set the dithering mode used
  STDMETHOD(SetDitherMode)(LAVDitherMode ditherMode) = 0;

  // Get the dithering mode used
  STDMETHOD_(LAVDitherMode, GetDitherMode)() = 0;

  // Set if the MS WMV9 DMO Decoder should be used for VC-1/WMV3
  STDMETHOD(SetUseMSWMV9Decoder)(BOOL bEnabled) = 0;

  // Get if the MS WMV9 DMO Decoder should be used for VC-1/WMV3
  STDMETHOD_(BOOL, GetUseMSWMV9Decoder)() = 0;

  // Set if DVD Video support is enabled
  STDMETHOD(SetDVDVideoSupport)(BOOL bEnabled) = 0;

  // Get if DVD Video support is enabled
  STDMETHOD_(BOOL,GetDVDVideoSupport)() = 0;

  // Set the HW Accel Resolution Flags
  // flags: bitmask of LAVHWResFlag flags
  STDMETHOD(SetHWAccelResolutionFlags)(DWORD dwResFlags) = 0;

  // Get the HW Accel Resolution Flags
  // flags: bitmask of LAVHWResFlag flags
  STDMETHOD_(DWORD, GetHWAccelResolutionFlags)() = 0;

  // Toggle Tray Icon
  STDMETHOD(SetTrayIcon)(BOOL bEnabled) = 0;

  // Get Tray Icon
  STDMETHOD_(BOOL,GetTrayIcon)() = 0;

  // Set the Deint Mode
  STDMETHOD(SetDeinterlacingMode)(LAVDeintMode deintMode) = 0;

  // Get the Deint Mode
  STDMETHOD_(LAVDeintMode,GetDeinterlacingMode)() = 0;

  // Set the index of the GPU to be used for hardware decoding
  // Only supported for CUVID and DXVA2 copy-back. If the device is not valid, it'll fallback to auto-detection
  // Must be called before an input is connected to LAV Video, and the setting is non-persistent
  // NOTE: For CUVID, the index defines the index of the CUDA capable device, while for DXVA2, the list includes all D3D9 devices
  STDMETHOD(SetGPUDeviceIndex)(DWORD dwDevice) = 0;
};

// LAV Video status interface
[uuid("1CC2385F-36FA-41B1-9942-5024CE0235DC")]
interface ILAVVideoStatus : public IUnknown
{
  // Get the name of the active decoder (can return NULL if none is active)
  STDMETHOD_(LPCWSTR, GetActiveDecoderName)() = 0;
};
