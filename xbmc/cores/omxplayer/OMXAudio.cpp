/*
 *      Copyright (c) 2002 d7o3g4q and RUNTiME
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2012-2013 Team XBMC
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

#if defined(TARGET_WINDOWS)
#include "system.h"
#endif

#include "OMXAudio.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "linux/RBP.h"

#define CLASSNAME "COMXAudio"

#include "linux/XMemUtils.h"

#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
//#include "cores/AudioEngine/AEFactory.h"
#include "Util.h"
#include <algorithm>
#include <cassert>

extern "C" {
#include "libavutil/crc.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
}

// the size of the audio_render output port buffers
#define AUDIO_DECODE_OUTPUT_BUFFER (32*1024)
static const char rounded_up_channels_shift[] = {0,0,1,2,2,3,3,3,3};

static const GUID KSDATAFORMAT_SUBTYPE_PCM = {
  WAVE_FORMAT_PCM,
  0x0000, 0x0010,
  {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
COMXAudio::COMXAudio() :
  m_Initialized     (false  ),
  m_CurrentVolume   (0      ),
  m_Mute            (false  ),
  m_drc             (0      ),
  m_Passthrough     (false  ),
  m_BytesPerSec     (0      ),
  m_InputBytesPerSec(0      ),
  m_BufferLen       (0      ),
  m_ChunkLen        (0      ),
  m_InputChannels   (0      ),
  m_OutputChannels  (0      ),
  m_BitsPerSample   (0      ),
  m_maxLevel        (0.0f   ),
  m_amplification   (1.0f   ),
  m_attenuation     (1.0f   ),
  m_submitted       (0.0f   ),
  m_omx_clock       (NULL   ),
  m_av_clock        (NULL   ),
  m_settings_changed(false  ),
  m_setStartTime    (false  ),
  m_SampleRate      (0      ),
  m_eEncoding       (OMX_AUDIO_CodingPCM),
  m_extradata       (NULL   ),
  m_extrasize       (0      ),
  m_last_pts        (DVD_NOPTS_VALUE),
  m_submitted_eos   (false  ),
  m_failed_eos      (false  ),
  m_output          (AESINKPI_UNKNOWN)
{
  //CAEFactory::Suspend();
  //while (!CAEFactory::IsSuspended())
  //  Sleep(10);
}

COMXAudio::~COMXAudio()
{
  Deinitialize();

  //CAEFactory::Resume();
}

bool COMXAudio::PortSettingsChanged()
{
  CSingleLock lock (m_critSection);
  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;

  if (m_settings_changed)
  {
    m_omx_decoder.DisablePort(m_omx_decoder.GetOutputPort(), true);
    m_omx_decoder.EnablePort(m_omx_decoder.GetOutputPort(), true);
    return true;
  }

  if(!m_Passthrough)
  {
    if(!m_omx_mixer.Initialize("OMX.broadcom.audio_mixer", OMX_IndexParamAudioInit))
      return false;
  }
  if(m_output == AESINKPI_BOTH)
  {
    if(!m_omx_splitter.Initialize("OMX.broadcom.audio_splitter", OMX_IndexParamAudioInit))
      return false;
  }
  if (m_output == AESINKPI_BOTH || m_output == AESINKPI_ANALOGUE)
  {
    if(!m_omx_render_analog.Initialize("OMX.broadcom.audio_render", OMX_IndexParamAudioInit))
      return false;
  }
  if (m_output == AESINKPI_BOTH || m_output != AESINKPI_ANALOGUE)
  {
    if(!m_omx_render_hdmi.Initialize("OMX.broadcom.audio_render", OMX_IndexParamAudioInit))
      return false;
  }

  SetDynamicRangeCompression((long)(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_VolumeAmplification * 100));
  UpdateAttenuation();

  if( m_omx_mixer.IsInitialized() )
  {
    /* setup mixer output */
    OMX_INIT_STRUCTURE(m_pcm_output);
    m_pcm_output.nPortIndex = m_omx_decoder.GetOutputPort();
    omx_err = m_omx_decoder.GetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - error m_omx_decoder GetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }

    memcpy(m_pcm_output.eChannelMapping, m_output_channels, sizeof(m_output_channels));
    // round up to power of 2
    m_pcm_output.nChannels = m_OutputChannels > 4 ? 8 : m_OutputChannels > 2 ? 4 : m_OutputChannels;
    /* limit samplerate (through resampling) if requested */
    m_pcm_output.nSamplingRate = std::min(std::max((int)m_pcm_output.nSamplingRate, 8000), CServiceBroker::GetSettings().GetInt(CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE));

    m_pcm_output.nPortIndex = m_omx_mixer.GetOutputPort();
    omx_err = m_omx_mixer.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - error m_omx_mixer SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }

    CLog::Log(LOGDEBUG, "%s::%s - Output bps %d samplerate %d channels %d buffer size %d bytes per second %d",
        CLASSNAME, __func__, (int)m_pcm_output.nBitPerSample, (int)m_pcm_output.nSamplingRate, (int)m_pcm_output.nChannels, m_BufferLen, m_BytesPerSec);
    PrintPCM(&m_pcm_output, std::string("output"));

    if( m_omx_splitter.IsInitialized() )
    {
      m_pcm_output.nPortIndex = m_omx_splitter.GetInputPort();
      omx_err = m_omx_splitter.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
      if(omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s - error m_omx_splitter SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
        return false;
      }

      m_pcm_output.nPortIndex = m_omx_splitter.GetOutputPort();
      omx_err = m_omx_splitter.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
      if(omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s - error m_omx_splitter SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
        return false;
      }
      m_pcm_output.nPortIndex = m_omx_splitter.GetOutputPort() + 1;
      omx_err = m_omx_splitter.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
      if(omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s - error m_omx_splitter SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
        return false;
      }
    }

    if( m_omx_render_analog.IsInitialized() )
    {
      m_pcm_output.nPortIndex = m_omx_render_analog.GetInputPort();
      omx_err = m_omx_render_analog.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
      if(omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s - error m_omx_render_analog SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
        return false;
      }
    }

    if( m_omx_render_hdmi.IsInitialized() )
    {
      m_pcm_output.nPortIndex = m_omx_render_hdmi.GetInputPort();
      omx_err = m_omx_render_hdmi.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
      if(omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s - error m_omx_render_hdmi SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
        return false;
      }
    }
  }
  if( m_omx_render_analog.IsInitialized() )
  {
    m_omx_tunnel_clock_analog.Initialize(m_omx_clock, m_omx_clock->GetInputPort(),
      &m_omx_render_analog, m_omx_render_analog.GetInputPort()+1);

    omx_err = m_omx_tunnel_clock_analog.Establish();
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_clock_analog.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
    m_omx_render_analog.ResetEos();
  }
  if( m_omx_render_hdmi.IsInitialized() )
  {
    m_omx_tunnel_clock_hdmi.Initialize(m_omx_clock, m_omx_clock->GetInputPort() + (m_omx_render_analog.IsInitialized() ? 2 : 0),
      &m_omx_render_hdmi, m_omx_render_hdmi.GetInputPort()+1);

    omx_err = m_omx_tunnel_clock_hdmi.Establish();
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_clock_hdmi.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
    m_omx_render_hdmi.ResetEos();
  }

  if( m_omx_render_analog.IsInitialized() )
  {
    // By default audio_render is the clock master, and if output samples don't fit the timestamps, it will speed up/slow down the clock.
    // This tends to be better for maintaining audio sync and avoiding audio glitches, but can affect video/display sync
    if(CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK) || m_output == AESINKPI_BOTH)
    {
      OMX_CONFIG_BOOLEANTYPE configBool;
      OMX_INIT_STRUCTURE(configBool);
      configBool.bEnabled = OMX_FALSE;

      omx_err = m_omx_render_analog.SetConfig(OMX_IndexConfigBrcmClockReferenceSource, &configBool);
      if (omx_err != OMX_ErrorNone)
         return false;
    }

    OMX_CONFIG_BRCMAUDIODESTINATIONTYPE audioDest;
    OMX_INIT_STRUCTURE(audioDest);
    strncpy((char *)audioDest.sName, "local", strlen("local"));
    omx_err = m_omx_render_analog.SetConfig(OMX_IndexConfigBrcmAudioDestination, &audioDest);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_render_analog.SetConfig omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  if( m_omx_render_hdmi.IsInitialized() )
  {
    // By default audio_render is the clock master, and if output samples don't fit the timestamps, it will speed up/slow down the clock.
    // This tends to be better for maintaining audio sync and avoiding audio glitches, but can affect video/display sync
    if(CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK))
    {
      OMX_CONFIG_BOOLEANTYPE configBool;
      OMX_INIT_STRUCTURE(configBool);
      configBool.bEnabled = OMX_FALSE;

      omx_err = m_omx_render_hdmi.SetConfig(OMX_IndexConfigBrcmClockReferenceSource, &configBool);
      if (omx_err != OMX_ErrorNone)
         return false;
    }

    OMX_CONFIG_BRCMAUDIODESTINATIONTYPE audioDest;
    OMX_INIT_STRUCTURE(audioDest);
    strncpy((char *)audioDest.sName, "hdmi", strlen("hdmi"));
    omx_err = m_omx_render_hdmi.SetConfig(OMX_IndexConfigBrcmAudioDestination, &audioDest);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_render_hdmi.SetConfig omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  if( m_omx_splitter.IsInitialized() )
  {
    m_omx_tunnel_splitter_analog.Initialize(&m_omx_splitter, m_omx_splitter.GetOutputPort(), &m_omx_render_analog, m_omx_render_analog.GetInputPort());
    omx_err = m_omx_tunnel_splitter_analog.Establish();
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_splitter_analog.Establish 0x%08x", omx_err);
      return false;
    }

    m_omx_tunnel_splitter_hdmi.Initialize(&m_omx_splitter, m_omx_splitter.GetOutputPort() + 1, &m_omx_render_hdmi, m_omx_render_hdmi.GetInputPort());
    omx_err = m_omx_tunnel_splitter_hdmi.Establish();
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_splitter_hdmi.Establish 0x%08x", omx_err);
      return false;
    }
  }
  if( m_omx_mixer.IsInitialized() )
  {
    m_omx_tunnel_decoder.Initialize(&m_omx_decoder, m_omx_decoder.GetOutputPort(), &m_omx_mixer, m_omx_mixer.GetInputPort());
    if( m_omx_splitter.IsInitialized() )
    {
      m_omx_tunnel_mixer.Initialize(&m_omx_mixer, m_omx_mixer.GetOutputPort(), &m_omx_splitter, m_omx_splitter.GetInputPort());
    }
    else
    {
      if( m_omx_render_analog.IsInitialized() )
      {
        m_omx_tunnel_mixer.Initialize(&m_omx_mixer, m_omx_mixer.GetOutputPort(), &m_omx_render_analog, m_omx_render_analog.GetInputPort());
      }
      if( m_omx_render_hdmi.IsInitialized() )
      {
        m_omx_tunnel_mixer.Initialize(&m_omx_mixer, m_omx_mixer.GetOutputPort(), &m_omx_render_hdmi, m_omx_render_hdmi.GetInputPort());
      }
    }
    CLog::Log(LOGDEBUG, "%s::%s - bits:%d mode:%d channels:%d srate:%d nopassthrough", CLASSNAME, __func__,
            (int)m_pcm_input.nBitPerSample, m_pcm_input.ePCMMode, (int)m_pcm_input.nChannels, (int)m_pcm_input.nSamplingRate);
  }
  else
  {
    if( m_omx_render_analog.IsInitialized() )
    {
      m_omx_tunnel_decoder.Initialize(&m_omx_decoder, m_omx_decoder.GetOutputPort(), &m_omx_render_analog, m_omx_render_analog.GetInputPort());
    }
    else if( m_omx_render_hdmi.IsInitialized() )
    {
      m_omx_tunnel_decoder.Initialize(&m_omx_decoder, m_omx_decoder.GetOutputPort(), &m_omx_render_hdmi, m_omx_render_hdmi.GetInputPort());
    }
    CLog::Log(LOGDEBUG, "%s::%s - bits:%d mode:%d channels:%d srate:%d passthrough", CLASSNAME, __func__,
            0, 0, 0, 0);
  }

  omx_err = m_omx_tunnel_decoder.Establish();
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_decoder.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    return false;
  }

  if( m_omx_mixer.IsInitialized() )
  {
    omx_err = m_omx_mixer.SetStateForComponent(OMX_StateExecuting);
    if(omx_err != OMX_ErrorNone) {
      CLog::Log(LOGERROR, "%s::%s - m_omx_mixer OMX_StateExecuting omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  if( m_omx_mixer.IsInitialized() )
  {
    omx_err = m_omx_tunnel_mixer.Establish();
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_decoder.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  if( m_omx_splitter.IsInitialized() )
  {
    omx_err = m_omx_splitter.SetStateForComponent(OMX_StateExecuting);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_splitter OMX_StateExecuting 0x%08x", CLASSNAME, __func__, omx_err);
     return false;
    }
  }
  if( m_omx_render_analog.IsInitialized() )
  {
    omx_err = m_omx_render_analog.SetStateForComponent(OMX_StateExecuting);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_render_analog OMX_StateExecuting omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }
  if( m_omx_render_hdmi.IsInitialized() )
  {
    omx_err = m_omx_render_hdmi.SetStateForComponent(OMX_StateExecuting);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_render_hdmi OMX_StateExecuting omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  m_settings_changed = true;
  return true;
}

static uint64_t GetAVChannelLayout(CAEChannelInfo &info)
{
  #ifdef DEBUG_VERBOSE
  CLog::Log(LOGINFO, "%s::%s", CLASSNAME, __func__);
  #endif
  uint64_t channelLayout = 0;
  if (info.HasChannel(AE_CH_FL))   channelLayout |= AV_CH_FRONT_LEFT;
  if (info.HasChannel(AE_CH_FR))   channelLayout |= AV_CH_FRONT_RIGHT;
  if (info.HasChannel(AE_CH_FC))   channelLayout |= AV_CH_FRONT_CENTER;
  if (info.HasChannel(AE_CH_LFE))  channelLayout |= AV_CH_LOW_FREQUENCY;
  if (info.HasChannel(AE_CH_BL))   channelLayout |= AV_CH_BACK_LEFT;
  if (info.HasChannel(AE_CH_BR))   channelLayout |= AV_CH_BACK_RIGHT;
  if (info.HasChannel(AE_CH_FLOC)) channelLayout |= AV_CH_FRONT_LEFT_OF_CENTER;
  if (info.HasChannel(AE_CH_FROC)) channelLayout |= AV_CH_FRONT_RIGHT_OF_CENTER;
  if (info.HasChannel(AE_CH_BC))   channelLayout |= AV_CH_BACK_CENTER;
  if (info.HasChannel(AE_CH_SL))   channelLayout |= AV_CH_SIDE_LEFT;
  if (info.HasChannel(AE_CH_SR))   channelLayout |= AV_CH_SIDE_RIGHT;
  if (info.HasChannel(AE_CH_TC))   channelLayout |= AV_CH_TOP_CENTER;
  if (info.HasChannel(AE_CH_TFL))  channelLayout |= AV_CH_TOP_FRONT_LEFT;
  if (info.HasChannel(AE_CH_TFC))  channelLayout |= AV_CH_TOP_FRONT_CENTER;
  if (info.HasChannel(AE_CH_TFR))  channelLayout |= AV_CH_TOP_FRONT_RIGHT;
  if (info.HasChannel(AE_CH_TBL))   channelLayout |= AV_CH_TOP_BACK_LEFT;
  if (info.HasChannel(AE_CH_TBC))   channelLayout |= AV_CH_TOP_BACK_CENTER;
  if (info.HasChannel(AE_CH_TBR))   channelLayout |= AV_CH_TOP_BACK_RIGHT;

  return channelLayout;
}

static void SetAudioProps(bool stream_channels, uint32_t channel_map)
{
  char command[80], response[80];

  sprintf(command, "hdmi_stream_channels %d", stream_channels ? 1 : 0);
  vc_gencmd(response, sizeof response, command);

  sprintf(command, "hdmi_channel_map 0x%08x", channel_map);
  vc_gencmd(response, sizeof response, command);

  CLog::Log(LOGDEBUG, "%s:%s hdmi_stream_channels %d hdmi_channel_map %08x", CLASSNAME, __func__, stream_channels, channel_map);
}

static uint32_t GetChannelMap(const CAEChannelInfo &channelLayout, bool passthrough)
{
  unsigned int channels = channelLayout.Count();
  uint32_t channel_map = 0;
  if (passthrough)
    return 0;

  static const unsigned char map_normal[] =
  {
    0, //AE_CH_RAW ,
    1, //AE_CH_FL
    2, //AE_CH_FR
    4, //AE_CH_FC
    3, //AE_CH_LFE
    7, //AE_CH_BL
    8, //AE_CH_BR
    1, //AE_CH_FLOC,
    2, //AE_CH_FROC,
    4, //AE_CH_BC,
    5, //AE_CH_SL
    6, //AE_CH_SR
  };
  static const unsigned char map_back[] =
  {
    0, //AE_CH_RAW ,
    1, //AE_CH_FL
    2, //AE_CH_FR
    4, //AE_CH_FC
    3, //AE_CH_LFE
    5, //AE_CH_BL
    6, //AE_CH_BR
    1, //AE_CH_FLOC,
    2, //AE_CH_FROC,
    4, //AE_CH_BC,
    5, //AE_CH_SL
    6, //AE_CH_SR
  };
  const unsigned char *map = map_normal;
  // According to CEA-861-D only RL and RR are known. In case of a format having SL and SR channels
  // but no BR BL channels, we use the wide map in order to open only the num of channels really
  // needed.
  if (channelLayout.HasChannel(AE_CH_BL) && !channelLayout.HasChannel(AE_CH_SL))
    map = map_back;

  for (unsigned int i = 0; i < channels; ++i)
  {
    AEChannel c = channelLayout[i];
    unsigned int chan = 0;
    if ((unsigned int)c < sizeof map_normal / sizeof *map_normal)
      chan = map[(unsigned int)c];
    if (chan > 0)
      channel_map |= (chan-1) << (3*i);
  }
  // These numbers are from Table 28 Audio InfoFrame Data byte 4 of CEA 861
  // and describe the speaker layout
  static const uint8_t cea_map[] = {
    0xff, // 0
    0xff, // 1
    0x00, // 2.0
    0x02, // 3.0
    0x08, // 4.0
    0x0a, // 5.0
    0xff, // 6
    0x12, // 7.0
    0xff, // 8
  };
  static const uint8_t cea_map_lfe[] = {
    0xff, // 0
    0xff, // 1
    0xff, // 2
    0x01, // 2.1
    0x03, // 3.1
    0x09, // 4.1
    0x0b, // 5.1
    0xff, // 7
    0x13, // 7.1
  };
  uint8_t cea = channelLayout.HasChannel(AE_CH_LFE) ? cea_map_lfe[channels] : cea_map[channels];
  if (cea == 0xff)
    CLog::Log(LOGERROR, "%s::%s - Unexpected CEA mapping %d,%d", CLASSNAME, __func__, channelLayout.HasChannel(AE_CH_LFE), channels);

  channel_map |= cea << 24;

  return channel_map;
}

bool COMXAudio::Initialize(AEAudioFormat format, OMXClock *clock, CDVDStreamInfo &hints, CAEChannelInfo channelMap, bool bUsePassthrough)
{
  CSingleLock lock (m_critSection);
  OMX_ERRORTYPE omx_err;

  Deinitialize();

  m_Passthrough = bUsePassthrough;

  m_InputChannels = channelMap.Count();

  if(m_InputChannels == 0)
    return false;

  if(hints.samplerate == 0)
    return false;

  m_av_clock = clock;

  if(!m_av_clock)
    return false;

  SetCodingType(format);

  if (m_Passthrough || CServiceBroker::GetSettings().GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE) == "PI:HDMI")
    m_output = AESINKPI_HDMI;
  else if (CServiceBroker::GetSettings().GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE) == "PI:Analogue")
    m_output = AESINKPI_ANALOGUE;
  else if (CServiceBroker::GetSettings().GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE) == "PI:Both")
    m_output = AESINKPI_BOTH;
  else assert(0);

  if(hints.extrasize > 0 && hints.extradata != NULL)
  {
    m_extrasize = hints.extrasize;
    m_extradata = (uint8_t *)malloc(m_extrasize);
    memcpy(m_extradata, hints.extradata, hints.extrasize);
  }

  m_omx_clock   = m_av_clock->GetOMXClock();

  m_drc         = 0;

  memset(m_input_channels, 0x0, sizeof(m_input_channels));
  memset(m_output_channels, 0x0, sizeof(m_output_channels));
  memset(&m_wave_header, 0x0, sizeof(m_wave_header));

  m_wave_header.Format.nChannels  = 2;
  m_wave_header.dwChannelMask = 3; // SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;

  if (!m_Passthrough)
  {
    bool upmix = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX);
    bool normalize = !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME);
    void *remapLayout = NULL;

    CAEChannelInfo stdLayout = (enum AEStdChLayout)CServiceBroker::GetSettings().GetInt(CSettings::SETTING_AUDIOOUTPUT_CHANNELS);

    // ignore layout setting for analogue
    if (m_output == AESINKPI_ANALOGUE || m_output == AESINKPI_BOTH)
      stdLayout = AE_CH_LAYOUT_2_0;

    CAEChannelInfo resolvedMap = channelMap;
    resolvedMap.ResolveChannels(stdLayout);

    if (CServiceBroker::GetSettings().GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) == 1 /*AE_CONFIG_FIXED*/ || (upmix && channelMap.Count() <= 2))
      resolvedMap = stdLayout;

    uint64_t m_dst_chan_layout = GetAVChannelLayout(resolvedMap);
    uint64_t m_src_chan_layout = GetAVChannelLayout(channelMap);

    m_InputChannels = channelMap.Count();
    m_OutputChannels = resolvedMap.Count();

    int m_dst_channels = m_OutputChannels;
    int m_src_channels = m_InputChannels;
    SetAudioProps(m_Passthrough, GetChannelMap(resolvedMap, m_Passthrough));

    CLog::Log(LOGINFO, "%s::%s remap:%p chan:%d->%d norm:%d upmix:%d %llx:%llx", CLASSNAME, __func__, remapLayout, m_src_channels, m_dst_channels, normalize, upmix, m_src_chan_layout, m_dst_chan_layout);

    // this code is just uses ffmpeg to produce the 8x8 mixing matrix
    // dummy sample rate and format, as we only care about channel mapping
    SwrContext *m_pContext = swr_alloc_set_opts(NULL, m_dst_chan_layout, AV_SAMPLE_FMT_FLT, 48000,
                                                          m_src_chan_layout, AV_SAMPLE_FMT_FLT, 48000, 0, NULL);
    if(!m_pContext)
    {
      CLog::Log(LOGERROR, "COMXAudio::Init - create context failed");
      return false;
    }
    // tell resampler to clamp float values
    // not required for sink stage (remapLayout == true)
    if (!remapLayout && normalize)
    {
       av_opt_set_double(m_pContext, "rematrix_maxval", 1.0, 0);
    }

    // stereo upmix
    if (upmix && m_src_channels == 2 && m_dst_channels > 2)
    {
      double m_rematrix[AE_CH_MAX][AE_CH_MAX];
      memset(m_rematrix, 0, sizeof(m_rematrix));
      for (int out=0; out<m_dst_channels; out++)
      {
        uint64_t out_chan = av_channel_layout_extract_channel(m_dst_chan_layout, out);
        switch(out_chan)
        {
          case AV_CH_FRONT_LEFT:
          case AV_CH_BACK_LEFT:
          case AV_CH_SIDE_LEFT:
            m_rematrix[out][0] = 1.0;
            break;
          case AV_CH_FRONT_RIGHT:
          case AV_CH_BACK_RIGHT:
          case AV_CH_SIDE_RIGHT:
            m_rematrix[out][1] = 1.0;
            break;
          case AV_CH_FRONT_CENTER:
            m_rematrix[out][0] = 0.5;
            m_rematrix[out][1] = 0.5;
            break;
          case AV_CH_LOW_FREQUENCY:
            m_rematrix[out][0] = 0.5;
            m_rematrix[out][1] = 0.5;
            break;
          default:
            break;
        }
      }

      if (swr_set_matrix(m_pContext, (const double*)m_rematrix, AE_CH_MAX) < 0)
      {
        CLog::Log(LOGERROR, "COMXAudio::Init - setting channel matrix failed");
        return false;
      }
    }

    if (swr_init(m_pContext) < 0)
    {
      CLog::Log(LOGERROR, "COMXAudio::Init - init resampler failed");
      return false;
    }

    const int samples = 8;
    uint8_t *output, *input;
    av_samples_alloc(&output, NULL, m_dst_channels, samples, AV_SAMPLE_FMT_FLT, 1);
    av_samples_alloc(&input , NULL, m_src_channels, samples, AV_SAMPLE_FMT_FLT, 1);

    // Produce "identity" samples
    float *f = (float *)input;
    for (int j=0; j < samples; j++)
      for (int i=0; i < m_src_channels; i++)
        *f++ = i == j ? 1.0f : 0.0f;

    int ret = swr_convert(m_pContext, &output, samples, (const uint8_t **)&input, samples);
    if (ret < 0)
      CLog::Log(LOGERROR, "COMXAudio::Resample - resample failed");

    f = (float *)output;
    for (int j=0; j < 8; j++)
    {
      for (int i=0; i < m_dst_channels; i++)
        m_downmix_matrix[8*i+j] = *f++;
      for (int i=m_dst_channels; i < 8; i++)
        m_downmix_matrix[8*i+j] = 0.0f;
    }

    for (int j=0; j < 8; j++)
    {
      char s[128] = {}, *t=s;
      for (int i=0; i < 8; i++)
        t += sprintf(t, "% 6.2f ", m_downmix_matrix[j*8+i]);
      CLog::Log(LOGINFO, "%s::%s  %s", CLASSNAME, __func__, s);
    }
    av_freep(&input);
    av_freep(&output);
    swr_free(&m_pContext);

    m_wave_header.dwChannelMask = m_src_chan_layout;
  }
  else
    SetAudioProps(m_Passthrough, 0);

  m_SampleRate    = format.m_sampleRate;
  m_BitsPerSample = m_Passthrough ? 16 : CAEUtil::DataFormatToBits(format.m_dataFormat);
  m_BytesPerSec   = m_SampleRate * 2 << rounded_up_channels_shift[m_InputChannels];
  m_BufferLen     = m_BytesPerSec * AUDIO_BUFFER_SECONDS;
  m_InputBytesPerSec = m_SampleRate * m_BitsPerSample * m_InputChannels >> 3;

  // should be big enough that common formats (e.g. 6 channel DTS) fit in a single packet.
  // we don't mind less common formats being split (e.g. ape/wma output large frames)
  // the audio_decode output buffer size is 32K, and typically we convert from
  // 6 channel 32bpp float to 8 channel 16bpp in, so a full 48K input buffer will fit the output buffer
  m_ChunkLen = AUDIO_DECODE_OUTPUT_BUFFER * (m_InputChannels * m_BitsPerSample) >> (rounded_up_channels_shift[m_InputChannels] + 4);

  m_wave_header.Samples.wSamplesPerBlock    = 0;
  m_wave_header.Format.nChannels            = m_InputChannels;
  m_wave_header.Format.nBlockAlign          = m_InputChannels * (m_BitsPerSample >> 3);
  // 0x8000 is custom format interpreted by GPU as WAVE_FORMAT_IEEE_FLOAT_PLANAR
  m_wave_header.Format.wFormatTag           = m_BitsPerSample == 32 ? 0x8000 : WAVE_FORMAT_PCM;
  m_wave_header.Format.nSamplesPerSec       = format.m_sampleRate;
  m_wave_header.Format.nAvgBytesPerSec      = m_BytesPerSec;
  m_wave_header.Format.wBitsPerSample       = m_BitsPerSample;
  m_wave_header.Samples.wValidBitsPerSample = m_BitsPerSample;
  m_wave_header.Format.cbSize               = 0;
  m_wave_header.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;

  OMX_INIT_STRUCTURE(m_pcm_input);
  memcpy(m_pcm_input.eChannelMapping, m_input_channels, sizeof(m_input_channels));
  m_pcm_input.eNumData              = OMX_NumericalDataSigned;
  m_pcm_input.eEndian               = OMX_EndianLittle;
  m_pcm_input.bInterleaved          = OMX_TRUE;
  m_pcm_input.nBitPerSample         = m_BitsPerSample;
  m_pcm_input.ePCMMode              = OMX_AUDIO_PCMModeLinear;
  m_pcm_input.nChannels             = m_InputChannels;
  m_pcm_input.nSamplingRate         = format.m_sampleRate;

  if(!m_omx_decoder.Initialize("OMX.broadcom.audio_decode", OMX_IndexParamAudioInit))
    return false;

  OMX_CONFIG_BOOLEANTYPE boolType;
  OMX_INIT_STRUCTURE(boolType);
  if(m_Passthrough)
    boolType.bEnabled = OMX_TRUE;
  else
    boolType.bEnabled = OMX_FALSE;
  omx_err = m_omx_decoder.SetParameter(OMX_IndexParamBrcmDecoderPassThrough, &boolType);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize - Error OMX_IndexParamBrcmDecoderPassThrough 0x%08x", omx_err);
    return false;
  }

  // set up the number/size of buffers for decoder input
  OMX_PARAM_PORTDEFINITIONTYPE port_param;
  OMX_INIT_STRUCTURE(port_param);
  port_param.nPortIndex = m_omx_decoder.GetInputPort();

  omx_err = m_omx_decoder.GetParameter(OMX_IndexParamPortDefinition, &port_param);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize error get OMX_IndexParamPortDefinition (input) omx_err(0x%08x)\n", omx_err);
    return false;
  }

  port_param.format.audio.eEncoding = m_eEncoding;

  port_param.nBufferSize = m_ChunkLen;
  port_param.nBufferCountActual = std::max((unsigned int)port_param.nBufferCountMin, 16U);

  omx_err = m_omx_decoder.SetParameter(OMX_IndexParamPortDefinition, &port_param);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize error set OMX_IndexParamPortDefinition (input) omx_err(0x%08x)\n", omx_err);
    return false;
  }

  // set up the number/size of buffers for decoder output
  OMX_INIT_STRUCTURE(port_param);
  port_param.nPortIndex = m_omx_decoder.GetOutputPort();

  omx_err = m_omx_decoder.GetParameter(OMX_IndexParamPortDefinition, &port_param);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize error get OMX_IndexParamPortDefinition (output) omx_err(0x%08x)\n", omx_err);
    return false;
  }

  port_param.nBufferCountActual = std::max((unsigned int)port_param.nBufferCountMin, m_BufferLen / port_param.nBufferSize);

  omx_err = m_omx_decoder.SetParameter(OMX_IndexParamPortDefinition, &port_param);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize error set OMX_IndexParamPortDefinition (output) omx_err(0x%08x)\n", omx_err);
    return false;
  }

  {
    OMX_AUDIO_PARAM_PORTFORMATTYPE formatType;
    OMX_INIT_STRUCTURE(formatType);
    formatType.nPortIndex = m_omx_decoder.GetInputPort();

    formatType.eEncoding = m_eEncoding;

    omx_err = m_omx_decoder.SetParameter(OMX_IndexParamAudioPortFormat, &formatType);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize error OMX_IndexParamAudioPortFormat omx_err(0x%08x)\n", omx_err);
      return false;
    }
  }

  omx_err = m_omx_decoder.AllocInputBuffers();
  if(omx_err != OMX_ErrorNone) 
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize - Error alloc buffers 0x%08x", omx_err);
    return false;
  }

  omx_err = m_omx_decoder.SetStateForComponent(OMX_StateExecuting);
  if(omx_err != OMX_ErrorNone) {
    CLog::Log(LOGERROR, "COMXAudio::Initialize - Error setting OMX_StateExecuting 0x%08x", omx_err);
    return false;
  }


  if(m_eEncoding == OMX_AUDIO_CodingPCM)
  {
    OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_decoder.GetInputBuffer();
    if(omx_buffer == NULL)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - buffer error 0x%08x", omx_err);
      return false;
    }

    omx_buffer->nOffset = 0;
    omx_buffer->nFilledLen = std::min(sizeof(m_wave_header), omx_buffer->nAllocLen);

    memset((unsigned char *)omx_buffer->pBuffer, 0x0, omx_buffer->nAllocLen);
    memcpy((unsigned char *)omx_buffer->pBuffer, &m_wave_header, omx_buffer->nFilledLen);
    omx_buffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;

    omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
      m_omx_decoder.DecoderEmptyBufferDone(m_omx_decoder.GetComponent(), omx_buffer);
      return false;
    }
  } 

  /* return on decoder error so m_Initialized stays false */
  if(m_omx_decoder.BadState())
    return false;

  m_Initialized   = true;
  m_settings_changed = false;
  m_setStartTime = true;
  m_submitted_eos = false;
  m_failed_eos = false;
  m_last_pts      = DVD_NOPTS_VALUE;
  m_submitted     = 0.0f;
  m_maxLevel      = 0.0f;

  CLog::Log(LOGDEBUG, "COMXAudio::Initialize Input bps %d samplerate %d channels %d buffer size %d bytes per second %d",
      (int)m_pcm_input.nBitPerSample, (int)m_pcm_input.nSamplingRate, (int)m_pcm_input.nChannels, m_BufferLen, m_InputBytesPerSec);
  PrintPCM(&m_pcm_input, std::string("input"));
  CLog::Log(LOGDEBUG, "COMXAudio::Initialize device passthrough %d", m_Passthrough);

  return true;
}

