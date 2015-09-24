/*
 *      Copyright (C) 2010-2015 Hendrik Leppkes
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

// {4158A22B-6553-45D0-8069-24716F8FF171}
DEFINE_GUID(IID_ILAVAudioSettings, 
0x4158a22b, 0x6553, 0x45d0, 0x80, 0x69, 0x24, 0x71, 0x6f, 0x8f, 0xf1, 0x71);

// {2d0a37bb-5645-46f8-8cc4-48de2a963ae6}
DEFINE_GUID(IID_ILAVAudioSettingsDSPlayerCustom, 
0x2d0a37bb, 0x5645, 0x46f8, 0x8c, 0xc4, 0x48, 0xde, 0x2a, 0x96, 0x3a, 0xe6);

// {A668B8F2-BA87-4F63-9D41-768F7DE9C50E}
DEFINE_GUID(IID_ILAVAudioStatus,
0xa668b8f2, 0xba87, 0x4f63, 0x9d, 0x41, 0x76, 0x8f, 0x7d, 0xe9, 0xc5, 0xe);

// Codecs supported in the LAV Audio configuration
// Codecs not listed here cannot be turned off. You can request codecs to be added to this list, if you wish.
typedef enum LAVAudioCodec {
  Codec_AAC,
  Codec_AC3,
  Codec_EAC3,
  Codec_DTS,
  Codec_MP2,
  Codec_MP3,
  Codec_TRUEHD,
  Codec_FLAC,
  Codec_VORBIS,
  Codec_LPCM,
  Codec_PCM,
  Codec_WAVPACK,
  Codec_TTA,
  Codec_WMA2,
  Codec_WMAPRO,
  Codec_Cook,
  Codec_RealAudio,
  Codec_WMALL,
  Codec_ALAC,
  Codec_Opus,
  Codec_AMR,
  Codec_Nellymoser,
  Codec_MSPCM,
  Codec_Truespeech,
  Codec_TAK,
  Codec_ATRAC,

  Codec_AudioNB            // Number of entries (do not use when dynamically linking)
} LAVAudioCodec;

// Bitstreaming Codecs supported in LAV Audio
typedef enum LAVBitstreamCodec {
  Bitstream_AC3,
  Bitstream_EAC3,
  Bitstream_TRUEHD,
  Bitstream_DTS,
  Bitstream_DTSHD,

  Bitstream_NB        // Number of entries (do not use when dynamically linking)
} LAVBitstreamCodec;


// Supported Sample Formats in LAV Audio
typedef enum LAVAudioSampleFormat {
  SampleFormat_None = -1,
  SampleFormat_16,
  SampleFormat_24,
  SampleFormat_32,
  SampleFormat_U8,
  SampleFormat_FP32,
  SampleFormat_Bitstream,

  SampleFormat_NB     // Number of entries (do not use when dynamically linking)
} LAVAudioSampleFormat;

typedef enum LAVAudioMixingMode {
  MatrixEncoding_None,
  MatrixEncoding_Dolby,
  MatrixEncoding_DPLII,

  MatrixEncoding_NB
} LAVAudioMixingMode;

// LAV Audio configuration interface
[uuid("4158A22B-6553-45D0-8069-24716F8FF171")]
interface ILAVAudioSettings : public IUnknown
{
  // Switch to Runtime Config mode. This will reset all settings to default, and no changes to the settings will be saved
  // You can use this to programmatically configure LAV Audio without interfering with the users settings in the registry.
  // Subsequent calls to this function will reset all settings back to defaults, even if the mode does not change.
  //
  // Note that calling this function during playback is not supported and may exhibit undocumented behaviour. 
  // For smooth operations, it must be called before LAV Audio is connected to other filters.
  STDMETHOD(SetRuntimeConfig)(BOOL bRuntimeConfig) = 0;

  // Dynamic Range Compression
  // pbDRCEnabled: The state of DRC
  // piDRCLevel:   The DRC strength (0-100, 100 is maximum)
  STDMETHOD(GetDRC)(BOOL *pbDRCEnabled, int *piDRCLevel) = 0;
  STDMETHOD(SetDRC)(BOOL bDRCEnabled, int iDRCLevel) = 0;
  
  // Configure which codecs are enabled
  // If aCodec is invalid (possibly a version difference), Get will return FALSE, and Set E_FAIL.
  STDMETHOD_(BOOL,GetFormatConfiguration)(LAVAudioCodec aCodec) = 0;
  STDMETHOD(SetFormatConfiguration)(LAVAudioCodec aCodec, BOOL bEnabled) = 0;

  // Control Bitstreaming
  // If bsCodec is invalid (possibly a version difference), Get will return FALSE, and Set E_FAIL.
  STDMETHOD_(BOOL, GetBitstreamConfig)(LAVBitstreamCodec bsCodec) = 0;
  STDMETHOD(SetBitstreamConfig)(LAVBitstreamCodec bsCodec, BOOL bEnabled) = 0;
  
  // Should "normal" DTS frames be encapsulated in DTS-HD frames when bitstreaming?
  STDMETHOD_(BOOL,GetDTSHDFraming)() = 0;
  STDMETHOD(SetDTSHDFraming)(BOOL bHDFraming) = 0;

  // Control Auto A/V syncing
  STDMETHOD_(BOOL,GetAutoAVSync)() = 0;
  STDMETHOD(SetAutoAVSync)(BOOL bAutoSync) = 0;

  // Convert all Channel Layouts to standard layouts
  // Standard are: Mono, Stereo, 5.1, 6.1, 7.1
  STDMETHOD_(BOOL,GetOutputStandardLayout)() = 0;
  STDMETHOD(SetOutputStandardLayout)(BOOL bStdLayout) = 0;
  
  // Expand Mono to Stereo by simply doubling the audio
  STDMETHOD_(BOOL,GetExpandMono)() = 0;
  STDMETHOD(SetExpandMono)(BOOL bExpandMono) = 0;

  // Expand 6.1 to 7.1 by doubling the back center
  STDMETHOD_(BOOL,GetExpand61)() = 0;
  STDMETHOD(SetExpand61)(BOOL bExpand61) = 0;

  // Allow Raw PCM and SPDIF encoded input
  STDMETHOD_(BOOL,GetAllowRawSPDIFInput)() = 0;
  STDMETHOD(SetAllowRawSPDIFInput)(BOOL bAllow) = 0;

  // Configure which sample formats are enabled
  // Note: SampleFormat_Bitstream cannot be controlled by this
  STDMETHOD_(BOOL,GetSampleFormat)(LAVAudioSampleFormat format) = 0;
  STDMETHOD(SetSampleFormat)(LAVAudioSampleFormat format, BOOL bEnabled) = 0;

  // Configure a delay for the audio
  STDMETHOD(GetAudioDelay)(BOOL *pbEnabled, int *pDelay) = 0;
  STDMETHOD(SetAudioDelay)(BOOL bEnabled, int delay) = 0;

  // Enable/Disable Mixing
  STDMETHOD(SetMixingEnabled)(BOOL bEnabled) = 0;
  STDMETHOD_(BOOL,GetMixingEnabled)() = 0;

  // Control Mixing Layout
  STDMETHOD(SetMixingLayout)(DWORD dwLayout) = 0;
  STDMETHOD_(DWORD,GetMixingLayout)() = 0;

#define LAV_MIXING_FLAG_UNTOUCHED_STEREO 0x0001
#define LAV_MIXING_FLAG_NORMALIZE_MATRIX 0x0002
#define LAV_MIXING_FLAG_CLIP_PROTECTION  0x0004
  // Set Mixing Flags
  STDMETHOD(SetMixingFlags)(DWORD dwFlags) = 0;
  STDMETHOD_(DWORD,GetMixingFlags)() = 0;

  // Set Mixing Mode
  STDMETHOD(SetMixingMode)(LAVAudioMixingMode mixingMode) = 0;
  STDMETHOD_(LAVAudioMixingMode,GetMixingMode)() = 0;

  // Set Mixing Levels
  STDMETHOD(SetMixingLevels)(DWORD dwCenterLevel, DWORD dwSurroundLevel, DWORD dwLFELevel) = 0;
  STDMETHOD(GetMixingLevels)(DWORD *dwCenterLevel, DWORD *dwSurroundLevel, DWORD *dwLFELevel) = 0;

  // Toggle Tray Icon
  STDMETHOD(SetTrayIcon)(BOOL bEnabled) = 0;
  STDMETHOD_(BOOL,GetTrayIcon)() = 0;

  // Toggle Dithering for sample format conversion
  STDMETHOD(SetSampleConvertDithering)(BOOL bEnabled) = 0;
  STDMETHOD_(BOOL,GetSampleConvertDithering)() = 0;

  // Suppress sample format changes. This will allow channel count to increase, but not to reduce, instead adding empty channels
  // This option is NOT persistent
  STDMETHOD(SetSuppressFormatChanges)(BOOL bEnabled) = 0;
  STDMETHOD_(BOOL, GetSuppressFormatChanges)() = 0;

  // Use 5.1 legacy layout (using back channels instead of side)
  STDMETHOD_(BOOL, GetOutput51LegacyLayout)() = 0;
  STDMETHOD(SetOutput51LegacyLayout)(BOOL b51Legacy) = 0;
};

[uuid("2d0a37bb-5645-46f8-8cc4-48de2a963ae6")]
interface ILAVAudioSettingsDSPlayerCustom : public IUnknown
{
  // Set a custom callback function to handle the property page
  STDMETHOD(SetPropertyPageCallback)(HRESULT(*fpPropPageCallback)(IUnknown* pFilter)) = 0;
};

// LAV Audio Status Interface
// Get the current playback stats
[uuid("A668B8F2-BA87-4F63-9D41-768F7DE9C50E")]
interface ILAVAudioStatus : public IUnknown
{
  // Check if the given sample format is supported by the current playback chain
  STDMETHOD_(BOOL,IsSampleFormatSupported)(LAVAudioSampleFormat sfCheck) = 0;

  // Get details about the current decoding format
  STDMETHOD(GetDecodeDetails)(LPCSTR *pCodec, LPCSTR *pDecodeFormat, int *pnChannels, int *pSampleRate, DWORD *pChannelMask) = 0;
  
  // Get details about the current output format
  STDMETHOD(GetOutputDetails)(LPCSTR *pOutputFormat, int *pnChannels, int *pSampleRate, DWORD *pChannelMask) = 0;
  
  // Enable Volume measurements
  STDMETHOD(EnableVolumeStats)() = 0;

  // Disable Volume measurements
  STDMETHOD(DisableVolumeStats)() = 0;

  // Get Volume Average for the given channel
  STDMETHOD(GetChannelVolumeAverage)(WORD nChannel, float *pfDb) = 0;
};
