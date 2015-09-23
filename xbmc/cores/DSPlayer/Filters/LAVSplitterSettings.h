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

#include <Unknwn.h>       // IUnknown and GUID Macros

// {774A919D-EA95-4A87-8A1E-F48ABE8499C7}
DEFINE_GUID(IID_ILAVFSettings, 
0x774a919d, 0xea95, 0x4a87, 0x8a, 0x1e, 0xf4, 0x8a, 0xbe, 0x84, 0x99, 0xc7);

typedef enum LAVSubtitleMode {
  LAVSubtitleMode_NoSubs,
  LAVSubtitleMode_ForcedOnly,
  LAVSubtitleMode_Default,
  LAVSubtitleMode_Advanced
} LAVSubtitleMode;

[uuid("774A919D-EA95-4A87-8A1E-F48ABE8499C7")]
interface ILAVFSettings : public IUnknown
{
  // Switch to Runtime Config mode. This will reset all settings to default, and no changes to the settings will be saved
  // You can use this to programmatically configure LAV Splitter without interfering with the users settings in the registry.
  // Subsequent calls to this function will reset all settings back to defaults, even if the mode does not change.
  //
  // Note that calling this function during playback is not supported and may exhibit undocumented behaviour. 
  // For smooth operations, it must be called before LAV Splitter opens a file.
  STDMETHOD(SetRuntimeConfig)(BOOL bRuntimeConfig) = 0;

  // Retrieve the preferred languages as ISO 639-2 language codes, comma separated
  // If the result is NULL, no language has been set
  // Memory for the string will be allocated, and has to be free'ed by the caller with CoTaskMemFree
  STDMETHOD(GetPreferredLanguages)(LPWSTR *ppLanguages) = 0;

  // Set the preferred languages as ISO 639-2 language codes, comma separated
  // To reset to no preferred language, pass NULL or the empty string
  STDMETHOD(SetPreferredLanguages)(LPCWSTR pLanguages) = 0;
  
  // Retrieve the preferred subtitle languages as ISO 639-2 language codes, comma separated
  // If the result is NULL, no language has been set
  // If no subtitle language is set, the main language preference is used.
  // Memory for the string will be allocated, and has to be free'ed by the caller with CoTaskMemFree
  STDMETHOD(GetPreferredSubtitleLanguages)(LPWSTR *ppLanguages) = 0;

  // Set the preferred subtitle languages as ISO 639-2 language codes, comma separated
  // To reset to no preferred language, pass NULL or the empty string
  // If no subtitle language is set, the main language preference is used.
  STDMETHOD(SetPreferredSubtitleLanguages)(LPCWSTR pLanguages) = 0;

  // Get the current subtitle mode
  // See enum for possible values
  STDMETHOD_(LAVSubtitleMode,GetSubtitleMode)() = 0;

  // Set the current subtitle mode
  // See enum for possible values
  STDMETHOD(SetSubtitleMode)(LAVSubtitleMode mode) = 0;

  // Get the subtitle matching language flag
  // TRUE = Only subtitles with a language in the preferred list will be used; FALSE = All subtitles will be used
  // @deprecated - do not use anymore, deprecated and non-functional, replaced by advanced subtitle mode
  STDMETHOD_(BOOL,GetSubtitleMatchingLanguage)() = 0;

  // Set the subtitle matching language flag
  // TRUE = Only subtitles with a language in the preferred list will be used; FALSE = All subtitles will be used
  // @deprecated - do not use anymore, deprecated and non-functional, replaced by advanced subtitle mode
  STDMETHOD(SetSubtitleMatchingLanguage)(BOOL dwMode) = 0;

  // Control whether a special "Forced Subtitles" stream will be created for PGS subs
  STDMETHOD_(BOOL,GetPGSForcedStream)() = 0;

  // Control whether a special "Forced Subtitles" stream will be created for PGS subs
  STDMETHOD(SetPGSForcedStream)(BOOL bFlag) = 0;

  // Get the PGS forced subs config
  // TRUE = only forced PGS frames will be shown, FALSE = all frames will be shown
  STDMETHOD_(BOOL,GetPGSOnlyForced)() = 0;

  // Set the PGS forced subs config
  // TRUE = only forced PGS frames will be shown, FALSE = all frames will be shown
  STDMETHOD(SetPGSOnlyForced)(BOOL bForced) = 0;

  // Get the VC-1 Timestamp Processing mode
  // 0 - No Timestamp Correction, 1 - Always Timestamp Correction, 2 - Auto (Correction for Decoders that need it)
  STDMETHOD_(int,GetVC1TimestampMode)() = 0;
  
  // Set the VC-1 Timestamp Processing mode
  // 0 - No Timestamp Correction, 1 - Always Timestamp Correction, 2 - Auto (Correction for Decoders that need it)
  STDMETHOD(SetVC1TimestampMode)(int iMode) = 0;