//***********************************************************************************************
bool COMXAudio::Deinitialize()
{
  CSingleLock lock (m_critSection);

  if ( m_omx_tunnel_clock_analog.IsInitialized() )
    m_omx_tunnel_clock_analog.Deestablish();
  if ( m_omx_tunnel_clock_hdmi.IsInitialized() )
    m_omx_tunnel_clock_hdmi.Deestablish();

  // ignore expected errors on teardown
  if ( m_omx_mixer.IsInitialized() )
    m_omx_mixer.IgnoreNextError(OMX_ErrorPortUnpopulated);
  else
  {
    if ( m_omx_render_hdmi.IsInitialized() )
      m_omx_render_hdmi.IgnoreNextError(OMX_ErrorPortUnpopulated);
    if ( m_omx_render_analog.IsInitialized() )
      m_omx_render_analog.IgnoreNextError(OMX_ErrorPortUnpopulated);
  }

  m_omx_tunnel_decoder.Deestablish();
  if ( m_omx_tunnel_mixer.IsInitialized() )
    m_omx_tunnel_mixer.Deestablish();
  if ( m_omx_tunnel_splitter_hdmi.IsInitialized() )
    m_omx_tunnel_splitter_hdmi.Deestablish();
  if ( m_omx_tunnel_splitter_analog.IsInitialized() )
    m_omx_tunnel_splitter_analog.Deestablish();

  m_omx_decoder.FlushInput();

  m_omx_decoder.Deinitialize();
  if ( m_omx_mixer.IsInitialized() )
    m_omx_mixer.Deinitialize();
  if ( m_omx_splitter.IsInitialized() )
    m_omx_splitter.Deinitialize();
  if ( m_omx_render_hdmi.IsInitialized() )
    m_omx_render_hdmi.Deinitialize();
  if ( m_omx_render_analog.IsInitialized() )
    m_omx_render_analog.Deinitialize();

  m_BytesPerSec = 0;
  m_BufferLen   = 0;

  m_omx_clock = NULL;
  m_av_clock  = NULL;

  m_Initialized = false;

  if(m_extradata)
    free(m_extradata);
  m_extradata = NULL;
  m_extrasize = 0;

  while(!m_ampqueue.empty())
    m_ampqueue.pop_front();

  m_last_pts      = DVD_NOPTS_VALUE;
  m_submitted     = 0.0f;
  m_maxLevel      = 0.0f;

  return true;
}

