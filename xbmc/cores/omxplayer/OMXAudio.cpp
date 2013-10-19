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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#elif defined(TARGET_WINDOWS)
#include "system.h"
#endif

#include "OMXAudio.h"
#include "Application.h"
#include "utils/log.h"

#define CLASSNAME "COMXAudio"

#include "linux/XMemUtils.h"

#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
#include "cores/AudioEngine/Utils/AEConvert.h"

using namespace std;

#define OMX_MAX_CHANNELS 10

static enum AEChannel OMXChannelMap[OMX_MAX_CHANNELS] = 
{
  AE_CH_FL      , AE_CH_FR, 
  AE_CH_FC      , AE_CH_LFE, 
  AE_CH_BL      , AE_CH_BR,
  AE_CH_SL      , AE_CH_SR,
  AE_CH_BC      , AE_CH_RAW
};

static enum OMX_AUDIO_CHANNELTYPE OMXChannels[OMX_MAX_CHANNELS] =
{
  OMX_AUDIO_ChannelLF, OMX_AUDIO_ChannelRF,
  OMX_AUDIO_ChannelCF, OMX_AUDIO_ChannelLFE,
  OMX_AUDIO_ChannelLR, OMX_AUDIO_ChannelRR,
  OMX_AUDIO_ChannelLS, OMX_AUDIO_ChannelRS,
  OMX_AUDIO_ChannelCS, OMX_AUDIO_ChannelNone
};

static unsigned int WAVEChannels[OMX_MAX_CHANNELS] =
{
  SPEAKER_FRONT_LEFT,       SPEAKER_FRONT_RIGHT,
  SPEAKER_TOP_FRONT_CENTER, SPEAKER_LOW_FREQUENCY,
  SPEAKER_BACK_LEFT,        SPEAKER_BACK_RIGHT,
  SPEAKER_SIDE_LEFT,        SPEAKER_SIDE_RIGHT,
  SPEAKER_BACK_CENTER,      0
};

static const uint16_t AC3Bitrates[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod   [] = {48000, 44100, 32000, 0};

static const uint16_t DTSFSCod   [] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0};

// 7.1 downmixing coefficients
const float downmixing_coefficients_8[OMX_AUDIO_MAXCHANNELS] = {
  //        L       R
  /* L */   1,      0,
  /* R */   0,      1,
  /* C */   0.7071, 0.7071,
  /* LFE */ 0.7071, 0.7071,
  /* Ls */  0.7071, 0,
  /* Rs */  0,      0.7071,
  /* Lr */  0.7071, 0,
  /* Rr */  0,      0.7071
};

