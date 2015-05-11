/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "system.h"

#if defined(TARGET_RASPBERRY_PI)

#include <stdint.h>
#include <limits.h>
#include <cassert>

#include "AESinkPi.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "linux/RBP.h"

#define CLASSNAME "CAESinkPi"

#define NUM_OMX_BUFFERS 2
#define AUDIO_PLAYBUFFER (0.1) // 100ms

static const unsigned int PassthroughSampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };

CAEDeviceInfo CAESinkPi::m_info;

CAESinkPi::CAESinkPi() :
    m_sinkbuffer_sec_per_byte(0),
    m_Initialized(false),
    m_submitted(0),
    m_omx_output(NULL),
    m_output(AESINKPI_UNKNOWN)
{
}

CAESinkPi::~CAESinkPi()
{
}

void CAESinkPi::SetAudioDest()
{
  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;
  OMX_CONFIG_BRCMAUDIODESTINATIONTYPE audioDest;
  OMX_INIT_STRUCTURE(audioDest);
  if ( m_omx_render.IsInitialized() )
  {
    if (m_output == AESINKPI_ANALOGUE)
      strncpy((char *)audioDest.sName, "local", strlen("local"));
    else
      strncpy((char *)audioDest.sName, "hdmi", strlen("hdmi"));
    omx_err = m_omx_render.SetConfig(OMX_IndexConfigBrcmAudioDestination, &audioDest);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - m_omx_render.SetConfig omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }
  if ( m_omx_render_slave.IsInitialized() )
  {
    if (m_output != AESINKPI_ANALOGUE)
      strncpy((char *)audioDest.sName, "local", strlen("local"));
    else
      strncpy((char *)audioDest.sName, "hdmi", strlen("hdmi"));
    omx_err = m_omx_render_slave.SetConfig(OMX_IndexConfigBrcmAudioDestination, &audioDest);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - m_omx_render_slave.SetConfig omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }
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

bool CAESinkPi::Initialize(AEAudioFormat &format, std::string &device)
{
  // This may be called before Application calls g_RBP.Initialise, so call it here too
  g_RBP.Initialize();

  /* if we are raw need to let gpu know */
  m_passthrough = AE_IS_RAW(format.m_dataFormat);

  m_initDevice = device;
  m_initFormat = format;

  if (m_passthrough || CSettings::Get().GetString("audiooutput.audiodevice") == "PI:HDMI")
    m_output = AESINKPI_HDMI;
  else if (CSettings::Get().GetString("audiooutput.audiodevice") == "PI:Analogue")
    m_output = AESINKPI_ANALOGUE;
  else if (CSettings::Get().GetString("audiooutput.audiodevice") == "PI:Both")
    m_output = AESINKPI_BOTH;
  else assert(0);

  // analogue only supports stereo
  if (m_output == AESINKPI_ANALOGUE || m_output == AESINKPI_BOTH)
    format.m_channelLayout = AE_CH_LAYOUT_2_0;

  // setup for a 50ms sink feed from SoftAE
  if (format.m_dataFormat != AE_FMT_FLOATP && format.m_dataFormat != AE_FMT_FLOAT &&
      format.m_dataFormat != AE_FMT_S32NE && format.m_dataFormat != AE_FMT_S32NEP && format.m_dataFormat != AE_FMT_S32LE &&
      format.m_dataFormat != AE_FMT_S16NE && format.m_dataFormat != AE_FMT_S16NEP && format.m_dataFormat != AE_FMT_S16LE)
    format.m_dataFormat = AE_FMT_S16LE;
  unsigned int channels    = format.m_channelLayout.Count();
  unsigned int sample_size = CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3;
  format.m_frameSize     = sample_size * channels;
  format.m_sampleRate    = std::max(8000U, std::min(192000U, format.m_sampleRate));
  format.m_frames        = format.m_sampleRate * AUDIO_PLAYBUFFER / NUM_OMX_BUFFERS;
  format.m_frameSamples  = format.m_frames * channels;

  SetAudioProps(m_passthrough, GetChannelMap(format.m_channelLayout, m_passthrough));

  m_format = format;
  m_sinkbuffer_sec_per_byte = 1.0 / (double)(m_format.m_frameSize * m_format.m_sampleRate);

  CLog::Log(LOGDEBUG, "%s:%s Format:%d Channels:%d Samplerate:%d framesize:%d bufsize:%d bytes/s=%.2f dest=%s", CLASSNAME, __func__,
                m_format.m_dataFormat, channels, m_format.m_sampleRate, m_format.m_frameSize, m_format.m_frameSize * m_format.m_frames, 1.0/m_sinkbuffer_sec_per_byte,
                CSettings::Get().GetString("audiooutput.audiodevice").c_str());

  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;

  if (!m_omx_render.Initialize("OMX.broadcom.audio_render", OMX_IndexParamAudioInit))
    CLog::Log(LOGERROR, "%s::%s - m_omx_render.Initialize omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

  if (m_output == AESINKPI_BOTH)
  {
    if (!m_omx_splitter.Initialize("OMX.broadcom.audio_splitter", OMX_IndexParamAudioInit))
      CLog::Log(LOGERROR, "%s::%s - m_omx_splitter.Initialize omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    if (!m_omx_render_slave.Initialize("OMX.broadcom.audio_render", OMX_IndexParamAudioInit))
      CLog::Log(LOGERROR, "%s::%s - m_omx_render.Initialize omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    m_omx_output = &m_omx_splitter;
  }
  else
    m_omx_output = &m_omx_render;

  SetAudioDest();

  OMX_INIT_STRUCTURE(m_pcm_input);
  m_pcm_input.eNumData              = OMX_NumericalDataSigned;
  m_pcm_input.eEndian               = OMX_EndianLittle;
  m_pcm_input.bInterleaved          = OMX_TRUE;
  m_pcm_input.nBitPerSample         = sample_size * 8;
  // 0x8000 = float, 0x10000 = planar
  uint32_t flags = 0;
  if (m_format.m_dataFormat == AE_FMT_FLOAT || m_format.m_dataFormat == AE_FMT_FLOATP)
   flags |= 0x8000;
  if (AE_IS_PLANAR(m_format.m_dataFormat))
   flags |= 0x10000;
  m_pcm_input.ePCMMode              = flags == 0 ? OMX_AUDIO_PCMModeLinear : (OMX_AUDIO_PCMMODETYPE)flags;
  m_pcm_input.nChannels             = channels;
  m_pcm_input.nSamplingRate         = m_format.m_sampleRate;

  if ( m_omx_splitter.IsInitialized() )
  {
    m_pcm_input.nPortIndex = m_omx_splitter.GetInputPort();
    omx_err = m_omx_splitter.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - error m_omx_splitter SetParameter in omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

    m_pcm_input.nPortIndex = m_omx_splitter.GetOutputPort();
    omx_err = m_omx_splitter.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
    if(omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - error m_omx_splitter SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

    m_pcm_input.nPortIndex = m_omx_splitter.GetOutputPort() + 1;
    omx_err = m_omx_splitter.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
    if(omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - error m_omx_splitter SetParameter omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }

  if ( m_omx_render_slave.IsInitialized() )
  {
    m_pcm_input.nPortIndex = m_omx_render_slave.GetInputPort();
    omx_err = m_omx_render_slave.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - error m_omx_render_slave SetParameter in omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }

  if ( m_omx_render.IsInitialized() )
  {
    m_pcm_input.nPortIndex = m_omx_render.GetInputPort();
    omx_err = m_omx_render.SetParameter(OMX_IndexParamAudioPcm, &m_pcm_input);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s::%s - error m_omx_render SetParameter in omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }

  if ( m_omx_output->IsInitialized() )
  {
    // set up the number/size of buffers for decoder input
    OMX_PARAM_PORTDEFINITIONTYPE port_param;
    OMX_INIT_STRUCTURE(port_param);
    port_param.nPortIndex = m_omx_output->GetInputPort();

    omx_err = m_omx_output->GetParameter(OMX_IndexParamPortDefinition, &port_param);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s:%s - error get OMX_IndexParamPortDefinition (input) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

    port_param.nBufferCountActual = std::max((unsigned int)port_param.nBufferCountMin, (unsigned int)NUM_OMX_BUFFERS);
    port_param.nBufferSize = ALIGN_UP(m_format.m_frameSize * m_format.m_frames, port_param.nBufferAlignment);

    omx_err = m_omx_output->SetParameter(OMX_IndexParamPortDefinition, &port_param);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s:%s - error set OMX_IndexParamPortDefinition (input) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);

    omx_err = m_omx_output->AllocInputBuffers();
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s:%s - Error alloc buffers 0x%08x", CLASSNAME, __func__, omx_err);
  }

  if ( m_omx_splitter.IsInitialized() )
  {
    m_omx_tunnel_splitter.Initialize(&m_omx_splitter, m_omx_splitter.GetOutputPort(), &m_omx_render, m_omx_render.GetInputPort());
    omx_err = m_omx_tunnel_splitter.Establish();
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_splitter.Establish 0x%08x", omx_err);
      return false;
    }

    m_omx_tunnel_splitter_slave.Initialize(&m_omx_splitter, m_omx_splitter.GetOutputPort() + 1, &m_omx_render_slave, m_omx_render_slave.GetInputPort());
    omx_err = m_omx_tunnel_splitter_slave.Establish();
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXAudio::Initialize - Error m_omx_tunnel_splitter_slave.Establish 0x%08x", omx_err);
      return false;
    }
  }

  if ( m_omx_splitter.IsInitialized() )
  {
    omx_err = m_omx_splitter.SetStateForComponent(OMX_StateExecuting);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s:%s - m_omx_splitter OMX_StateExecuting omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }
  if ( m_omx_render.IsInitialized() )
  {
    omx_err = m_omx_render.SetStateForComponent(OMX_StateExecuting);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s:%s - m_omx_render OMX_StateExecuting omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }
  if ( m_omx_render_slave.IsInitialized() )
  {
    omx_err = m_omx_render_slave.SetStateForComponent(OMX_StateExecuting);
    if (omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "%s:%s - m_omx_render_slave OMX_StateExecuting omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }

  m_Initialized = true;
  return true;
}


void CAESinkPi::Deinitialize()
{
  CLog::Log(LOGDEBUG, "%s:%s", CLASSNAME, __func__);
  SetAudioProps(false, 0);

  if ( m_omx_render.IsInitialized() )
    m_omx_render.IgnoreNextError(OMX_ErrorPortUnpopulated);
  if ( m_omx_render_slave.IsInitialized() )
    m_omx_render_slave.IgnoreNextError(OMX_ErrorPortUnpopulated);

  if ( m_omx_tunnel_splitter.IsInitialized() )
    m_omx_tunnel_splitter.Deestablish();
  if ( m_omx_tunnel_splitter_slave.IsInitialized() )
    m_omx_tunnel_splitter_slave.Deestablish();

  if ( m_omx_splitter.IsInitialized() )
    m_omx_splitter.FlushAll();
  if ( m_omx_render.IsInitialized() )
    m_omx_render.FlushAll();
  if ( m_omx_render_slave.IsInitialized() )
    m_omx_render_slave.FlushAll();

  if ( m_omx_splitter.IsInitialized() )
    m_omx_splitter.Deinitialize();
  if ( m_omx_render.IsInitialized() )
    m_omx_render.Deinitialize();
  if ( m_omx_render_slave.IsInitialized() )
    m_omx_render_slave.Deinitialize();

  m_Initialized = false;
}

bool CAESinkPi::IsCompatible(const AEAudioFormat &format, const std::string &device)
{
  bool compatible =
      /* compare against the requested format and the real format */
      (m_initFormat.m_sampleRate    == format.m_sampleRate    || m_format.m_sampleRate    == format.m_sampleRate   ) &&
      (m_initFormat.m_dataFormat    == format.m_dataFormat    || m_format.m_dataFormat    == format.m_dataFormat   ) &&
      (m_initFormat.m_channelLayout == format.m_channelLayout || m_format.m_channelLayout == format.m_channelLayout) &&
      (m_initDevice == device);
  CLog::Log(LOGDEBUG, "%s:%s Format:%d Channels:%d Samplerate:%d = %d", CLASSNAME, __func__, format.m_dataFormat, format.m_channelLayout.Count(), format.m_sampleRate, compatible);
  return compatible;
}

void CAESinkPi::GetDelay(AEDelayStatus& status)
{
  OMX_PARAM_U32TYPE param;
  OMX_INIT_STRUCTURE(param);

  if (!m_Initialized)
  {
    status.SetDelay(0);
    return;
  }

  param.nPortIndex = m_omx_render.GetInputPort();

  OMX_ERRORTYPE omx_err = m_omx_render.GetConfig(OMX_IndexConfigAudioRenderingLatency, &param);

  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - error getting OMX_IndexConfigAudioRenderingLatency error 0x%08x",
      CLASSNAME, __func__, omx_err);
  }
  double sinkbuffer_seconds_to_empty = m_sinkbuffer_sec_per_byte * param.nU32 * m_format.m_frameSize;
  status.SetDelay(sinkbuffer_seconds_to_empty);
}

double CAESinkPi::GetCacheTotal()
{
  return AUDIO_PLAYBUFFER;
}

unsigned int CAESinkPi::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_Initialized || !m_omx_output || !frames)
    return frames;

  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *omx_buffer = NULL;

  unsigned int channels    = m_format.m_channelLayout.Count();
  unsigned int sample_size = CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3;
  const int planes = AE_IS_PLANAR(m_format.m_dataFormat) ? channels : 1;
  const int chans  = AE_IS_PLANAR(m_format.m_dataFormat) ? 1 : channels;
  const int pitch  = chans * sample_size;

  AEDelayStatus status;
  GetDelay(status);
  double delay = status.GetDelay();
  if (delay <= 0.0 && m_submitted)
    CLog::Log(LOGNOTICE, "%s:%s Underrun (delay:%.2f frames:%d)", CLASSNAME, __func__, delay, frames);

  omx_buffer = m_omx_output->GetInputBuffer(1000);
  if (omx_buffer == NULL)
  {
    CLog::Log(LOGERROR, "CAESinkPi::AddPackets timeout");
    return 0;
  }

  omx_buffer->nFilledLen = frames * m_format.m_frameSize;
  // must be true
  assert(omx_buffer->nFilledLen <= omx_buffer->nAllocLen);
  omx_buffer->nTimeStamp = ToOMXTime(0);
  omx_buffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;

  if (omx_buffer->nFilledLen)
  {
    int planesize = omx_buffer->nFilledLen / planes;
    for (int i=0; i < planes; i++)
      memcpy((uint8_t *)omx_buffer->pBuffer + i * planesize, data[i] + offset * pitch, planesize);
  }
  omx_err = m_omx_output->EmptyThisBuffer(omx_buffer);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s:%s frames=%d err=%x", CLASSNAME, __func__, frames, omx_err);
    m_omx_output->DecoderEmptyBufferDone(m_omx_output->GetComponent(), omx_buffer);
  }
  m_submitted++;
  GetDelay(status);
  delay = status.GetDelay();
  if (delay > AUDIO_PLAYBUFFER)
    Sleep((int)(1000.0f * (delay - AUDIO_PLAYBUFFER)));
  return frames;
}