void COMXAudio::Flush()
{
  CSingleLock lock (m_critSection);
  if(!m_Initialized)
    return;

  m_omx_decoder.FlushAll();

  if ( m_omx_mixer.IsInitialized() )
    m_omx_mixer.FlushAll();

  if ( m_omx_splitter.IsInitialized() )
    m_omx_splitter.FlushAll();

  if ( m_omx_render_analog.IsInitialized() )
    m_omx_render_analog.FlushAll();
  if ( m_omx_render_hdmi.IsInitialized() )
    m_omx_render_hdmi.FlushAll();

  if ( m_omx_render_analog.IsInitialized() )
    m_omx_render_analog.ResetEos();
  if ( m_omx_render_hdmi.IsInitialized() )
    m_omx_render_hdmi.ResetEos();

  m_last_pts      = DVD_NOPTS_VALUE;
  m_submitted     = 0.0f;
  m_maxLevel      = 0.0f;
  m_setStartTime  = true;
}

//***********************************************************************************************
void COMXAudio::SetDynamicRangeCompression(long drc)
{
  CSingleLock lock (m_critSection);
  m_amplification = powf(10.0f, (float)drc / 2000.0f);
  if (m_settings_changed)
    UpdateAttenuation();
}

//***********************************************************************************************
void COMXAudio::SetMute(bool bMute)
{
  CSingleLock lock (m_critSection);
  m_Mute = bMute;
  if (m_settings_changed)
    UpdateAttenuation();
}