// 7.1 downmixing coefficients with boosted centre channel
const float downmixing_coefficients_8_boostcentre[OMX_AUDIO_MAXCHANNELS] = {
  //        L       R
  /* L */   0.7071, 0,
  /* R */   0,      0.7071,
  /* C */   1,      1,
  /* LFE */ 0.7071, 0.7071,
  /* Ls */  0.7071, 0,
  /* Rs */  0,      0.7071,
  /* Lr */  0.7071, 0,
  /* Rr */  0,      0.7071
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
COMXAudio::COMXAudio() :
  m_pCallback       (NULL   ),
  m_Initialized     (false  ),
  m_CurrentVolume   (0      ),
  m_Mute            (false  ),
  m_drc             (0      ),
  m_Passthrough     (false  ),
  m_HWDecode        (false  ),
  m_BytesPerSec     (0      ),
  m_BufferLen       (0      ),
  m_ChunkLen        (0      ),
  m_BitsPerSample   (0      ),
  m_maxLevel        (0.0f   ),
  m_amplification   (1.0f   ),
  m_attenuation     (1.0f   ),
  m_desired_attenuation(1.0f),
  m_omx_clock       (NULL   ),
  m_av_clock        (NULL   ),
  m_settings_changed(false  ),
  m_setStartTime    (false  ),
  m_LostSync        (true   ),
  m_SampleRate      (0      ),
  m_eEncoding       (OMX_AUDIO_CodingPCM),
  m_extradata       (NULL   ),
  m_extrasize       (0      ),
  m_last_pts        (DVD_NOPTS_VALUE),
  m_submitted_eos   (false  ),
  m_failed_eos      (false  )
{
  m_vizBufferSize   = m_vizRemapBufferSize = VIS_PACKET_SIZE * sizeof(float);
  m_vizRemapBuffer  = (uint8_t *)_aligned_malloc(m_vizRemapBufferSize,16);
  m_vizBuffer       = (uint8_t *)_aligned_malloc(m_vizBufferSize,16);
}

COMXAudio::~COMXAudio()
{
  Deinitialize();

  _aligned_free(m_vizRemapBuffer);
  _aligned_free(m_vizBuffer);
}


CAEChannelInfo COMXAudio::GetChannelLayout(AEAudioFormat format)
{
  unsigned int count = 0;

  if(format.m_dataFormat == AE_FMT_AC3 ||
    format.m_dataFormat == AE_FMT_DTS ||
    format.m_dataFormat == AE_FMT_EAC3)
    count = 2;
  else if (format.m_dataFormat == AE_FMT_TRUEHD ||
    format.m_dataFormat == AE_FMT_DTSHD)
    count = 8;
  else
  {
    for (unsigned int c = 0; c < 8; ++c)
    {
      for (unsigned int i = 0; i < format.m_channelLayout.Count(); ++i)
      {
        if (format.m_channelLayout[i] == OMXChannelMap[c])
        {
          count = c + 1;
          break;
        }
      }
    }
  }

  CAEChannelInfo info;
  for (unsigned int i = 0; i < count; ++i)
    info += OMXChannelMap[i];

  return info;
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
  if(CSettings::Get().GetBool("audiooutput.dualaudio"))
  {
    if(!m_omx_splitter.Initialize("OMX.broadcom.audio_splitter", OMX_IndexParamAudioInit))
      return false;
  }
  if (CSettings::Get().GetBool("audiooutput.dualaudio") || CSettings::Get().GetString("audiooutput.audiodevice") == "Analogue")
  {
    if(!m_omx_render_analog.Initialize("OMX.broadcom.audio_render", OMX_IndexParamAudioInit))
      return false;
  }
  if (CSettings::Get().GetBool("audiooutput.dualaudio") || CSettings::Get().GetString("audiooutput.audiodevice") == "HDMI")
  {
    if(!m_omx_render_hdmi.Initialize("OMX.broadcom.audio_render", OMX_IndexParamAudioInit))
      return false;
  }

  SetDynamicRangeCompression((long)(CMediaSettings::Get().GetCurrentVideoSettings().m_VolumeAmplification * 100));
  ApplyVolume();

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

    /* mixer output is always stereo */
    m_pcm_output.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    m_pcm_output.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
    m_pcm_output.nChannels = 2;
    /* limit samplerate (through resampling) if requested */
    m_pcm_output.nSamplingRate = std::min((int)m_pcm_output.nSamplingRate, CSettings::Get().GetInt("audiooutput.samplerate"));

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

    omx_err = m_omx_tunnel_clock_analog.Establish(false);
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

    omx_err = m_omx_tunnel_clock_hdmi.Establish(false);
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
    if(CSettings::Get().GetBool("videoplayer.usedisplayasclock"))
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
    if(CSettings::Get().GetBool("videoplayer.usedisplayasclock"))
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
    omx_err = m_omx_tunnel_splitter_analog.Establish(false);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_splitter_analog.Establish 0x%08x", omx_err);
      return false;
    }

    m_omx_tunnel_splitter_hdmi.Initialize(&m_omx_splitter, m_omx_splitter.GetOutputPort() + 1, &m_omx_render_hdmi, m_omx_render_hdmi.GetInputPort());
    omx_err = m_omx_tunnel_splitter_hdmi.Establish(false);
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

  omx_err = m_omx_tunnel_decoder.Establish(false);
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
    omx_err = m_omx_tunnel_mixer.Establish(false);
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

bool COMXAudio::Initialize(AEAudioFormat format, OMXClock *clock, CDVDStreamInfo &hints, bool bUsePassthrough, bool bUseHWDecode)
{
  CSingleLock lock (m_critSection);
  OMX_ERRORTYPE omx_err;

  Deinitialize();

  if(!m_dllAvUtil.Load())
    return false;

  m_HWDecode    = bUseHWDecode;
  m_Passthrough = bUsePassthrough;

  m_format = format;

  if(m_format.m_channelLayout.Count() == 0)
    return false;

  if(hints.samplerate == 0)
    return false;

  m_av_clock = clock;

  if(!m_av_clock)
    return false;

  /* passthrough overwrites hw decode */
  if(m_Passthrough)
  {
    m_HWDecode = false;
  }
  else if(m_HWDecode)
  {
    /* check again if we are capable to hw decode the format */
    m_HWDecode = CanHWDecode(hints.codec);
  }
  SetCodingType(format.m_dataFormat);

  if(hints.extrasize > 0 && hints.extradata != NULL)
  {
    m_extrasize = hints.extrasize;
    m_extradata = (uint8_t *)malloc(m_extrasize);
    memcpy(m_extradata, hints.extradata, hints.extrasize);
  }

  m_omx_clock   = m_av_clock->GetOMXClock();

  m_drc         = 0;

  memset(m_input_channels, 0x0, sizeof(m_input_channels));
  memset(&m_wave_header, 0x0, sizeof(m_wave_header));

  for(int i = 0; i < OMX_AUDIO_MAXCHANNELS; i++)
  {
    m_pcm_input.eChannelMapping[i] = OMX_AUDIO_ChannelNone;
    m_input_channels[i] = OMX_AUDIO_ChannelMax;
  }

  m_input_channels[0] = OMX_AUDIO_ChannelLF;
  m_input_channels[1] = OMX_AUDIO_ChannelRF;
  m_input_channels[2] = OMX_AUDIO_ChannelMax;

  m_wave_header.Format.nChannels  = 2;
  m_wave_header.dwChannelMask     = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;

  if (!m_Passthrough)
  {
    /* setup input channel map */
    int map = 0;
    int chan = 0;

    for (unsigned int ch = 0; ch < m_format.m_channelLayout.Count(); ++ch)
    {
      for(map = 0; map < OMX_MAX_CHANNELS; ++map)
      {
        if (m_format.m_channelLayout[ch] == OMXChannelMap[map])
        {
          m_input_channels[chan] = OMXChannels[map]; 
          m_wave_header.dwChannelMask |= WAVEChannels[map];
          chan++;
          break;
        }
      }
    }

    m_vizRemap.Initialize(m_format.m_channelLayout, CAEChannelInfo(AE_CH_LAYOUT_2_0), false, true);
  }

  OMX_INIT_STRUCTURE(m_pcm_input);

  memcpy(m_pcm_input.eChannelMapping, m_input_channels, sizeof(m_input_channels));

  m_SampleRate    = m_format.m_sampleRate;
  m_BitsPerSample = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_BufferLen     = m_BytesPerSec = m_format.m_sampleRate * (16 >> 3) * m_format.m_channelLayout.Count();
  m_BufferLen     *= AUDIO_BUFFER_SECONDS;
  // the audio_decode output buffer size is 32K, and typically we convert from
  // 6 channel 32bpp float to 8 channel 16bpp in, so a full 48K input buffer will fit the outbut buffer
  m_ChunkLen      = 48*1024;

  m_wave_header.Samples.wSamplesPerBlock    = 0;
  m_wave_header.Format.nChannels            = m_format.m_channelLayout.Count();
  m_wave_header.Format.nBlockAlign          = m_format.m_channelLayout.Count() * 
    (m_BitsPerSample >> 3);
  // 0x8000 is custom format interpreted by GPU as WAVE_FORMAT_IEEE_FLOAT_PLANAR
  m_wave_header.Format.wFormatTag           = m_BitsPerSample == 32 ? 0x8000 : WAVE_FORMAT_PCM;
  m_wave_header.Format.nSamplesPerSec       = m_format.m_sampleRate;
  m_wave_header.Format.nAvgBytesPerSec      = m_BytesPerSec;
  m_wave_header.Format.wBitsPerSample       = m_BitsPerSample;
  m_wave_header.Samples.wValidBitsPerSample = m_BitsPerSample;
  m_wave_header.Format.cbSize               = 0;
  m_wave_header.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;

  m_pcm_input.eNumData              = OMX_NumericalDataSigned;
  m_pcm_input.eEndian               = OMX_EndianLittle;
  m_pcm_input.bInterleaved          = OMX_TRUE;
  m_pcm_input.nBitPerSample         = m_BitsPerSample;
  m_pcm_input.ePCMMode              = OMX_AUDIO_PCMModeLinear;
  m_pcm_input.nChannels             = m_format.m_channelLayout.Count();
  m_pcm_input.nSamplingRate         = m_format.m_sampleRate;

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
    CLog::Log(LOGERROR, "COMXAudio::Initialize error set OMX_IndexParamPortDefinition (intput) omx_err(0x%08x)\n", omx_err);
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
    omx_buffer->nFilledLen = sizeof(m_wave_header);
    if(omx_buffer->nFilledLen > omx_buffer->nAllocLen)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - omx_buffer->nFilledLen > omx_buffer->nAllocLen");
      return false;
    }
    memset((unsigned char *)omx_buffer->pBuffer, 0x0, omx_buffer->nAllocLen);
    memcpy((unsigned char *)omx_buffer->pBuffer, &m_wave_header, omx_buffer->nFilledLen);
    omx_buffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;

    omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
      return false;
    }
  } 
  else if(m_HWDecode)
  {
    // send decoder config
    if(m_extrasize > 0 && m_extradata != NULL)
    {
      OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_decoder.GetInputBuffer();
  
      if(omx_buffer == NULL)
      {
        CLog::Log(LOGERROR, "%s::%s - buffer error 0x%08x", CLASSNAME, __func__, omx_err);
        return false;
      }
  
      omx_buffer->nOffset = 0;
      omx_buffer->nFilledLen = m_extrasize;
      if(omx_buffer->nFilledLen > omx_buffer->nAllocLen)
      {
        CLog::Log(LOGERROR, "%s::%s - omx_buffer->nFilledLen > omx_buffer->nAllocLen", CLASSNAME, __func__);
        return false;
      }

      memset((unsigned char *)omx_buffer->pBuffer, 0x0, omx_buffer->nAllocLen);
      memcpy((unsigned char *)omx_buffer->pBuffer, m_extradata, omx_buffer->nFilledLen);
      omx_buffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
  
      omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
      if (omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
        return false;
      }
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

  CLog::Log(LOGDEBUG, "COMXAudio::Initialize Input bps %d samplerate %d channels %d buffer size %d bytes per second %d",
      (int)m_pcm_input.nBitPerSample, (int)m_pcm_input.nSamplingRate, (int)m_pcm_input.nChannels, m_BufferLen, m_BytesPerSec);
  PrintPCM(&m_pcm_input, std::string("input"));
  CLog::Log(LOGDEBUG, "COMXAudio::Initialize device passthrough %d hwdecode %d",
     m_Passthrough, m_HWDecode);

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

  m_omx_decoder.Deinitialize(true);
  if ( m_omx_mixer.IsInitialized() )
    m_omx_mixer.Deinitialize(true);
  if ( m_omx_splitter.IsInitialized() )
    m_omx_splitter.Deinitialize(true);
  if ( m_omx_render_hdmi.IsInitialized() )
    m_omx_render_hdmi.Deinitialize(true);
  if ( m_omx_render_analog.IsInitialized() )
    m_omx_render_analog.Deinitialize(true);

  m_BytesPerSec = 0;
  m_BufferLen   = 0;

  m_omx_clock = NULL;
  m_av_clock  = NULL;

  m_Initialized = false;
  m_LostSync    = true;
  m_HWDecode    = false;

  if(m_extradata)
    free(m_extradata);
  m_extradata = NULL;
  m_extrasize = 0;

  while(!m_vizqueue.empty())
    m_vizqueue.pop();

  m_dllAvUtil.Unload();

  m_last_pts      = DVD_NOPTS_VALUE;

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

  m_last_pts      = DVD_NOPTS_VALUE;
  m_LostSync      = true;
  m_setStartTime  = true;
}

//***********************************************************************************************
void COMXAudio::SetDynamicRangeCompression(long drc)
{
  CSingleLock lock (m_critSection);
  m_amplification = powf(10.0f, (float)drc / 2000.0f);
  if (m_settings_changed)
    ApplyVolume();
}

//***********************************************************************************************
void COMXAudio::SetMute(bool bMute)
{
  CSingleLock lock (m_critSection);
  m_Mute = bMute;
  if (m_settings_changed)
    ApplyVolume();
}

//***********************************************************************************************
void COMXAudio::SetVolume(float fVolume)
{
  CSingleLock lock (m_critSection);
  m_CurrentVolume = fVolume;
  if (m_settings_changed)
    ApplyVolume();
}

//***********************************************************************************************
bool COMXAudio::ApplyVolume(void)
{
  CSingleLock lock (m_critSection);

  if (!m_Initialized || m_Passthrough)
    return false;

  float fVolume = m_Mute ? VOLUME_MINIMUM : m_CurrentVolume;

  // the analogue volume is too quiet for some. Allow use of an advancedsetting to boost this (at risk of distortion) (deprecated)
  double gain = pow(10, (g_advancedSettings.m_ac3Gain - 12.0f) / 20.0);
  double r = 1.0;
  const float* coeff = downmixing_coefficients_8;

  // alternate coffeciciants that boost centre channel more
  if(!CSettings::Get().GetBool("audiooutput.boostcentre") && m_format.m_channelLayout.Count() > 2)
    coeff = downmixing_coefficients_8_boostcentre;

  // normally we normalise the levels, can be skipped (boosted) at risk of distortion
  if(!CSettings::Get().GetBool("audiooutput.normalizelevels"))
  {
    double sum_L = 0;
    double sum_R = 0;
    for(size_t i = 0; i < OMX_AUDIO_MAXCHANNELS; ++i)
    {
      if (m_input_channels[i] == OMX_AUDIO_ChannelMax)
        break;
      if(i & 1)
        sum_R += coeff[i];
      else
        sum_L += coeff[i];
    }

    r /= max(sum_L, sum_R);
  }
  r *= gain;

  OMX_CONFIG_BRCMAUDIODOWNMIXCOEFFICIENTS mix;
  OMX_INIT_STRUCTURE(mix);
  OMX_ERRORTYPE omx_err;

  assert(sizeof(mix.coeff)/sizeof(mix.coeff[0]) == 16);

  if (m_amplification != 1.0)
  {
    // reduce scaling so overflow can be seen
    for(size_t i = 0; i < 16; ++i)
      mix.coeff[i] = static_cast<unsigned int>(0x10000 * (coeff[i] * r * 0.01f));

    mix.nPortIndex = m_omx_decoder.GetInputPort();
    omx_err = m_omx_decoder.SetConfig(OMX_IndexConfigBrcmAudioDownmixCoefficients, &mix);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - error setting decoder OMX_IndexConfigBrcmAudioDownmixCoefficients, error 0x%08x\n",
                CLASSNAME, __func__, omx_err);
      return false;
    }
  }
  for(size_t i = 0; i < 16; ++i)
    mix.coeff[i] = static_cast<unsigned int>(0x10000 * (coeff[i] * r * fVolume * m_amplification * m_attenuation));

  mix.nPortIndex = m_omx_mixer.GetInputPort();
  omx_err = m_omx_mixer.SetConfig(OMX_IndexConfigBrcmAudioDownmixCoefficients, &mix);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - error setting mixer OMX_IndexConfigBrcmAudioDownmixCoefficients, error 0x%08x\n",
              CLASSNAME, __func__, omx_err);
    return false;
  }
  CLog::Log(LOGINFO, "%s::%s - Volume=%.2f (* %.2f * %.2f)\n", CLASSNAME, __func__, fVolume, m_amplification, m_attenuation);
  return true;
}

