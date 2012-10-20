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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#include "OMXAudio.h"
#include "utils/log.h"

#define CLASSNAME "COMXAudio"

#include "linux/XMemUtils.h"

#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
#include "cores/AudioEngine/Utils/AEConvert.h"

#ifndef VOLUME_MINIMUM
#define VOLUME_MINIMUM -6000  // -60dB
#endif

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
  SPEAKER_BACK_CENTER,      SPEAKER_SIDE_RIGHT
};

static const uint16_t AC3Bitrates[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod   [] = {48000, 44100, 32000, 0};

static const uint16_t DTSFSCod   [] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
COMXAudio::COMXAudio() :
  m_pCallback       (NULL   ),
  m_Initialized     (false  ),
  m_Pause           (false  ),
  m_CanPause        (false  ),
  m_CurrentVolume   (0      ),
  m_Passthrough     (false  ),
  m_HWDecode        (false  ),
  m_BytesPerSec     (0      ),
  m_BufferLen       (0      ),
  m_ChunkLen        (0      ),
  m_OutputChannels  (0      ),
  m_BitsPerSample   (0      ),
  m_omx_clock       (NULL   ),
  m_av_clock        (NULL   ),
  m_external_clock  (false  ),
  m_first_frame     (true   ),
  m_LostSync        (true   ),
  m_SampleRate      (0      ),
  m_eEncoding       (OMX_AUDIO_CodingPCM),
  m_extradata       (NULL   ),
  m_extrasize       (0      ),
  m_omx_render      (NULL   ),
  m_last_pts        (DVD_NOPTS_VALUE)
{
  m_vizBufferSize   = m_vizRemapBufferSize = VIS_PACKET_SIZE * sizeof(float);
  m_vizRemapBuffer  = (uint8_t *)_aligned_malloc(m_vizRemapBufferSize,16);
  m_vizBuffer       = (uint8_t *)_aligned_malloc(m_vizBufferSize,16);
}

COMXAudio::~COMXAudio()
{
  if(m_Initialized)
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

bool COMXAudio::Initialize(AEAudioFormat format, std::string& device, OMXClock *clock, CDVDStreamInfo &hints, bool bUsePassthrough, bool bUseHWDecode)
{
  m_HWDecode    = bUseHWDecode;
  m_Passthrough = bUsePassthrough;

  m_format = format;

  if(hints.samplerate == 0)
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

  SetClock(clock);

  if(hints.extrasize > 0 && hints.extradata != NULL)
  {
    m_extrasize = hints.extrasize;
    m_extradata = (uint8_t *)malloc(m_extrasize);
    memcpy(m_extradata, hints.extradata, hints.extrasize);
  }

  return Initialize(format, device);
}

bool COMXAudio::Initialize(AEAudioFormat format, std::string& device)
{
  if(m_Initialized)
    Deinitialize();

  m_format = format;

  if(m_format.m_channelLayout.Count() == 0)
    return false;

  if(!m_dllAvUtil.Load())
    return false;

  if(m_av_clock == NULL)
  {
    /* no external clock set. generate one */
    m_external_clock = false;

    m_av_clock = new OMXClock();
    
    if(!m_av_clock->OMXInitialize(false, true))
    {
      delete m_av_clock;
      m_av_clock = NULL;
      CLog::Log(LOGERROR, "COMXAudio::Initialize error creating av clock\n");
      return false;
    }
  }

  m_omx_clock = m_av_clock->GetOMXClock();

  /*
  m_Passthrough = false;

  if(OMX_IS_RAW(m_format.m_dataFormat))
    m_Passthrough =true;
  */

  m_drc         = 0;

  m_CurrentVolume = g_settings.m_fVolumeLevel; 

  memset(m_input_channels, 0x0, sizeof(m_input_channels));
  memset(m_output_channels, 0x0, sizeof(m_output_channels));
  memset(&m_wave_header, 0x0, sizeof(m_wave_header));

  for(int i = 0; i < OMX_AUDIO_MAXCHANNELS; i++)
    m_pcm_input.eChannelMapping[i] = OMX_AUDIO_ChannelNone;

  m_output_channels[0] = OMX_AUDIO_ChannelLF;
  m_output_channels[1] = OMX_AUDIO_ChannelRF;
  m_output_channels[2] = OMX_AUDIO_ChannelMax;

  m_input_channels[0] = OMX_AUDIO_ChannelLF;
  m_input_channels[1] = OMX_AUDIO_ChannelRF;
  m_input_channels[2] = OMX_AUDIO_ChannelMax;

  m_OutputChannels                = 2;
  m_wave_header.Format.nChannels  = m_OutputChannels;
  m_wave_header.dwChannelMask     = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;

  if (!m_Passthrough)
  {
    /* setup output channel map */
    /*
    int ch = 0, map;
    int chan = 0;
    m_OutputChannels = 0;

    for (unsigned int ch = 0; ch < m_format.m_channelLayout.Count(); ++ch)
    {
      for(map = 0; map < OMX_MAX_CHANNELS; ++map)
      {
        if (m_output_channels[ch] == OMXChannelMap[map])
        {
          printf("output %d\n", chan);
          m_output_channels[chan] = OMXChannels[map]; 
          chan++;
          break;
        }
      }
    }

    m_OutputChannels = chan;
    */

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

  OMX_INIT_STRUCTURE(m_pcm_output);
  OMX_INIT_STRUCTURE(m_pcm_input);

  memcpy(m_pcm_output.eChannelMapping, m_output_channels, sizeof(m_output_channels));
  memcpy(m_pcm_input.eChannelMapping, m_input_channels, sizeof(m_input_channels));

  // set the m_pcm_output parameters
  m_pcm_output.eNumData            = OMX_NumericalDataSigned;
  m_pcm_output.eEndian             = OMX_EndianLittle;
  m_pcm_output.bInterleaved        = OMX_TRUE;
  m_pcm_output.nBitPerSample       = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_pcm_output.ePCMMode            = OMX_AUDIO_PCMModeLinear;
  m_pcm_output.nChannels           = m_OutputChannels;
  m_pcm_output.nSamplingRate       = m_format.m_sampleRate;

  m_SampleRate    = m_format.m_sampleRate;
  m_BitsPerSample = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_BufferLen     = m_BytesPerSec = m_format.m_sampleRate * 
    (CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3) * 
    m_format.m_channelLayout.Count();
  m_BufferLen     *= AUDIO_BUFFER_SECONDS;
  m_ChunkLen      = 6144;

  m_wave_header.Samples.wSamplesPerBlock    = 0;
  m_wave_header.Format.nChannels            = m_format.m_channelLayout.Count();
  m_wave_header.Format.nBlockAlign          = m_format.m_channelLayout.Count() * 
    (CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3);
  m_wave_header.Format.wFormatTag           = WAVE_FORMAT_PCM;
  m_wave_header.Format.nSamplesPerSec       = m_format.m_sampleRate;
  m_wave_header.Format.nAvgBytesPerSec      = m_BytesPerSec;
  m_wave_header.Format.wBitsPerSample       = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_wave_header.Samples.wValidBitsPerSample = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_wave_header.Format.cbSize               = 0;
  m_wave_header.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;

  m_pcm_input.eNumData              = OMX_NumericalDataSigned;
  m_pcm_input.eEndian               = OMX_EndianLittle;
  m_pcm_input.bInterleaved          = OMX_TRUE;
  m_pcm_input.nBitPerSample         = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  m_pcm_input.ePCMMode              = OMX_AUDIO_PCMModeLinear;
  m_pcm_input.nChannels             = m_format.m_channelLayout.Count();
  m_pcm_input.nSamplingRate         = m_format.m_sampleRate;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  std::string componentName = "";

  componentName = "OMX.broadcom.audio_render";
  m_omx_render = new COMXCoreComponent();
  if(!m_omx_render)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize error allocate OMX.broadcom.audio_render\n");
    return false;
  }

  if(!m_omx_render->Initialize((const std::string)componentName, OMX_IndexParamAudioInit))
    return false;

  OMX_CONFIG_BRCMAUDIODESTINATIONTYPE audioDest;
  OMX_INIT_STRUCTURE(audioDest);
  strncpy((char *)audioDest.sName, device.c_str(), strlen(device.c_str()));

  omx_err = m_omx_render->SetConfig(OMX_IndexConfigBrcmAudioDestination, &audioDest);
  if (omx_err != OMX_ErrorNone)
    return false;

  OMX_CONFIG_BOOLEANTYPE configBool;
  OMX_INIT_STRUCTURE(configBool);
  configBool.bEnabled = OMX_FALSE;

  omx_err = m_omx_render->SetConfig(OMX_IndexConfigBrcmClockReferenceSource, &configBool);
  if (omx_err != OMX_ErrorNone)
    return false;

  componentName = "OMX.broadcom.audio_decode";
  if(!m_omx_decoder.Initialize((const std::string)componentName, OMX_IndexParamAudioInit))
    return false;

  if(!m_Passthrough)
  {
    componentName = "OMX.broadcom.audio_mixer";
    if(!m_omx_mixer.Initialize((const std::string)componentName, OMX_IndexParamAudioInit))
      return false;
  }

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

  // set up the number/size of buffers
  OMX_PARAM_PORTDEFINITIONTYPE port_param;
  OMX_INIT_STRUCTURE(port_param);
  port_param.nPortIndex = m_omx_decoder.GetInputPort();

  omx_err = m_omx_decoder.GetParameter(OMX_IndexParamPortDefinition, &port_param);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize error get OMX_IndexParamPortDefinition omx_err(0x%08x)\n", omx_err);
    return false;
  }

  port_param.format.audio.eEncoding = m_eEncoding;

  port_param.nBufferSize = m_ChunkLen;
  port_param.nBufferCountActual = m_BufferLen / m_ChunkLen;

  omx_err = m_omx_decoder.SetParameter(OMX_IndexParamPortDefinition, &port_param);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize error set OMX_IndexParamPortDefinition omx_err(0x%08x)\n", omx_err);
    return false;
  }

  //if(m_HWDecode)
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

  m_omx_tunnel_clock.Initialize(m_omx_clock, m_omx_clock->GetInputPort(), m_omx_render, m_omx_render->GetInputPort()+1);

  omx_err = m_omx_tunnel_clock.Establish(false);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize m_omx_tunnel_clock.Establish\n");
    return false;
  }

  if(!m_external_clock)
  {
    omx_err = m_omx_clock->SetStateForComponent(OMX_StateExecuting);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize m_omx_clock.SetStateForComponent\n");
      return false;
    }
  }

  omx_err = m_omx_decoder.AllocInputBuffers();
  if(omx_err != OMX_ErrorNone) 
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize - Error alloc buffers 0x%08x", omx_err);
    return false;
  }

  if(!m_Passthrough)
  {
    m_omx_tunnel_decoder.Initialize(&m_omx_decoder, m_omx_decoder.GetOutputPort(), &m_omx_mixer, m_omx_mixer.GetInputPort());
    omx_err = m_omx_tunnel_decoder.Establish(false);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_decoder.Establish 0x%08x", omx_err);
      return false;
    }
  
    omx_err = m_omx_decoder.SetStateForComponent(OMX_StateExecuting);
    if(omx_err != OMX_ErrorNone) {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error setting OMX_StateExecuting 0x%08x", omx_err);
      return false;
    }

    m_omx_tunnel_mixer.Initialize(&m_omx_mixer, m_omx_mixer.GetOutputPort(), m_omx_render, m_omx_render->GetInputPort());
    omx_err = m_omx_tunnel_mixer.Establish(false);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_decoder.Establish 0x%08x", omx_err);
      return false;
    }
  
    omx_err = m_omx_mixer.SetStateForComponent(OMX_StateExecuting);
    if(omx_err != OMX_ErrorNone) {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error setting OMX_StateExecuting 0x%08x", omx_err);
      return false;
    }
  }
  else
  {
    m_omx_tunnel_decoder.Initialize(&m_omx_decoder, m_omx_decoder.GetOutputPort(), m_omx_render, m_omx_render->GetInputPort());
    omx_err = m_omx_tunnel_decoder.Establish(false);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_decoder.Establish 0x%08x", omx_err);
      return false;
    }
  
    omx_err = m_omx_decoder.SetStateForComponent(OMX_StateExecuting);
    if(omx_err != OMX_ErrorNone) {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error setting OMX_StateExecuting 0x%08x", omx_err);
      return false;
    }
  }

  omx_err = m_omx_render->SetStateForComponent(OMX_StateExecuting);
  if(omx_err != OMX_ErrorNone) 
  {
    CLog::Log(LOGERROR, "COMXAudio::Initialize - Error setting OMX_StateExecuting 0x%08x", omx_err);
    return false;
  }

  m_omx_decoder.EnablePort(m_omx_decoder.GetInputPort(), true);

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

  m_Initialized   = true;
  m_first_frame   = true;
  m_last_pts      = DVD_NOPTS_VALUE;

  SetCurrentVolume(m_CurrentVolume);

  CLog::Log(LOGDEBUG, "COMXAudio::Initialize Ouput bps %d samplerate %d channels %d buffer size %d bytes per second %d", 
      (int)m_pcm_output.nBitPerSample, (int)m_pcm_output.nSamplingRate, (int)m_pcm_output.nChannels, m_BufferLen, m_BytesPerSec);
  CLog::Log(LOGDEBUG, "COMXAudio::Initialize Input bps %d samplerate %d channels %d buffer size %d bytes per second %d", 
      (int)m_pcm_input.nBitPerSample, (int)m_pcm_input.nSamplingRate, (int)m_pcm_input.nChannels, m_BufferLen, m_BytesPerSec);
  CLog::Log(LOGDEBUG, "COMXAudio::Initialize device %s passthrough %d hwdecode %d external clock %d", 
      device.c_str(), m_Passthrough, m_HWDecode, m_external_clock);

  m_av_clock->OMXStateExecute(false);

  return true;
}