void CAESinkPi::Drain()
{
  AEDelayStatus status;
  GetDelay(status);
  int delay = (int)(status.GetDelay() * 1000.0);
  if (delay)
    Sleep(delay);
  CLog::Log(LOGDEBUG, "%s:%s delay:%dms now:%dms", CLASSNAME, __func__, delay, (int)(status.GetDelay() * 1000.0));
}

void CAESinkPi::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  m_info.m_channels.Reset();
  m_info.m_dataFormats.clear();
  m_info.m_sampleRates.clear();

  m_info.m_deviceType = AE_DEVTYPE_HDMI;
  m_info.m_deviceName = "HDMI";
  m_info.m_displayName = "HDMI";
  m_info.m_displayNameExtra = "";
  m_info.m_channels += AE_CH_FL;
  m_info.m_channels += AE_CH_FR;
  for (unsigned int i=0; i<sizeof PassthroughSampleRates/sizeof *PassthroughSampleRates; i++)
    m_info.m_sampleRates.push_back(PassthroughSampleRates[i]);
  m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
  m_info.m_dataFormats.push_back(AE_FMT_S32NE);
  m_info.m_dataFormats.push_back(AE_FMT_S16NE);
  m_info.m_dataFormats.push_back(AE_FMT_S32LE);
  m_info.m_dataFormats.push_back(AE_FMT_S16LE);
  m_info.m_dataFormats.push_back(AE_FMT_FLOATP);
  m_info.m_dataFormats.push_back(AE_FMT_S32NEP);
  m_info.m_dataFormats.push_back(AE_FMT_S16NEP);
  m_info.m_dataFormats.push_back(AE_FMT_AC3);
  m_info.m_dataFormats.push_back(AE_FMT_DTS);
  m_info.m_dataFormats.push_back(AE_FMT_EAC3);

  list.push_back(m_info);

  m_info.m_channels.Reset();
  m_info.m_dataFormats.clear();
  m_info.m_sampleRates.clear();

  m_info.m_deviceType = AE_DEVTYPE_PCM;
  m_info.m_deviceName = "Analogue";
  m_info.m_displayName = "Analogue";
  m_info.m_displayNameExtra = "";
  m_info.m_channels += AE_CH_FL;
  m_info.m_channels += AE_CH_FR;
  m_info.m_sampleRates.push_back(48000);
  m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
  m_info.m_dataFormats.push_back(AE_FMT_S32LE);
  m_info.m_dataFormats.push_back(AE_FMT_S16LE);
  m_info.m_dataFormats.push_back(AE_FMT_FLOATP);
  m_info.m_dataFormats.push_back(AE_FMT_S32NEP);
  m_info.m_dataFormats.push_back(AE_FMT_S16NEP);

  list.push_back(m_info);

  m_info.m_channels.Reset();
  m_info.m_dataFormats.clear();
  m_info.m_sampleRates.clear();

  m_info.m_deviceType = AE_DEVTYPE_PCM;
  m_info.m_deviceName = "Both";
  m_info.m_displayName = "HDMI and Analogue";
  m_info.m_displayNameExtra = "";
  m_info.m_channels += AE_CH_FL;
  m_info.m_channels += AE_CH_FR;
  m_info.m_sampleRates.push_back(48000);
  m_info.m_dataFormats.push_back(AE_FMT_FLOAT);
  m_info.m_dataFormats.push_back(AE_FMT_S32LE);
  m_info.m_dataFormats.push_back(AE_FMT_S16LE);
  m_info.m_dataFormats.push_back(AE_FMT_FLOATP);
  m_info.m_dataFormats.push_back(AE_FMT_S32NEP);
  m_info.m_dataFormats.push_back(AE_FMT_S16NEP);

  list.push_back(m_info);
}

#endif