void COMXAudio::VizPacket(const void* data, unsigned int len, double pts)
{
    /* input samples */
    unsigned int vizBufferSamples = len / (CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3);

    /* input frames */
    unsigned int frames = vizBufferSamples / m_format.m_channelLayout.Count();
    float *floatBuffer = (float *)data;

    if (m_format.m_dataFormat != AE_FMT_FLOAT)
    {
      CAEConvert::AEConvertToFn m_convertFn = CAEConvert::ToFloat(m_format.m_dataFormat);

      /* check convert buffer */
      CheckOutputBufferSize((void **)&m_vizBuffer, &m_vizBufferSize, vizBufferSamples * (CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3));

      /* convert to float */
      m_convertFn((uint8_t *)data, vizBufferSamples, (float *)m_vizBuffer);
      floatBuffer = (float *)m_vizBuffer;
    }

    // Viz channel count is 2
    CheckOutputBufferSize((void **)&m_vizRemapBuffer, &m_vizRemapBufferSize, frames * 2 * sizeof(float));

    /* remap */
    m_vizRemap.Remap(floatBuffer, (float*)m_vizRemapBuffer, frames);

    /* output samples */
    vizBufferSamples = vizBufferSamples / m_format.m_channelLayout.Count() * 2;

    /* viz size is limited */
    if(vizBufferSamples > VIS_PACKET_SIZE)
      vizBufferSamples = VIS_PACKET_SIZE;

    vizblock_t v;
    v.pts = pts;
    v.num_samples = vizBufferSamples;
    memcpy(v.samples, m_vizRemapBuffer, vizBufferSamples * sizeof(float));
    m_vizqueue.push(v);

    double stamp = m_av_clock->OMXMediaTime();
    while(!m_vizqueue.empty())
    {
      vizblock_t &v = m_vizqueue.front();
      /* if packet has almost reached media time (allow time for rendering delay) then display it */
      /* we'll also consume if queue gets unexpectedly long to avoid filling memory */
      if (v.pts == DVD_NOPTS_VALUE || v.pts - stamp < DVD_SEC_TO_TIME(1.0/30.0) || v.pts - stamp > DVD_SEC_TO_TIME(15.0))
      {
         m_pCallback->OnAudioData(v.samples, v.num_samples);
         m_vizqueue.pop();
      }
      else break;
   }
}