//***********************************************************************************************
bool COMXAudio::Deinitialize()
{
  if(!m_Initialized)
    return true;

  CSingleLock lock (m_critSection);

  if(m_av_clock && !m_external_clock)
  {
    m_av_clock->Lock();
    m_av_clock->OMXStop(false);
  }

  m_omx_tunnel_decoder.Flush();
  if(!m_Passthrough)
    m_omx_tunnel_mixer.Flush();
  m_omx_tunnel_clock.Flush();

  m_omx_tunnel_clock.Deestablish();
  if(!m_Passthrough)
    m_omx_tunnel_mixer.Deestablish();
  m_omx_tunnel_decoder.Deestablish();

  m_omx_decoder.FlushInput();

  m_omx_render->Deinitialize();
  if(!m_Passthrough)
    m_omx_mixer.Deinitialize();
  m_omx_decoder.Deinitialize();

  m_BytesPerSec = 0;
  m_BufferLen   = 0;

  if(m_av_clock && !m_external_clock)
  {
    m_av_clock->OMXReset(false);
    m_av_clock->UnLock();
    delete m_av_clock;
    m_external_clock = false;
  }

  m_omx_clock = NULL;
  m_av_clock  = NULL;

  m_Initialized = false;
  m_LostSync    = true;
  m_HWDecode    = false;

  if(m_extradata)
    free(m_extradata);
  m_extradata = NULL;
  m_extrasize = 0;

  m_dllAvUtil.Unload();

  m_first_frame   = true;
  m_last_pts      = DVD_NOPTS_VALUE;

  delete m_omx_render;
  m_omx_render = NULL;

  return true;
}

