#pragma once

/*
 *      Copyright (c) 2002 d7o3g4q and RUNTiME
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "linux/PlatformDefs.h"
#include "DVDStreamInfo.h"

#include "OMXClock.h"
#include "OMXCore.h"

#include "threads/CriticalSection.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}

#define AUDIO_BUFFER_SECONDS 3
#define VIS_PACKET_SIZE 512

typedef struct tGUID
{
  DWORD Data1;
  WORD  Data2, Data3;
  BYTE  Data4[8];
} __attribute__((__packed__)) GUID;

typedef struct tWAVEFORMATEXTENSIBLE
{
  WAVEFORMATEX Format;
  union
  {
    WORD wValidBitsPerSample;
    WORD wSamplesPerBlock;
    WORD wReserved;
  } Samples;
  DWORD dwChannelMask;
  GUID SubFormat;
} __attribute__((__packed__)) WAVEFORMATEXTENSIBLE;

class COMXAudio
{
public:
  unsigned int GetChunkLen();
  float GetDelay();
  float GetCacheTime();
  float GetCacheTotal();
  COMXAudio();
  bool Initialize(AEAudioFormat format, OMXClock *clock, CDVDStreamInfo &hints, CAEChannelInfo channelMap, bool bUsePassthrough);
  bool PortSettingsChanged();
  ~COMXAudio();

  unsigned int AddPackets(const void* data, unsigned int len, double dts, double pts, unsigned int frame_size, bool &settings_changed);
  unsigned int GetSpace();
  bool Deinitialize();

  void SetVolume(float nVolume);
  void SetMute(bool bOnOff);
  void SetDynamicRangeCompression(long drc);
  float GetDynamicRangeAmplification() const { return 20.0f * log10f(m_amplification * m_attenuation); }
  bool ApplyVolume();
  int SetPlaySpeed(int iSpeed);
  void SubmitEOS();
  bool IsEOS();
  void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);

  void Flush();

  void Process();

  void SetCodingType(AEAudioFormat format);

  static void PrintChannels(OMX_AUDIO_CHANNELTYPE eChannelMapping[]);
  void PrintPCM(OMX_AUDIO_PARAM_PCMMODETYPE *pcm, std::string direction);
  void UpdateAttenuation();

  bool BadState() const { return !m_Initialized; };
  unsigned int GetAudioRenderingLatency() const;
  float GetMaxLevel(double &pts);

private:
  bool          m_Initialized;
  float         m_CurrentVolume;
  bool          m_Mute;
  long          m_drc;
  bool          m_Passthrough;
  unsigned int  m_BytesPerSec;
  unsigned int  m_InputBytesPerSec;
  unsigned int  m_BufferLen;
  unsigned int  m_ChunkLen;
  unsigned int  m_InputChannels;
  unsigned int  m_OutputChannels;
  unsigned int  m_BitsPerSample;
  float         m_maxLevel;
  float         m_amplification;
  float         m_attenuation;
  float         m_submitted;
  COMXCoreComponent *m_omx_clock;
  OMXClock       *m_av_clock;
  bool          m_settings_changed;
  bool          m_setStartTime;
  int           m_SampleRate;
  OMX_AUDIO_CODINGTYPE m_eEncoding;
  uint8_t       *m_extradata;
  int           m_extrasize;
  // stuff for visualisation
  double        m_last_pts;
  bool          m_submitted_eos;
  bool          m_failed_eos;
  enum { AESINKPI_UNKNOWN, AESINKPI_HDMI, AESINKPI_ANALOGUE, AESINKPI_BOTH } m_output;

  typedef struct {
    double pts;
    float level;
  } amplitudes_t;
  std::deque<amplitudes_t> m_ampqueue;

  float m_downmix_matrix[OMX_AUDIO_MAXCHANNELS*OMX_AUDIO_MAXCHANNELS];

  OMX_AUDIO_CHANNELTYPE m_input_channels[OMX_AUDIO_MAXCHANNELS];
  OMX_AUDIO_CHANNELTYPE m_output_channels[OMX_AUDIO_MAXCHANNELS];
  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_output;
  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_input;
  OMX_AUDIO_PARAM_DTSTYPE     m_dtsParam;
  WAVEFORMATEXTENSIBLE        m_wave_header;
  IAEStream *m_pAudioStream;
protected:
  COMXCoreComponent m_omx_render_analog;
  COMXCoreComponent m_omx_render_hdmi;
  COMXCoreComponent m_omx_splitter;
  COMXCoreComponent m_omx_mixer;
  COMXCoreComponent m_omx_decoder;
  COMXCoreTunnel    m_omx_tunnel_clock_analog;
  COMXCoreTunnel    m_omx_tunnel_clock_hdmi;
  COMXCoreTunnel    m_omx_tunnel_mixer;
  COMXCoreTunnel    m_omx_tunnel_decoder;
  COMXCoreTunnel    m_omx_tunnel_splitter_analog;
  COMXCoreTunnel    m_omx_tunnel_splitter_hdmi;

  CCriticalSection m_critSection;
};

