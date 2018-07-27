/*
 *  Copyright (c) 2002 d7o3g4q and RUNTiME
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "platform/linux/PlatformDefs.h"
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
  unsigned short  Data2, Data3;
  unsigned char  Data4[8];
} __attribute__((__packed__)) GUID;

// Audio stuff
typedef struct tWAVEFORMATEX
{
  unsigned short wFormatTag;
  unsigned short nChannels;
  DWORD   nSamplesPerSec;
  DWORD   nAvgBytesPerSec;
  unsigned short nBlockAlign;
  unsigned short wBitsPerSample;
  unsigned short cbSize;
 } __attribute__((__packed__)) WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;

 #define WAVE_FORMAT_UNKNOWN           0x0000
 #define WAVE_FORMAT_PCM               0x0001
 #define WAVE_FORMAT_ADPCM             0x0002
 #define WAVE_FORMAT_IEEE_FLOAT        0x0003
 #define WAVE_FORMAT_EXTENSIBLE        0xFFFE

typedef struct tWAVEFORMATEXTENSIBLE
{
  WAVEFORMATEX Format;
  union
  {
    unsigned short wValidBitsPerSample;
    unsigned short wSamplesPerBlock;
    unsigned short wReserved;
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
  COMXAudio(CProcessInfo &processInfo);
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
  CProcessInfo&     m_processInfo;
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

  mutable CCriticalSection m_critSection;
};