  // Set whether substreams (AC3 in TrueHD, for example) should be shown as a separate stream
  STDMETHOD(SetSubstreamsEnabled)(BOOL bSubStreams) = 0;

  // Check whether substreams (AC3 in TrueHD, for example) should be shown as a separate stream
  STDMETHOD_(BOOL,GetSubstreamsEnabled)() = 0;

  // @deprecated - no longer required
  STDMETHOD(SetVideoParsingEnabled)(BOOL bEnabled) = 0;
  
  // @deprecated - no longer required
  STDMETHOD_(BOOL,GetVideoParsingEnabled)() = 0;

  // Set if LAV Splitter should try to fix broken HD-PVR streams
  // @deprecated - no longer required
  STDMETHOD(SetFixBrokenHDPVR)(BOOL bEnabled) = 0;

  // Query if LAV Splitter should try to fix broken HD-PVR streams
  // @deprecated - no longer required
  STDMETHOD_(BOOL,GetFixBrokenHDPVR)() = 0;

  // Control whether the given format is enabled
  STDMETHOD_(HRESULT,SetFormatEnabled)(LPCSTR strFormat, BOOL bEnabled) = 0;

  // Check if the given format is enabled
  STDMETHOD_(BOOL,IsFormatEnabled)(LPCSTR strFormat) = 0;

  // Set if LAV Splitter should always completely remove the filter connected to its Audio Pin when the audio stream is changed
  STDMETHOD(SetStreamSwitchRemoveAudio)(BOOL bEnabled) = 0;

  // Query if LAV Splitter should always completely remove the filter connected to its Audio Pin when the audio stream is changed
  STDMETHOD_(BOOL,GetStreamSwitchRemoveAudio)() = 0;

  // Advanced Subtitle configuration. Refer to the documentation for details.
  // If no advanced config exists, will be NULL.
  // Memory for the string will be allocated, and has to be free'ed by the caller with CoTaskMemFree
  STDMETHOD(GetAdvancedSubtitleConfig)(LPWSTR *ppAdvancedConfig) = 0;

  // Advanced Subtitle configuration. Refer to the documentation for details.
  // To reset the config, pass NULL or the empty string.
  // If no subtitle language is set, the main language preference is used.
  STDMETHOD(SetAdvancedSubtitleConfig)(LPCWSTR pAdvancedConfig) = 0;

  // Set if LAV Splitter should prefer audio streams for the hearing or visually impaired
  STDMETHOD(SetUseAudioForHearingVisuallyImpaired)(BOOL bEnabled) = 0;

  // Get if LAV Splitter should prefer audio streams for the hearing or visually impaired
  STDMETHOD_(BOOL,GetUseAudioForHearingVisuallyImpaired)() = 0;

  // Set the maximum queue size, in megabytes
  STDMETHOD(SetMaxQueueMemSize)(DWORD dwMaxSize) = 0;

  // Get the maximum queue size, in megabytes
  STDMETHOD_(DWORD,GetMaxQueueMemSize)() = 0;

  // Toggle Tray Icon
  STDMETHOD(SetTrayIcon)(BOOL bEnabled) = 0;

  // Get Tray Icon
  STDMETHOD_(BOOL,GetTrayIcon)() = 0;

  // Toggle whether higher quality audio streams are preferred
  STDMETHOD(SetPreferHighQualityAudioStreams)(BOOL bEnabled) = 0;

  // Toggle whether higher quality audio streams are preferred
  STDMETHOD_(BOOL,GetPreferHighQualityAudioStreams)() = 0;

  // Toggle whether Matroska Linked Segments should be loaded from other files
  STDMETHOD(SetLoadMatroskaExternalSegments)(BOOL bEnabled) = 0;

  // Get whether Matroska Linked Segments should be loaded from other files
  STDMETHOD_(BOOL,GetLoadMatroskaExternalSegments)() = 0;

  // Get the list of available formats
  // Memory for the string array will be allocated, and has to be free'ed by the caller with CoTaskMemFree
  STDMETHOD(GetFormats)(LPSTR** formats, UINT* nFormats) = 0;

  // Set the duration (in ms) of analysis for network streams (to find the streams and codec parameters)
  STDMETHOD(SetNetworkStreamAnalysisDuration)(DWORD dwDuration) = 0;

  // Get the duration (in ms) of analysis for network streams (to find the streams and codec parameters)
  STDMETHOD_(DWORD, GetNetworkStreamAnalysisDuration)() = 0;

  // Set the maximum queue size, in number of packets
  STDMETHOD(SetMaxQueueSize)(DWORD dwMaxSize) = 0;

  // Get the maximum queue size, in number of packets
  STDMETHOD_(DWORD, GetMaxQueueSize)() = 0;
};