//***********************************************************************************************
void COMXAudio::SetVolume(float fVolume)
{
  CSingleLock lock (m_critSection);
  m_CurrentVolume = fVolume;
  if (m_settings_changed)
    UpdateAttenuation();
}

//***********************************************************************************************
bool COMXAudio::ApplyVolume(void)
{
  CSingleLock lock (m_critSection);

  if (!m_Initialized || m_Passthrough)
    return false;

  float fVolume = m_Mute ? VOLUME_MINIMUM : m_CurrentVolume;
  // need to convert a log scale of 0.0=-60dB, 1.0=0dB to a linear scale (0.0=silence, 1.0=full)
  fVolume = CAEUtil::GainToScale(CAEUtil::PercentToGain(fVolume));

  // the analogue volume is too quiet for some. Allow use of an advancedsetting to boost this (at risk of distortion) (deprecated)
  double gain = pow(10, (g_advancedSettings.m_ac3Gain - 12.0f) / 20.0);

  const float* coeff = m_downmix_matrix;

  OMX_CONFIG_BRCMAUDIODOWNMIXCOEFFICIENTS8x8 mix;
  OMX_INIT_STRUCTURE(mix);
  OMX_ERRORTYPE omx_err;

  assert(ARRAY_SIZE(mix.coeff) == 64);

  if (m_amplification != 1.0)
  {
    // reduce scaling so overflow can be seen
    for(size_t i = 0; i < 8*8; ++i)
      mix.coeff[i] = static_cast<unsigned int>(0x10000 * (coeff[i] * gain * 0.01f));

    mix.nPortIndex = m_omx_decoder.GetInputPort();
    omx_err = m_omx_decoder.SetConfig(OMX_IndexConfigBrcmAudioDownmixCoefficients8x8, &mix);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - error setting decoder OMX_IndexConfigBrcmAudioDownmixCoefficients, error 0x%08x\n",
                CLASSNAME, __func__, omx_err);
      return false;
    }
  }
  for(size_t i = 0; i < 8*8; ++i)
    mix.coeff[i] = static_cast<unsigned int>(0x10000 * (coeff[i] * gain * fVolume * m_amplification * m_attenuation));

  mix.nPortIndex = m_omx_mixer.GetInputPort();
  omx_err = m_omx_mixer.SetConfig(OMX_IndexConfigBrcmAudioDownmixCoefficients8x8, &mix);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - error setting mixer OMX_IndexConfigBrcmAudioDownmixCoefficients, error 0x%08x\n",
              CLASSNAME, __func__, omx_err);
    return false;
  }
  CLog::Log(LOGINFO, "%s::%s - Volume=%.2f (* %.2f * %.2f)\n", CLASSNAME, __func__, fVolume, m_amplification, m_attenuation);
  return true;
}