void COMXAudio::Flush()
{
  if(!m_Initialized)
    return;

  m_omx_decoder.FlushInput();
  m_omx_tunnel_decoder.Flush();
  if(!m_Passthrough)
    m_omx_tunnel_mixer.Flush();
  
  m_last_pts      = DVD_NOPTS_VALUE;
  m_LostSync      = true;
  //m_first_frame   = true;
}

//***********************************************************************************************
bool COMXAudio::Pause()
{
  if (!m_Initialized)
     return -1;

  if(m_Pause) return true;
  m_Pause = true;

  m_omx_decoder.SetStateForComponent(OMX_StatePause);

  return true;
}

//***********************************************************************************************
bool COMXAudio::Resume()
{
  if (!m_Initialized)
     return -1;

  if(!m_Pause) return true;
  m_Pause = false;

  m_omx_decoder.SetStateForComponent(OMX_StateExecuting);

  return true;
}

//***********************************************************************************************
bool COMXAudio::Stop()
{
  if (!m_Initialized)
     return -1;

  Flush();

  m_Pause = false;

  return true;
}

//***********************************************************************************************
long COMXAudio::GetCurrentVolume() const
{
  return m_CurrentVolume;
}

//***********************************************************************************************
void COMXAudio::Mute(bool bMute)
{
  if(!m_Initialized)
    return;

  if (bMute)
    SetCurrentVolume(VOLUME_MINIMUM);
  else
    SetCurrentVolume(m_CurrentVolume);
}

