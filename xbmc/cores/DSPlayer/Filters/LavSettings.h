/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
// MadvrSettings.h: interface for the CMadvrSettings class.
//
//////////////////////////////////////////////////////////////////////

//#if !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
//#define AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#pragma once

#include "Filters/LAVAudioSettings.h"
#include "Filters/LAVVideoSettings.h"
#include "Filters/LAVSplitterSettings.h"

class CLavSettings
{
public:
  CLavSettings();
  ~CLavSettings() {};

  BOOL video_bTrayIcon;
  DWORD video_dwStreamAR;
  DWORD video_dwNumThreads;
  BOOL video_bPixFmts[LAVOutPixFmt_NB];
  DWORD video_dwRGBRange;
  DWORD video_dwHWAccel;
  BOOL video_bHWFormats[HWCodec_NB];
  DWORD video_dwHWAccelResFlags;
  DWORD video_dwHWDeintMode;
  DWORD video_dwHWDeintOutput;
  BOOL video_bHWDeintHQ;
  DWORD video_dwDeintFieldOrder;
  LAVDeintMode video_deintMode;
  DWORD video_dwSWDeintMode;
  DWORD video_dwSWDeintOutput;
  DWORD video_dwDitherMode;

  BOOL audio_bTrayIcon;
  BOOL audio_bDRCEnabled;
  int audio_iDRCLevel;
  BOOL audio_bBitstream[Bitstream_NB];
  BOOL audio_bDTSHDFraming;
  BOOL audio_bAutoAVSync;
  BOOL audio_bExpandMono;
  BOOL audio_bExpand61;
  BOOL audio_bOutputStandardLayout;
  BOOL audio_bAllowRawSPDIF;
  BOOL audio_bSampleFormats[SampleFormat_NB];
  BOOL audio_bSampleConvertDither;
  BOOL audio_bAudioDelayEnabled;
  int  audio_iAudioDelay;
  BOOL audio_bMixingEnabled;
  DWORD audio_dwMixingLayout;
  DWORD audio_dwMixingFlags;
  DWORD audio_dwMixingMode;
  DWORD audio_dwMixingCenterLevel;
  DWORD audio_dwMixingSurroundLevel;
  DWORD audio_dwMixingLFELevel;

  BOOL splitter_bTrayIcon;
  std::wstring splitter_prefAudioLangs;
  std::wstring splitter_prefSubLangs;
  std::wstring splitter_subtitleAdvanced;
  LAVSubtitleMode splitter_subtitleMode;
  BOOL splitter_bPGSForcedStream;
  BOOL splitter_bPGSOnlyForced;
  int splitter_iVC1Mode;
  BOOL splitter_bSubstreams;
  BOOL splitter_bMatroskaExternalSegments;
  BOOL splitter_bStreamSwitchRemoveAudio;
  BOOL splitter_bImpairedAudio;
  BOOL splitter_bPreferHighQualityAudio;
  DWORD splitter_dwQueueMaxSize;
  DWORD splitter_dwNetworkAnalysisDuration;

private:
};

//#endif // !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
