/*
* XBMC Media Center
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

//////////////////////////////////////////////////////////////////////

#ifndef __OPENMAXAUDIORENDER_H__
#define __OPENMAXAUDIORENDER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cores/AudioEngine/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Utils/AERemap.h"
#include "cores/IAudioCallback.h"
#include "linux/PlatformDefs.h"
#include "DVDStreamInfo.h"

#include "OMXClock.h"
#include "OMXCore.h"
#include "DllAvCodec.h"
#include "DllAvUtil.h"

#include "threads/CriticalSection.h"

#define AUDIO_BUFFER_SECONDS 2
#define VIS_PACKET_SIZE 512

#define OMX_IS_RAW(x)       \
(                           \
  (x) == AE_FMT_AC3   ||    \
  (x) == AE_FMT_DTS         \
)

class COMXAudio
{
public:
  void UnRegisterAudioCallback();
  void RegisterAudioCallback(IAudioCallback* pCallback);
  unsigned int GetChunkLen();
  float GetDelay();
  float GetCacheTime();
  float GetCacheTotal();
  COMXAudio();
  bool Initialize(AEAudioFormat format, std::string& device, OMXClock *clock, CDVDStreamInfo &hints, bool bUsePassthrough, bool bUseHWDecode);
  bool Initialize(AEAudioFormat format, std::string& device);
  ~COMXAudio();

  unsigned int AddPackets(const void* data, unsigned int len);
  unsigned int AddPackets(const void* data, unsigned int len, double dts, double pts);
  unsigned int GetSpace();
  bool Deinitialize();
  bool Pause();
  bool Stop();
  bool Resume();

  long GetCurrentVolume() const;
  void Mute(bool bMute);
  bool SetCurrentVolume(float fVolume);
  void SetDynamicRangeCompression(long drc) { m_drc = drc; }
  int SetPlaySpeed(int iSpeed);
  void WaitCompletion();
  void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);

  void Flush();

  void Process();

  bool SetClock(OMXClock *clock);
  void SetCodingType(AEDataFormat dataFormat);
  static bool CanHWDecode(CodecID codec);

  void PrintChannels(OMX_AUDIO_CHANNELTYPE eChannelMapping[]);
  void PrintPCM(OMX_AUDIO_PARAM_PCMMODETYPE *pcm, std::string direction);
  void PrintDDP(OMX_AUDIO_PARAM_DDPTYPE *ddparm);
  void PrintDTS(OMX_AUDIO_PARAM_DTSTYPE *dtsparam);
  unsigned int SyncDTS(BYTE* pData, unsigned int iSize);
  unsigned int SyncAC3(BYTE* pData, unsigned int iSize);

private:
  IAudioCallback* m_pCallback;
  bool          m_Initialized;
  bool          m_Pause;
  bool          m_CanPause;
  float         m_CurrentVolume;
  long          m_drc;
  bool          m_Passthrough;
  bool          m_HWDecode;
  unsigned int  m_BytesPerSec;
  unsigned int  m_BufferLen;
  unsigned int  m_ChunkLen;
  unsigned int  m_OutputChannels;
  unsigned int  m_BitsPerSample;
  COMXCoreComponent *m_omx_clock;
  OMXClock       *m_av_clock;
  bool          m_external_clock;
  bool          m_first_frame;
  bool          m_LostSync;
  int           m_SampleRate;
  OMX_AUDIO_CODINGTYPE m_eEncoding;
  uint8_t       *m_extradata;
  int           m_extrasize;
  // stuff for visualisation
  unsigned int  m_vizBufferSamples;
  double        m_last_pts;
  int           m_vizBufferSize;
  uint8_t       *m_vizBuffer;
  int           m_vizRemapBufferSize;
  uint8_t       *m_vizRemapBuffer;
  CAERemap      m_vizRemap;

  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_output;
  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_input;
  OMX_AUDIO_PARAM_DTSTYPE     m_dtsParam;
  WAVEFORMATEXTENSIBLE        m_wave_header;
  AEAudioFormat m_format;
protected:
  COMXCoreComponent *m_omx_render;
  COMXCoreComponent m_omx_mixer;
  COMXCoreComponent m_omx_decoder;
  COMXCoreTunel     m_omx_tunnel_clock;
  COMXCoreTunel     m_omx_tunnel_mixer;
  COMXCoreTunel     m_omx_tunnel_decoder;
  DllAvUtil         m_dllAvUtil;

  OMX_AUDIO_CHANNELTYPE m_input_channels[OMX_AUDIO_MAXCHANNELS];
  OMX_AUDIO_CHANNELTYPE m_output_channels[OMX_AUDIO_MAXCHANNELS];

  CAEChannelInfo    m_channelLayout;

  CAEChannelInfo    GetChannelLayout(AEAudioFormat format);

  void CheckOutputBufferSize(void **buffer, int *oldSize, int newSize);
  unsigned int GetAudioRenderingLatency();
  CCriticalSection m_critSection;
};
#endif