//***********************************************************************************************
unsigned int COMXAudio::AddPackets(const void* data, unsigned int len, double dts, double pts, unsigned int frame_size, bool &settings_changed)
{
  CSingleLock lock (m_critSection);

  if(!m_Initialized)
  {
    CLog::Log(LOGERROR,"COMXAudio::AddPackets - sanity failed. no valid play handle!");
    return len;
  }

  unsigned pitch = m_Passthrough ? 1:(m_BitsPerSample >> 3) * m_InputChannels;
  unsigned int demuxer_samples = len / pitch;
  unsigned int demuxer_samples_sent = 0;
  uint8_t *demuxer_content = (uint8_t *)data;

  OMX_ERRORTYPE omx_err;

  OMX_BUFFERHEADERTYPE *omx_buffer = NULL;

  while(demuxer_samples_sent < demuxer_samples)
  {
    // 200ms timeout
    omx_buffer = m_omx_decoder.GetInputBuffer(200);

    if(omx_buffer == NULL)
    {
      CLog::Log(LOGERROR, "COMXAudio::Decode timeout\n");
      return len;
    }

    omx_buffer->nOffset = 0;
    omx_buffer->nFlags  = 0;

    // we want audio_decode output buffer size to be no more than AUDIO_DECODE_OUTPUT_BUFFER.
    // it will be 16-bit and rounded up to next power of 2 in channels
    unsigned int max_buffer = AUDIO_DECODE_OUTPUT_BUFFER * (m_InputChannels * m_BitsPerSample) >> (rounded_up_channels_shift[m_InputChannels] + 4);

    unsigned int remaining = demuxer_samples-demuxer_samples_sent;
    unsigned int samples_space = std::min(max_buffer, omx_buffer->nAllocLen)/pitch;
    unsigned int samples = std::min(remaining, samples_space);

    omx_buffer->nFilledLen = samples * pitch;

    unsigned int frames = frame_size ? len/frame_size:0;
    if ((samples < demuxer_samples || frames > 1) && m_BitsPerSample==32 && !m_Passthrough)
    {
      const unsigned int sample_pitch   = m_BitsPerSample >> 3;
      const unsigned int frame_samples  = frame_size / pitch;
      const unsigned int plane_size     = frame_samples * sample_pitch;
      const unsigned int out_plane_size = samples * sample_pitch;
      //CLog::Log(LOGDEBUG, "%s::%s samples:%d/%d ps:%d ops:%d fs:%d pitch:%d filled:%d frames=%d", CLASSNAME, __func__, samples, demuxer_samples, plane_size, out_plane_size, frame_size, pitch, omx_buffer->nFilledLen, frames);
      for (unsigned int sample = 0; sample < samples; )
      {
        unsigned int frame = (demuxer_samples_sent + sample) / frame_samples;
        unsigned int sample_in_frame = (demuxer_samples_sent + sample) - frame * frame_samples;
        int out_remaining = std::min(std::min(frame_samples - sample_in_frame, samples), samples-sample);
        uint8_t *src = demuxer_content + frame*frame_size + sample_in_frame * sample_pitch;
        uint8_t *dst = (uint8_t *)omx_buffer->pBuffer + sample * sample_pitch;
        for (unsigned int channel = 0; channel < m_InputChannels; channel++)
        {
          //CLog::Log(LOGDEBUG, "%s::%s copy(%d,%d,%d) (s:%d f:%d sin:%d c:%d)", CLASSNAME, __func__, dst-(uint8_t *)omx_buffer->pBuffer, src-demuxer_content, out_remaining, sample, frame, sample_in_frame, channel);
          memcpy(dst, src, out_remaining * sample_pitch);
          src += plane_size;
          dst += out_plane_size;
        }
        sample += out_remaining;
      }
    }
    else
    {
       uint8_t *dst = omx_buffer->pBuffer;
       uint8_t *src = demuxer_content + demuxer_samples_sent * pitch;
       memcpy(dst, src, omx_buffer->nFilledLen);
    }

    uint64_t val  = (uint64_t)(pts == DVD_NOPTS_VALUE) ? 0 : pts;

    if(m_setStartTime)
    {
      omx_buffer->nFlags = OMX_BUFFERFLAG_STARTTIME;
      if(pts == DVD_NOPTS_VALUE)
        omx_buffer->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;

      m_last_pts = pts;

      CLog::Log(LOGDEBUG, "COMXAudio::Decode ADec : setStartTime %f\n", (float)val / DVD_TIME_BASE);
      m_setStartTime = false;
    }
    else
    {
      if(pts == DVD_NOPTS_VALUE)
      {
        omx_buffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
        m_last_pts = pts;
      }
      else if (m_last_pts != pts)
      {
        if(pts > m_last_pts)
          m_last_pts = pts;
        else
          omx_buffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
      }
      else if (m_last_pts == pts)
      {
        omx_buffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
      }
    }

    omx_buffer->nTimeStamp = ToOMXTime(val);

    demuxer_samples_sent += samples;

    if(demuxer_samples_sent == demuxer_samples)
      omx_buffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

    omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() finally failed\n", CLASSNAME, __func__);
      m_omx_decoder.DecoderEmptyBufferDone(m_omx_decoder.GetComponent(), omx_buffer);
      return 0;
    }
    //CLog::Log(LOGINFO, "AudiD: dts:%.0f pts:%.0f size:%d\n", dts, pts, len);

    omx_err = m_omx_decoder.WaitForEvent(OMX_EventPortSettingsChanged, 0);
    if (omx_err == OMX_ErrorNone)
    {
      if(!PortSettingsChanged())
      {
        CLog::Log(LOGERROR, "%s::%s - error PortSettingsChanged omx_err(0x%08x)\n", CLASSNAME, __func__, omx_err);
      }
    }
  }
  m_submitted += (float)demuxer_samples / m_SampleRate;
  if (m_amplification != 1.0)
    UpdateAttenuation();
  settings_changed = m_settings_changed;
  return len;
}