//***********************************************************************************************
bool COMXAudio::SetCurrentVolume(float fVolume)
{
  CSingleLock lock (m_critSection);

  if(!m_Initialized || m_Passthrough)
    return -1;

  m_CurrentVolume = fVolume;

  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_INIT_STRUCTURE(volume);
  volume.nPortIndex = m_omx_render->GetInputPort();

  volume.bLinear    = OMX_TRUE;
  float hardwareVolume = std::max(VOLUME_MINIMUM, std::min(VOLUME_MAXIMUM, fVolume)) * 100.0f;
  volume.sVolume.nValue = (int)hardwareVolume;

  m_omx_render->SetConfig(OMX_IndexConfigAudioVolume, &volume);

  return true;
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

  m_vizBufferSamples = 0;

  if (m_pCallback && len)
  {
    /* input samples */
    m_vizBufferSamples = len / (CAEUtil::DataFormatToBits(AE_FMT_S16LE) >> 3);
    CAEConvert::AEConvertToFn m_convertFn = CAEConvert::ToFloat(AE_FMT_S16LE);
    /* input frames */
    unsigned int frames = m_vizBufferSamples / m_format.m_channelLayout.Count();

    /* check convert buffer */
    CheckOutputBufferSize((void **)&m_vizBuffer, &m_vizBufferSize, m_vizBufferSamples * (CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3));

    /* convert to float */
    m_convertFn((uint8_t *)data, m_vizBufferSamples, (float *)m_vizBuffer);

    /* check remap buffer */
    CheckOutputBufferSize((void **)&m_vizRemapBuffer, &m_vizRemapBufferSize, frames * 2 * (CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3));

    /* remap */
    m_vizRemap.Remap((float *)m_vizBuffer, (float*)m_vizRemapBuffer, frames);

    /* output samples */
    m_vizBufferSamples = m_vizBufferSamples / m_format.m_channelLayout.Count() * 2;

    /* viz size is limited */
    if(m_vizBufferSamples > VIS_PACKET_SIZE)
      m_vizBufferSamples = VIS_PACKET_SIZE;

    if(m_pCallback)
      m_pCallback->OnAudioData((float *)m_vizRemapBuffer, m_vizBufferSamples);
  }

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

  unsigned int demuxer_bytes = (unsigned int)len;
  uint8_t *demuxer_content = (uint8_t *)data;

  OMX_ERRORTYPE omx_err;

  OMX_BUFFERHEADERTYPE *omx_buffer = NULL;

  while(demuxer_bytes)
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

    omx_buffer->nFilledLen = (demuxer_bytes > omx_buffer->nAllocLen) ? omx_buffer->nAllocLen : demuxer_bytes;
    memcpy(omx_buffer->pBuffer, demuxer_content, omx_buffer->nFilledLen);

    uint64_t val  = (uint64_t)(pts == DVD_NOPTS_VALUE) ? 0 : pts;

    if(m_av_clock->AudioStart())
    {
      omx_buffer->nFlags = OMX_BUFFERFLAG_STARTTIME;

      m_last_pts = pts;

      CLog::Log(LOGDEBUG, "ADec : setStartTime %f\n", (float)val / DVD_TIME_BASE);
      m_av_clock->AudioStart(false);
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

    demuxer_bytes -= omx_buffer->nFilledLen;
    demuxer_content += omx_buffer->nFilledLen;

    if(demuxer_bytes == 0)
      omx_buffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

    int nRetry = 0;
    while(true)
    {
      omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
      if (omx_err == OMX_ErrorNone)
      {
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

    if(m_first_frame)
    {
      m_first_frame = false;
      //m_omx_render.WaitForEvent(OMX_EventPortSettingsChanged);

      m_omx_render->DisablePort(m_omx_render->GetInputPort(), false);
      if(!m_Passthrough)
      {
        m_omx_mixer.DisablePort(m_omx_mixer.GetOutputPort(), false);
        m_omx_mixer.DisablePort(m_omx_mixer.GetInputPort(), false);
      }
      m_omx_decoder.DisablePort(m_omx_decoder.GetOutputPort(), false);

      if(!m_Passthrough)
      {
        if(m_HWDecode)
        {
          OMX_INIT_STRUCTURE(m_pcm_input);
          m_pcm_input.nPortIndex      = m_omx_decoder.GetOutputPort();
          omx_err = m_omx_decoder.GetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
          if(omx_err != OMX_ErrorNone)
          {
            CLog::Log(LOGERROR, "COMXAudio::AddPackets error GetParameter 1 omx_err(0x%08x)\n", omx_err);
          }
        }

        if ((m_pcm_input.nChannels > m_pcm_output.nChannels) &&g_guiSettings.GetBool("audiooutput.normalizelevels"))
        {
          OMX_AUDIO_CONFIG_VOLUMETYPE volume;
          OMX_INIT_STRUCTURE(volume);
          volume.nPortIndex = m_omx_mixer.GetInputPort();
          volume.bLinear    = OMX_FALSE;
          volume.sVolume.nValue = (int)(g_advancedSettings.m_ac3Gain*100.0f+0.5f);
          m_omx_mixer.SetConfig(OMX_IndexConfigAudioVolume, &volume);
        }

        memcpy(m_pcm_input.eChannelMapping, m_input_channels, sizeof(m_input_channels));
        m_pcm_input.nSamplingRate = m_format.m_sampleRate;

        /* setup mixer input */
        m_pcm_input.nPortIndex      = m_omx_mixer.GetInputPort();
        omx_err = m_omx_mixer.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error SetParameter 1 input omx_err(0x%08x)\n", omx_err);
        }
        omx_err = m_omx_mixer.GetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error GetParameter 2 input omx_err(0x%08x)\n", omx_err);
        }

        m_pcm_output.nSamplingRate = m_format.m_sampleRate;

        /* setup mixer output */
        m_pcm_output.nPortIndex      = m_omx_mixer.GetOutputPort();
        omx_err = m_omx_mixer.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error SetParameter 1 output omx_err(0x%08x)\n", omx_err);
        }
        omx_err = m_omx_mixer.GetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error GetParameter 2 output omx_err(0x%08x)\n", omx_err);
        }

        m_pcm_output.nSamplingRate = m_format.m_sampleRate;

        m_pcm_output.nPortIndex      = m_omx_render->GetInputPort();
        omx_err = m_omx_render->SetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error SetParameter 1 render omx_err(0x%08x)\n", omx_err);
        }
        omx_err = m_omx_render->GetParameter(OMX_IndexParamAudioPcm, &m_pcm_output);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error GetParameter 2 render omx_err(0x%08x)\n", omx_err);
        }

        PrintPCM(&m_pcm_input, std::string("input"));
        PrintPCM(&m_pcm_output, std::string("output"));
      }
      else
      {
        OMX_AUDIO_PARAM_PORTFORMATTYPE formatType;
        OMX_INIT_STRUCTURE(formatType);
        formatType.nPortIndex = m_omx_render->GetInputPort();

        omx_err = m_omx_render->GetParameter(OMX_IndexParamAudioPortFormat, &formatType);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error OMX_IndexParamAudioPortFormat omx_err(0x%08x)\n", omx_err);
          assert(0);
        }

        formatType.eEncoding = m_eEncoding;

        omx_err = m_omx_render->SetParameter(OMX_IndexParamAudioPortFormat, &formatType);
        if(omx_err != OMX_ErrorNone)
        {
          CLog::Log(LOGERROR, "COMXAudio::AddPackets error OMX_IndexParamAudioPortFormat omx_err(0x%08x)\n", omx_err);
          assert(0);
        }

        if(m_eEncoding == OMX_AUDIO_CodingDDP)
        {
          OMX_AUDIO_PARAM_DDPTYPE m_ddParam;
          OMX_INIT_STRUCTURE(m_ddParam);

          m_ddParam.nPortIndex      = m_omx_render->GetInputPort();

          m_ddParam.nChannels       = m_format.m_channelLayout.Count(); //(m_InputChannels == 6) ? 8 : m_InputChannels;
          m_ddParam.nSampleRate     = m_SampleRate;
          m_ddParam.eBitStreamId    = OMX_AUDIO_DDPBitStreamIdAC3;
          m_ddParam.nBitRate        = 0;

          for(unsigned int i = 0; i < OMX_MAX_CHANNELS; i++)
          {
            if(i >= m_ddParam.nChannels)
              break;

            m_ddParam.eChannelMapping[i] = OMXChannels[i];
          }
  
          m_omx_render->SetParameter(OMX_IndexParamAudioDdp, &m_ddParam);
          m_omx_render->GetParameter(OMX_IndexParamAudioDdp, &m_ddParam);
          PrintDDP(&m_ddParam);
        }
        else if(m_eEncoding == OMX_AUDIO_CodingDTS)
        {
          m_dtsParam.nPortIndex      = m_omx_render->GetInputPort();

          m_dtsParam.nChannels       = m_format.m_channelLayout.Count(); //(m_InputChannels == 6) ? 8 : m_InputChannels;
          m_dtsParam.nBitRate        = 0;

          for(unsigned int i = 0; i < OMX_MAX_CHANNELS; i++)
          {
            if(i >= m_dtsParam.nChannels)
              break;

            m_dtsParam.eChannelMapping[i] = OMXChannels[i];
          }
  
          m_omx_render->SetParameter(OMX_IndexParamAudioDts, &m_dtsParam);
          m_omx_render->GetParameter(OMX_IndexParamAudioDts, &m_dtsParam);
          PrintDTS(&m_dtsParam);
        }
      }

      m_omx_render->EnablePort(m_omx_render->GetInputPort(), false);
      if(!m_Passthrough)
      {
        m_omx_mixer.EnablePort(m_omx_mixer.GetOutputPort(), false);
        m_omx_mixer.EnablePort(m_omx_mixer.GetInputPort(), false);
      }
      m_omx_decoder.EnablePort(m_omx_decoder.GetOutputPort(), false);
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
  return (float)free / (float)m_BytesPerSec;
}