//***********************************************************************************************
unsigned int COMXAudio::AddPackets(const void* data, unsigned int len)
{
  return AddPackets(data, len, 0, 0);
}

//***********************************************************************************************
unsigned int COMXAudio::AddPackets(const void* data, unsigned int len, double dts, double pts)
{
  CSingleLock lock (m_critSection);

  if(!m_Initialized)
  {
    CLog::Log(LOGERROR,"COMXAudio::AddPackets - sanity failed. no valid play handle!");
    return len;
  }

  if (m_pCallback && len && !(m_Passthrough || m_HWDecode))
    VizPacket(data, len, pts);

  if(m_eEncoding == OMX_AUDIO_CodingDTS && m_LostSync && (m_Passthrough || m_HWDecode))
  {
    int skip = SyncDTS((uint8_t *)data, len);
    if(skip > 0)
      return len;
  }

  if(m_eEncoding == OMX_AUDIO_CodingDDP && m_LostSync && (m_Passthrough || m_HWDecode))
  {
    int skip = SyncAC3((uint8_t *)data, len);
    if(skip > 0)
      return len;
  }

  int m_InputChannels = m_format.m_channelLayout.Count();
  unsigned pitch = (m_Passthrough || m_HWDecode) ? 1:(m_BitsPerSample >> 3) * m_InputChannels;
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

    unsigned int remaining = demuxer_samples-demuxer_samples_sent;
    unsigned int samples_space = omx_buffer->nAllocLen/pitch;
    unsigned int samples = std::min(remaining, samples_space);

    omx_buffer->nFilledLen = samples * pitch;

    if (samples < demuxer_samples && m_BitsPerSample==32 && !(m_Passthrough || m_HWDecode))
    {
       uint8_t *dst = omx_buffer->pBuffer;
       uint8_t *src = demuxer_content + demuxer_samples_sent * (m_BitsPerSample >> 3);
       // we need to extract samples from planar audio, so the copying needs to be done per plane
       for (int i=0; i<m_InputChannels; i++)
       {
         memcpy(dst, src, omx_buffer->nFilledLen / m_InputChannels);
         dst += omx_buffer->nFilledLen / m_InputChannels;
         src += demuxer_samples * (m_BitsPerSample >> 3);
       }
       assert(dst <= omx_buffer->pBuffer + m_ChunkLen);
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
          omx_buffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;;
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

    int nRetry = 0;
    while(true)
    {
      omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
      if (omx_err == OMX_ErrorNone)
      {
        //CLog::Log(LOGINFO, "AudiD: dts:%.0f pts:%.0f size:%d\n", dts, pts, len);
        break;
      }
      else
      {
        CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
        nRetry++;
      }
      if(nRetry == 5)
      {
        CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() finaly failed\n", CLASSNAME, __func__);
        return 0;
      }
    }

    omx_err = m_omx_decoder.WaitForEvent(OMX_EventPortSettingsChanged, 0);
    if (omx_err == OMX_ErrorNone)
    {
      if(!PortSettingsChanged())
      {
        CLog::Log(LOGERROR, "%s::%s - error PortSettingsChanged omx_err(0x%08x)\n", CLASSNAME, __func__, omx_err);
      }
    }
  }

  if (m_amplification != 1.0)
  {
    double level_pts = 0.0;
    float level = GetMaxLevel(level_pts);
    if (level_pts != 0.0)
    {
      float alpha_h = -1.0f/(0.025f*log10f(0.999f));
      float alpha_r = -1.0f/(0.100f*log10f(0.900f));
      float hold    = powf(10.0f, -1.0f / (alpha_h * g_advancedSettings.m_limiterHold));
      float release = powf(10.0f, -1.0f / (alpha_r * g_advancedSettings.m_limiterRelease));
      m_maxLevel = level > m_maxLevel ? level : hold * m_maxLevel + (1.0f-hold) * level;

      float amp = m_amplification * m_desired_attenuation;

      // want m_maxLevel * amp -> 1.0
      m_desired_attenuation = std::min(1.0f, std::max(m_desired_attenuation / (amp * m_maxLevel), 1.0f/m_amplification));
      m_attenuation = release * m_attenuation + (1.0f-release) * m_desired_attenuation;

      ApplyVolume();
    }
  }
  return len;
}