void COMXAudio::UpdateAttenuation()
{
  // always called with m_critSection lock held
  if (!m_Initialized || m_Passthrough)
    return;

  if (m_amplification == 1.0)
  {
    ApplyVolume();
    return;
  }

  double level_pts = 0.0;
  float level = GetMaxLevel(level_pts);
  if (level_pts != 0.0)
  {
    amplitudes_t v;
    v.level = level;
    v.pts = level_pts;
    m_ampqueue.push_back(v);
  }
  double stamp = m_av_clock->OMXMediaTime();
  // discard too old data
  while(!m_ampqueue.empty())
  {
    amplitudes_t &v = m_ampqueue.front();
    /* we'll also consume if queue gets unexpectedly long to avoid filling memory */
    if (v.pts == DVD_NOPTS_VALUE || v.pts < stamp || v.pts - stamp > DVD_SEC_TO_TIME(15.0))
      m_ampqueue.pop_front();
    else break;
  }
  float maxlevel = 0.0f, imminent_maxlevel = 0.0f;
  for (int i=0; i < (int)m_ampqueue.size(); i++)
  {
    amplitudes_t &v = m_ampqueue[i];
    maxlevel = std::max(maxlevel, v.level);
    // check for maximum volume in next 200ms
    if (v.pts != DVD_NOPTS_VALUE && v.pts < stamp + DVD_SEC_TO_TIME(0.2))
      imminent_maxlevel = std::max(imminent_maxlevel, v.level);
  }

  if (maxlevel != 0.0)
  {
    float alpha_h = -1.0f/(0.025f*log10f(0.999f));
    float alpha_r = -1.0f/(0.100f*log10f(0.900f));
    float decay  = powf(10.0f, -1.0f / (alpha_h * g_advancedSettings.m_limiterHold));
    float attack = powf(10.0f, -1.0f / (alpha_r * g_advancedSettings.m_limiterRelease));
    // if we are going to clip imminently then deal with it now
    if (imminent_maxlevel > m_maxLevel)
      m_maxLevel = imminent_maxlevel;
    // clip but not imminently can ramp up more slowly
    else if (maxlevel > m_maxLevel)
      m_maxLevel = attack * m_maxLevel + (1.0f-attack) * maxlevel;
    // not clipping, decay more slowly
    else
      m_maxLevel = decay  * m_maxLevel + (1.0f-decay ) * maxlevel;

    // want m_maxLevel * amp -> 1.0
    float amp = m_amplification * m_attenuation;

    // We fade in the attenuation over first couple of seconds
    float start = std::min(std::max((m_submitted-1.0f), 0.0f), 1.0f);
    float attenuation = std::min(1.0f, std::max(m_attenuation / (amp * m_maxLevel), 1.0f/m_amplification));
    m_attenuation = (1.0f - start) * 1.0f/m_amplification + start * attenuation;
  }
  else
  {
    m_attenuation = 1.0f/m_amplification;
  }
  ApplyVolume();
}