float COMXAudio::GetCacheTime()
{
  float fBufferLenFull = (float)m_BufferLen - (float)GetSpace();
  if(fBufferLenFull < 0)
    fBufferLenFull = 0;
  float ret = fBufferLenFull / (float)m_BytesPerSec;
  return ret;
}

float COMXAudio::GetCacheTotal()
{
  return (float)m_BufferLen / (float)m_BytesPerSec;
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
  m_vizBufferSamples = 0;
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
  m_vizBufferSamples = 0;
}

unsigned int COMXAudio::GetAudioRenderingLatency()
{
  if(!m_Initialized)
    return 0;

  OMX_PARAM_U32TYPE param;
  OMX_INIT_STRUCTURE(param);
  param.nPortIndex = m_omx_render->GetInputPort();

  OMX_ERRORTYPE omx_err =
    m_omx_render->GetConfig(OMX_IndexConfigAudioRenderingLatency, &param);

  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - error getting OMX_IndexConfigAudioRenderingLatency error 0x%08x\n",
      CLASSNAME, __func__, omx_err);
    return 0;
  }

  return param.nU32;
}

void COMXAudio::WaitCompletion()
{
  CSingleLock lock (m_critSection);

  if(!m_Initialized || m_Pause)
    return;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_decoder.GetInputBuffer();

  if(omx_buffer == NULL)
  {
    CLog::Log(LOGERROR, "%s::%s - buffer error 0x%08x", CLASSNAME, __func__, omx_err);
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

  unsigned int nTimeOut = AUDIO_BUFFER_SECONDS * 1000;
  while(nTimeOut)
  {
    if(m_omx_render->IsEOS())
    {
      CLog::Log(LOGDEBUG, "%s::%s - got eos\n", CLASSNAME, __func__);
      break;
    }

    if(nTimeOut == 0)
    {
      CLog::Log(LOGERROR, "%s::%s - wait for eos timed out\n", CLASSNAME, __func__);
      break;
    }
    Sleep(50);
    nTimeOut -= 50;
  }

  nTimeOut = AUDIO_BUFFER_SECONDS * 1000;
  while(nTimeOut)
  {
    if(!GetAudioRenderingLatency())
      break;

    if(nTimeOut == 0)
    {
      CLog::Log(LOGERROR, "%s::%s - wait for GetAudioRenderingLatency timed out\n", CLASSNAME, __func__);
      break;
    }
    Sleep(50);
    nTimeOut -= 50;
  }

  return;
}

void COMXAudio::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}

bool COMXAudio::SetClock(OMXClock *clock)
{
  if(m_av_clock != NULL)
    return false;

  m_av_clock = clock;
  m_external_clock = true;
  return true;
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

bool COMXAudio::CanHWDecode(CodecID codec)
{
  bool ret = false;
  switch(codec)
  { 
    case CODEC_ID_DTS:
      CLog::Log(LOGDEBUG, "COMXAudio::CanHWDecode OMX_AUDIO_CodingDTS\n");
      ret = true;
      break;
    case CODEC_ID_AC3:
    case CODEC_ID_EAC3:
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
  //unsigned int fSize = 0;

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

    //fSize = framesize * 2;
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