//***********************************************************************************************
unsigned int COMXAudio::GetSpace()
{
  int free = m_omx_decoder.GetInputBufferSpace();
  return free;
}

float COMXAudio::GetDelay()
{
  unsigned int free = m_omx_decoder.GetInputBufferSize() - m_omx_decoder.GetInputBufferSpace();
  return m_BytesPerSec ? (float)free / (float)m_BytesPerSec : 0.0f;
}

float COMXAudio::GetCacheTime()
{
  float fBufferLenFull = (float)m_BufferLen - (float)GetSpace();
  if(fBufferLenFull < 0)
    fBufferLenFull = 0;
  float ret = m_BytesPerSec ? fBufferLenFull / (float)m_BytesPerSec : 0.0f;
  return ret;
}

float COMXAudio::GetCacheTotal()
{
  return m_BytesPerSec ? (float)m_BufferLen / (float)m_BytesPerSec : 0.0f;
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

void COMXAudio::RegisterAudioCallback(IAudioCallback *pCallback)
{
  if(!m_Passthrough && !m_HWDecode)
  {
    m_pCallback = pCallback;
    if (m_pCallback)
      m_pCallback->OnInitialize(2, m_SampleRate, 32);
  }
  else
    m_pCallback = NULL;
}

void COMXAudio::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

unsigned int COMXAudio::GetAudioRenderingLatency()
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

void COMXAudio::SetCodingType(AEDataFormat dataFormat)
{
  switch(dataFormat)
  { 
    case AE_FMT_DTS:
      CLog::Log(LOGDEBUG, "COMXAudio::SetCodingType OMX_AUDIO_CodingDTS\n");
      m_eEncoding = OMX_AUDIO_CodingDTS;
      break;
    case AE_FMT_AC3:
    case AE_FMT_EAC3:
      CLog::Log(LOGDEBUG, "COMXAudio::SetCodingType OMX_AUDIO_CodingDDP\n");
      m_eEncoding = OMX_AUDIO_CodingDDP;
      break;
    default:
      CLog::Log(LOGDEBUG, "COMXAudio::SetCodingType OMX_AUDIO_CodingPCM\n");
      m_eEncoding = OMX_AUDIO_CodingPCM;
      break;
  } 
}

bool COMXAudio::CanHWDecode(AVCodecID codec)
{
  bool ret = false;
  switch(codec)
  { 
    case AV_CODEC_ID_DTS:
      CLog::Log(LOGDEBUG, "COMXAudio::CanHWDecode OMX_AUDIO_CodingDTS\n");
      ret = true;
      break;
    case AV_CODEC_ID_AC3:
    case AV_CODEC_ID_EAC3:
      CLog::Log(LOGDEBUG, "COMXAudio::CanHWDecode OMX_AUDIO_CodingDDP\n");
      ret = true;
      break;
    default:
      CLog::Log(LOGDEBUG, "COMXAudio::CanHWDecode OMX_AUDIO_CodingPCM\n");
      ret = false;
      break;
  } 

  return ret;
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

void COMXAudio::PrintDDP(OMX_AUDIO_PARAM_DDPTYPE *ddparm)
{
  CLog::Log(LOGDEBUG, "ddparm->nPortIndex         : %d\n", (int)ddparm->nPortIndex);
  CLog::Log(LOGDEBUG, "ddparm->nChannels          : %d\n", (int)ddparm->nChannels);
  CLog::Log(LOGDEBUG, "ddparm->nBitRate           : %d\n", (int)ddparm->nBitRate);
  CLog::Log(LOGDEBUG, "ddparm->nSampleRate        : %d\n", (int)ddparm->nSampleRate);
  CLog::Log(LOGDEBUG, "ddparm->eBitStreamId       : %d\n", (int)ddparm->eBitStreamId);
  CLog::Log(LOGDEBUG, "ddparm->eBitStreamMode     : %d\n", (int)ddparm->eBitStreamMode);
  CLog::Log(LOGDEBUG, "ddparm->eDolbySurroundMode : %d\n", (int)ddparm->eDolbySurroundMode);

  PrintChannels(ddparm->eChannelMapping);
}

void COMXAudio::PrintDTS(OMX_AUDIO_PARAM_DTSTYPE *dtsparam)
{
  CLog::Log(LOGDEBUG, "dtsparam->nPortIndex         : %d\n", (int)dtsparam->nPortIndex);
  CLog::Log(LOGDEBUG, "dtsparam->nChannels          : %d\n", (int)dtsparam->nChannels);
  CLog::Log(LOGDEBUG, "dtsparam->nBitRate           : %d\n", (int)dtsparam->nBitRate);
  CLog::Log(LOGDEBUG, "dtsparam->nSampleRate        : %d\n", (int)dtsparam->nSampleRate);
  CLog::Log(LOGDEBUG, "dtsparam->nFormat            : 0x%08x\n", (int)dtsparam->nFormat);
  CLog::Log(LOGDEBUG, "dtsparam->nDtsType           : %d\n", (int)dtsparam->nDtsType);
  CLog::Log(LOGDEBUG, "dtsparam->nDtsFrameSizeBytes : %d\n", (int)dtsparam->nDtsFrameSizeBytes);

  PrintChannels(dtsparam->eChannelMapping);
}

/* ========================== SYNC FUNCTIONS ========================== */
unsigned int COMXAudio::SyncDTS(BYTE* pData, unsigned int iSize)
{
  OMX_INIT_STRUCTURE(m_dtsParam);

  unsigned int skip;
  unsigned int srCode;
  unsigned int dtsBlocks;
  bool littleEndian;

  for(skip = 0; iSize - skip > 8; ++skip, ++pData)
  {
    if (pData[0] == 0x7F && pData[1] == 0xFE && pData[2] == 0x80 && pData[3] == 0x01) 
    {
      /* 16bit le */
      littleEndian = true; 
      dtsBlocks    = ((pData[4] >> 2) & 0x7f) + 1;
      m_dtsParam.nFormat = 0x1 | 0x2;
    }
    else if (pData[0] == 0x1F && pData[1] == 0xFF && pData[2] == 0xE8 && pData[3] == 0x00 && pData[4] == 0x07 && (pData[5] & 0xF0) == 0xF0) 
    {
      /* 14bit le */
      littleEndian = true;
      dtsBlocks    = (((pData[4] & 0x7) << 4) | (pData[7] & 0x3C) >> 2) + 1;
      m_dtsParam.nFormat = 0x1 | 0x0;
    }
    else if (pData[1] == 0x7F && pData[0] == 0xFE && pData[3] == 0x80 && pData[2] == 0x01) 
    {
      /* 16bit be */ 
      littleEndian = false;
      dtsBlocks    = ((pData[5] >> 2) & 0x7f) + 1;
      m_dtsParam.nFormat = 0x0 | 0x2;
    }
    else if (pData[1] == 0x1F && pData[0] == 0xFF && pData[3] == 0xE8 && pData[2] == 0x00 && pData[5] == 0x07 && (pData[4] & 0xF0) == 0xF0) 
    {
      /* 14bit be */
      littleEndian = false; 
      dtsBlocks    = (((pData[5] & 0x7) << 4) | (pData[6] & 0x3C) >> 2) + 1;
      m_dtsParam.nFormat = 0x0 | 0x0;
    }
    else
    {
      continue;
    }

    if (littleEndian)
    {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((pData[4] & 0x80) == 0x80 && (pData[4] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      m_dtsParam.nDtsFrameSizeBytes = ((((pData[5] & 0x3) << 8 | pData[6]) << 4) | ((pData[7] & 0xF0) >> 4)) + 1;
      srCode = (pData[8] & 0x3C) >> 2;
   }
   else
   {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((pData[5] & 0x80) == 0x80 && (pData[5] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      m_dtsParam.nDtsFrameSizeBytes = ((((pData[4] & 0x3) << 8 | pData[7]) << 4) | ((pData[6] & 0xF0) >> 4)) + 1;
      srCode = (pData[9] & 0x3C) >> 2;
   }

    /* make sure the framesize is sane */
    if (m_dtsParam.nDtsFrameSizeBytes < 96 || m_dtsParam.nDtsFrameSizeBytes > 16384)
      continue;

    m_dtsParam.nSampleRate = DTSFSCod[srCode];

    switch(dtsBlocks << 5)
    {
      case 512 : 
        m_dtsParam.nDtsType = 1;
        break;
      case 1024: 
        m_dtsParam.nDtsType = 2;
        break;
      case 2048: 
        m_dtsParam.nDtsType = 3;
        break;
      default:
        m_dtsParam.nDtsType = 0;
        break;
    }

    //m_dtsParam.nFormat = 1;
    m_dtsParam.nDtsType = 1;

    m_LostSync = false;

    return skip;
  }

  m_LostSync = true;
  return iSize;
}

unsigned int COMXAudio::SyncAC3(BYTE* pData, unsigned int iSize)
{
  unsigned int skip = 0;

  for(skip = 0; iSize - skip > 6; ++skip, ++pData)
  {
    /* search for an ac3 sync word */
    if(pData[0] != 0x0b || pData[1] != 0x77)
      continue;

    uint8_t fscod      = pData[4] >> 6;
    uint8_t frmsizecod = pData[4] & 0x3F;
    uint8_t bsid       = pData[5] >> 3;

    /* sanity checks on the header */
    if (
        fscod      ==   3 ||
        frmsizecod >   37 ||
        bsid       > 0x11
    ) continue;

    /* get the details we need to check crc1 and framesize */
    uint16_t     bitrate   = AC3Bitrates[frmsizecod >> 1];
    unsigned int framesize = 0;
    switch(fscod)
    {
      case 0: framesize = bitrate * 2; break;
      case 1: framesize = (320 * bitrate / 147 + (frmsizecod & 1 ? 1 : 0)); break;
      case 2: framesize = bitrate * 4; break;
    }

    m_SampleRate = AC3FSCod[fscod];

    /* dont do extensive testing if we have not lost sync */
    if (!m_LostSync && skip == 0)
      return 0;

    unsigned int crc_size;
    /* if we have enough data, validate the entire packet, else try to validate crc2 (5/8 of the packet) */
    if (framesize <= iSize - skip)
         crc_size = framesize - 1;
    else crc_size = (framesize >> 1) + (framesize >> 3) - 1;

    if (crc_size <= iSize - skip)
      if(m_dllAvUtil.av_crc(m_dllAvUtil.av_crc_get_table(AV_CRC_16_ANSI), 0, &pData[2], crc_size * 2))
        continue;

    /* if we get here, we can sync */
    m_LostSync = false;
    return skip;
  }

  /* if we get here, the entire packet is invalid and we have lost sync */
  m_LostSync = true;
  return iSize;
}

void COMXAudio::CheckOutputBufferSize(void **buffer, int *oldSize, int newSize)
{
  if (newSize > *oldSize)
  {
    if (*buffer)
      _aligned_free(*buffer);
    *buffer = _aligned_malloc(newSize, 16);
    *oldSize = newSize;
  }
  memset(*buffer, 0x0, *oldSize);
}