//***********************************************************************************************
unsigned int COMXAudio::GetSpace()
{
  return m_omx_decoder.GetInputBufferSpace();
}

float COMXAudio::GetDelay()
{
  CSingleLock lock (m_critSection);
  double stamp = DVD_NOPTS_VALUE;
  double ret = 0.0;
  if (m_last_pts != DVD_NOPTS_VALUE && m_av_clock)
    stamp = m_av_clock->OMXMediaTime();
  // if possible the delay is current media time - time of last submitted packet
  if (stamp != DVD_NOPTS_VALUE && stamp != 0.0)
  {
    ret = (m_last_pts - stamp) * (1.0 / DVD_TIME_BASE);
  }
  else // just measure the input fifo
  {
    unsigned int used = m_omx_decoder.GetInputBufferSize() - m_omx_decoder.GetInputBufferSpace();
    ret = m_InputBytesPerSec ? (float)used / (float)m_InputBytesPerSec : 0.0f;
  }
  return ret;
}

float COMXAudio::GetCacheTime()
{
  return GetDelay();
}

float COMXAudio::GetCacheTotal()
{
  float audioplus_buffer = m_SampleRate ? 32.0f * 512.0f / m_SampleRate : 0.0f;
  float input_buffer = m_InputBytesPerSec ? (float)m_omx_decoder.GetInputBufferSize() / (float)m_InputBytesPerSec : 0;
  return AUDIO_BUFFER_SECONDS + input_buffer + audioplus_buffer;
}

//***********************************************************************************************
unsigned int COMXAudio::GetChunkLen()
{
  return m_ChunkLen;
}
//***********************************************************************************************
int COMXAudio::SetPlaySpeed(int iSpeed)
{
  return 0;
}

unsigned int COMXAudio::GetAudioRenderingLatency() const
{
  CSingleLock lock (m_critSection);

  if(!m_Initialized)
    return 0;

  OMX_PARAM_U32TYPE param;
  OMX_INIT_STRUCTURE(param);

  if(m_omx_render_analog.IsInitialized())
  {
    param.nPortIndex = m_omx_render_analog.GetInputPort();

    OMX_ERRORTYPE omx_err = m_omx_render_analog.GetConfig(OMX_IndexConfigAudioRenderingLatency, &param);

    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - error getting OMX_IndexConfigAudioRenderingLatency error 0x%08x\n",
        CLASSNAME, __func__, omx_err);
      return 0;
    }
  }
  else if(m_omx_render_hdmi.IsInitialized())
  {
    param.nPortIndex = m_omx_render_hdmi.GetInputPort();

    OMX_ERRORTYPE omx_err = m_omx_render_hdmi.GetConfig(OMX_IndexConfigAudioRenderingLatency, &param);

    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - error getting OMX_IndexConfigAudioRenderingLatency error 0x%08x\n",
        CLASSNAME, __func__, omx_err);
      return 0;
    }
  }

  return param.nU32;
}

float COMXAudio::GetMaxLevel(double &pts)
{
  CSingleLock lock (m_critSection);

  if(!m_Initialized)
    return 0;

  OMX_CONFIG_BRCMAUDIOMAXSAMPLE param;
  OMX_INIT_STRUCTURE(param);

  if(m_omx_decoder.IsInitialized())
  {
    param.nPortIndex = m_omx_decoder.GetInputPort();

    OMX_ERRORTYPE omx_err = m_omx_decoder.GetConfig(OMX_IndexConfigBrcmAudioMaxSample, &param);

    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - error getting OMX_IndexConfigBrcmAudioMaxSample error 0x%08x\n",
        CLASSNAME, __func__, omx_err);
      return 0;
    }
  }
  pts = FromOMXTime(param.nTimeStamp);
  return (float)param.nMaxSample * (100.0f / (1<<15));
}

void COMXAudio::SubmitEOS()
{
  CSingleLock lock (m_critSection);

  if(!m_Initialized)
    return;

  m_submitted_eos = true;
  m_failed_eos = false;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_decoder.GetInputBuffer(1000);

  if(omx_buffer == NULL)
  {
    CLog::Log(LOGERROR, "%s::%s - buffer error 0x%08x", CLASSNAME, __func__, omx_err);
    m_failed_eos = true;
    return;
  }

  omx_buffer->nOffset     = 0;
  omx_buffer->nFilledLen  = 0;
  omx_buffer->nTimeStamp  = ToOMXTime(0LL);

  omx_buffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS | OMX_BUFFERFLAG_TIME_UNKNOWN;

  omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
    m_omx_decoder.DecoderEmptyBufferDone(m_omx_decoder.GetComponent(), omx_buffer);
    return;
  }
  CLog::Log(LOGINFO, "%s::%s", CLASSNAME, __func__);
}

bool COMXAudio::IsEOS()
{
  if(!m_Initialized)
    return true;
  unsigned int latency = GetAudioRenderingLatency();
  CSingleLock lock (m_critSection);

  if (!m_failed_eos && !(m_omx_decoder.IsEOS() && latency == 0))
    return false;

  if (m_submitted_eos)
  {
    CLog::Log(LOGINFO, "%s::%s", CLASSNAME, __func__);
    m_submitted_eos = false;
  }
  return true;
}

void COMXAudio::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}

void COMXAudio::SetCodingType(AEAudioFormat format)
{
  CAEStreamInfo::DataType type = m_Passthrough ? format.m_streamInfo.m_type : CAEStreamInfo::STREAM_TYPE_NULL;
  switch(type)
  {
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      CLog::Log(LOGDEBUG, "COMXAudio::SetCodingType OMX_AUDIO_CodingDTS\n");
      m_eEncoding = OMX_AUDIO_CodingDTS;
      break;
    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      CLog::Log(LOGDEBUG, "COMXAudio::SetCodingType OMX_AUDIO_CodingDDP\n");
      m_eEncoding = OMX_AUDIO_CodingDDP;
      break;
    default:
      CLog::Log(LOGDEBUG, "COMXAudio::SetCodingType OMX_AUDIO_CodingPCM\n");
      m_eEncoding = OMX_AUDIO_CodingPCM;
      break;
  }
}

void COMXAudio::PrintChannels(OMX_AUDIO_CHANNELTYPE eChannelMapping[])
{
  for(int i = 0; i < OMX_AUDIO_MAXCHANNELS; i++)
  {
    switch(eChannelMapping[i])
    {
      case OMX_AUDIO_ChannelLF:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelLF\n");
        break;
      case OMX_AUDIO_ChannelRF:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelRF\n");
        break;
      case OMX_AUDIO_ChannelCF:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelCF\n");
        break;
      case OMX_AUDIO_ChannelLS:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelLS\n");
        break;
      case OMX_AUDIO_ChannelRS:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelRS\n");
        break;
      case OMX_AUDIO_ChannelLFE:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelLFE\n");
        break;
      case OMX_AUDIO_ChannelCS:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelCS\n");
        break;
      case OMX_AUDIO_ChannelLR:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelLR\n");
        break;
      case OMX_AUDIO_ChannelRR:
        CLog::Log(LOGDEBUG, "OMX_AUDIO_ChannelRR\n");
        break;
      case OMX_AUDIO_ChannelNone:
      case OMX_AUDIO_ChannelKhronosExtensions:
      case OMX_AUDIO_ChannelVendorStartUnused:
      case OMX_AUDIO_ChannelMax:
      default:
        break;
    }
  }
}

void COMXAudio::PrintPCM(OMX_AUDIO_PARAM_PCMMODETYPE *pcm, std::string direction)
{
  CLog::Log(LOGDEBUG, "pcm->direction      : %s\n", direction.c_str());
  CLog::Log(LOGDEBUG, "pcm->nPortIndex     : %d\n", (int)pcm->nPortIndex);
  CLog::Log(LOGDEBUG, "pcm->eNumData       : %d\n", pcm->eNumData);
  CLog::Log(LOGDEBUG, "pcm->eEndian        : %d\n", pcm->eEndian);
  CLog::Log(LOGDEBUG, "pcm->bInterleaved   : %d\n", (int)pcm->bInterleaved);
  CLog::Log(LOGDEBUG, "pcm->nBitPerSample  : %d\n", (int)pcm->nBitPerSample);
  CLog::Log(LOGDEBUG, "pcm->ePCMMode       : %d\n", pcm->ePCMMode);
  CLog::Log(LOGDEBUG, "pcm->nChannels      : %d\n", (int)pcm->nChannels);
  CLog::Log(LOGDEBUG, "pcm->nSamplingRate  : %d\n", (int)pcm->nSamplingRate);

  PrintChannels(pcm->eChannelMapping);
}
