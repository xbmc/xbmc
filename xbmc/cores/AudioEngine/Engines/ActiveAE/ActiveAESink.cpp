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

#include <sstream>

#include "ActiveAESink.h"
#include "Utils/AEUtil.h"
#include "utils/EndianSwap.h"
#include "ActiveAE.h"

#include "settings/Settings.h"

using namespace ActiveAE;

CActiveAESink::CActiveAESink(CEvent *inMsgEvent) :
  CThread("AESink"),
  m_controlPort("SinkControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("SinkDataPort", inMsgEvent, &m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;
  m_sink = NULL;
  m_stats = NULL;
  m_convertBuffer = NULL;
  m_volume = 0.0;
}

void CActiveAESink::Start()
{
  if (!IsRunning())
  {
    Create();
    SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
  }
}

void CActiveAESink::Dispose()
{
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();

  if (m_sink)
  {
    m_sink->Drain();
    m_sink->Deinitialize();
    delete m_sink;
    m_sink = NULL;
  }

  delete m_sampleOfSilence.pkt;
  m_sampleOfSilence.pkt = NULL;

  delete m_sampleOfNoise.pkt;
  m_sampleOfNoise.pkt = NULL;

  if (m_convertBuffer)
  {
    _aligned_free(m_convertBuffer);
    m_convertBuffer = NULL;
  }
}

bool CActiveAESink::IsCompatible(const AEAudioFormat format, const std::string &device)
{
  if (!m_sink)
    return false;
  return m_sink->IsCompatible(format, device);
}

bool CActiveAESink::HasVolume()
{
  if (!m_sink)
    return false;
  return m_sink->HasVolume();
}

enum SINK_STATES
{
  S_TOP = 0,                      // 0
  S_TOP_UNCONFIGURED,             // 1
  S_TOP_CONFIGURED,               // 2
  S_TOP_CONFIGURED_SUSPEND,       // 3
  S_TOP_CONFIGURED_IDLE,          // 4
  S_TOP_CONFIGURED_PLAY,          // 5
  S_TOP_CONFIGURED_SILENCE,       // 6
  S_TOP_CONFIGURED_WARMUP,        // 7
};

int SINK_parentStates[] = {
    -1,
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    2, //TOP_CONFIGURED_SUSPEND
    2, //TOP_CONFIGURED_IDLE
    2, //TOP_CONFIGURED_PLAY
    2, //TOP_CONFIGURED_SILENCE
    2, //TOP_CONFIGURED_WARMUP
};

void CActiveAESink::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = SINK_parentStates[state])
  {
    switch (state)
    {
    case S_TOP: // TOP
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CSinkControlProtocol::CONFIGURE:
          SinkConfig *data;
          data = (SinkConfig*)msg->data;
          if (data)
          {
            m_requestedFormat = data->format;
            m_stats = data->stats;
          }
          m_extError = false;
          m_extSilence = false;
          ReturnBuffers();
          OpenSink();

          if (!m_extError)
          {
            m_stats->SetSinkCacheTotal(m_sink->GetCacheTotal());
            m_state = S_TOP_CONFIGURED_IDLE;
            m_extTimeout = 10000;
            msg->Reply(CSinkControlProtocol::ACC, &m_sinkFormat, sizeof(AEAudioFormat));
          }
          else
          {
            m_state = S_TOP_UNCONFIGURED;
            msg->Reply(CSinkControlProtocol::ERR);
          }
          return;

        case CSinkControlProtocol::UNCONFIGURE:
          ReturnBuffers();
          if (m_sink)
          {
            m_sink->Drain();
            m_sink->Deinitialize();
            delete m_sink;
            m_sink = NULL;
          }
          m_state = S_TOP_UNCONFIGURED;
          msg->Reply(CSinkControlProtocol::ACC);
          return;

        case CSinkControlProtocol::FLUSH:
          ReturnBuffers();
          msg->Reply(CSinkControlProtocol::ACC);
          return;

        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::DRAIN:
          msg->Reply(CSinkDataProtocol::ACC);
          m_state = S_TOP_UNCONFIGURED;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "CActiveAESink::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case S_TOP_UNCONFIGURED:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          m_extTimeout = 1000;
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::SAMPLE:
          CSampleBuffer *samples;
          int timeout;
          samples = *((CSampleBuffer**)msg->data);
          timeout = 1000*samples->pkt->nb_samples/samples->pkt->config.sample_rate;
          Sleep(timeout);
          msg->Reply(CSinkDataProtocol::RETURNSAMPLE, &samples, sizeof(CSampleBuffer*));
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CSinkControlProtocol::SILENCEMODE:
          m_extSilence = *(bool*)msg->data;
          if (CSettings::Get().GetBool("audiooutput.streamsilence"))
            m_extSilence = true;
          if (m_extSilence)
          {
            m_extCycleCounter = 5;
            m_state = S_TOP_CONFIGURED_WARMUP;
            m_extTimeout = 0;
          }
          return;
        case CSinkControlProtocol::VOLUME:
          m_volume = *(float*)msg->data;
          m_sink->SetVolume(m_volume);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::DRAIN:
          m_sink->Drain();
          msg->Reply(CSinkDataProtocol::ACC);
          m_state = S_TOP_CONFIGURED_IDLE;
          m_extTimeout = 10000;
          return;
        case CSinkDataProtocol::SAMPLE:
          CSampleBuffer *samples;
          unsigned int delay;
          samples = *((CSampleBuffer**)msg->data);
          delay = OutputSamples(samples);
          msg->Reply(CSinkDataProtocol::RETURNSAMPLE, &samples, sizeof(CSampleBuffer*));
          if (m_extError)
          {
            m_sink->Deinitialize();
            delete m_sink;
            m_sink = NULL;
            m_state = S_TOP_CONFIGURED_SUSPEND;
            m_extTimeout = 0;
          }
          else
          {
            m_state = S_TOP_CONFIGURED_PLAY;
            m_extTimeout = delay / 2;
          }
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED_SUSPEND:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CSinkControlProtocol::SILENCEMODE:
          return;
        case CSinkControlProtocol::VOLUME:
          m_volume = *(float*)msg->data;
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::SAMPLE:
          m_extError = false;
          OpenSink();
          OutputSamples(&m_sampleOfNoise);
          m_state = S_TOP_CONFIGURED_PLAY;
          m_extTimeout = 0;
          m_bStateMachineSelfTrigger = true;
          return;
        case CSinkDataProtocol::DRAIN:
          msg->Reply(CSinkDataProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          m_extTimeout = 10000;
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED_IDLE:
      if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CSinkDataProtocol::SAMPLE:
          OutputSamples(&m_sampleOfNoise);
          m_state = S_TOP_CONFIGURED_PLAY;
          m_extTimeout = 0;
          m_bStateMachineSelfTrigger = true;
          return;
        default:
          break;
        }
      }
      else if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          m_sink->Deinitialize();
          delete m_sink;
          m_sink = NULL;
          m_state = S_TOP_CONFIGURED_SUSPEND;
          m_extTimeout = 10000;
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED_PLAY:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          if (m_extSilence)
          {
            m_state = S_TOP_CONFIGURED_SILENCE;
            m_extTimeout = 0;
          }
          else
          {
            m_sink->Drain();
            m_state = S_TOP_CONFIGURED_IDLE;
            m_extTimeout = 10000;
          }
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED_SILENCE:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          OutputSamples(&m_sampleOfSilence);
          m_extCycleCounter--;
          if (m_extError)
          {
            m_sink->Deinitialize();
            delete m_sink;
            m_sink = NULL;
            m_state = S_TOP_CONFIGURED_SUSPEND;
          }
          else if(m_extCycleCounter <= 0)
          {
            m_extCycleCounter = 2;
            m_state = S_TOP_CONFIGURED_WARMUP;
          }
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED_WARMUP:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          OutputSamples(&m_sampleOfNoise);
          m_extCycleCounter--;
          if (m_extError)
          {
            m_sink->Deinitialize();
            delete m_sink;
            m_sink = NULL;
            m_state = S_TOP_CONFIGURED_SUSPEND;
          }
          else if(m_extCycleCounter <= 0)
          {
            m_extCycleCounter = 20;
            m_state = S_TOP_CONFIGURED_SILENCE;
          }
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "CActiveSink::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void CActiveAESink::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;

  m_state = S_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;

  while (!m_bStop)
  {
    gotMsg = false;

    if (m_bStateMachineSelfTrigger)
    {
      m_bStateMachineSelfTrigger = false;
      // self trigger state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }
    // check control port
    else if (m_controlPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_controlPort;
    }
    // check data port
    else if (m_dataPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_dataPort;
    }

    if (gotMsg)
    {
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.WaitMSec(m_extTimeout))
    {
      continue;
    }
    // time out
    else
    {
      msg = m_controlPort.GetMessage();
      msg->signal = CSinkControlProtocol::TIMEOUT;
      port = 0;
      // signal timeout to state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
    }
  }
}

void CActiveAESink::EnumerateSinkList()
{
  unsigned int c_retry = 5;
  m_sinkInfoList.clear();
  CAESinkFactory::EnumerateEx(m_sinkInfoList);
  while(m_sinkInfoList.size() == 0 && c_retry > 0)
  {
    CLog::Log(LOGNOTICE, "No Devices found - retry: %d", c_retry);
    Sleep(2000);
    c_retry--;
    // retry the enumeration
    CAESinkFactory::EnumerateEx(m_sinkInfoList, true);
  }
  CLog::Log(LOGNOTICE, "Found %lu Lists of Devices", m_sinkInfoList.size());
  PrintSinks();
}

void CActiveAESink::PrintSinks()
{
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    CLog::Log(LOGNOTICE, "Enumerated %s devices:", itt->m_sinkName.c_str());
    int count = 0;
    for (AEDeviceInfoList::iterator itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
    {
      CLog::Log(LOGNOTICE, "    Device %d", ++count);
      CAEDeviceInfo& info = *itt2;
      std::stringstream ss((std::string)info);
      std::string line;
      while(std::getline(ss, line, '\n'))
        CLog::Log(LOGNOTICE, "        %s", line.c_str());
    }
  }
}

void CActiveAESink::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo devInfo = *itt2;
      if (passthrough && devInfo.m_deviceType == AE_DEVTYPE_PCM)
        continue;

      std::string device = sinkInfo.m_sinkName + ":" + devInfo.m_deviceName;

      std::stringstream ss;

      /* add the sink name if we have more then one sink type */
      if (m_sinkInfoList.size() > 1)
        ss << sinkInfo.m_sinkName << ": ";

      ss << devInfo.m_displayName;
      if (!devInfo.m_displayNameExtra.empty())
        ss << ", " << devInfo.m_displayNameExtra;

      devices.push_back(AEDevice(ss.str(), device));
    }
  }
}

std::string CActiveAESink::GetDefaultDevice(bool passthrough)
{
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo devInfo = *itt2;
      if (passthrough && devInfo.m_deviceType == AE_DEVTYPE_PCM)
        continue;

      std::string device = sinkInfo.m_sinkName + ":" + devInfo.m_deviceName;
      return device;
    }
  }
  return "default";
}

void CActiveAESink::GetDeviceFriendlyName(std::string &device)
{
  m_deviceFriendlyName = "Device not found";
  /* Match the device and find its friendly name */
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo& devInfo = *itt2;
      if (devInfo.m_deviceName == device)
      {
        m_deviceFriendlyName = devInfo.m_displayName;
        break;
      }
    }
  }
  return;
}

void CActiveAESink::OpenSink()
{
  std::string device, driver;
  bool passthrough = AE_IS_RAW(m_requestedFormat.m_dataFormat);
  if (passthrough)
    device = CSettings::Get().GetString("audiooutput.passthroughdevice");
  else
    device = CSettings::Get().GetString("audiooutput.audiodevice");

  CAESinkFactory::ParseDevice(device, driver);
  if (driver.empty() && m_sink)
    driver = m_sink->GetName();

  std::string sinkName;
  if (m_sink)
  {
    sinkName = m_sink->GetName();
    std::transform(sinkName.begin(), sinkName.end(), sinkName.begin(), ::toupper);
  }

  if (!m_sink || sinkName != driver || !m_sink->IsCompatible(m_requestedFormat, device))
  {
    CLog::Log(LOGINFO, "CActiveAE::OpenSink - sink incompatible, re-starting");

    if (m_sink)
    {
      m_sink->Drain();
      m_sink->Deinitialize();
      delete m_sink;
      m_sink = NULL;
    }

    // get the display name of the device
    GetDeviceFriendlyName(device);

    // if we already have a driver, prepend it to the device string
    if (!driver.empty())
      device = driver + ":" + device;

    // WARNING: this changes format and does not use passthrough
    m_sinkFormat = m_requestedFormat;
    m_sink = CAESinkFactory::Create(device, m_sinkFormat, passthrough);

    if (!m_sink)
    {
      m_extError = true;
      return;
    }

    m_sink->SetVolume(m_volume);

#ifdef WORDS_BIGENDIAN
    if (m_sinkFormat.m_dataFormat == AE_FMT_S16BE)
      m_sinkFormat.m_dataFormat = AE_FMT_S16NE;
    else if (m_sinkFormat.m_dataFormat == AE_FMT_S32BE)
      m_sinkFormat.m_dataFormat = AE_FMT_S32NE;
#else
    if (m_sinkFormat.m_dataFormat == AE_FMT_S16LE)
      m_sinkFormat.m_dataFormat = AE_FMT_S16NE;
    else if (m_sinkFormat.m_dataFormat == AE_FMT_S32LE)
      m_sinkFormat.m_dataFormat = AE_FMT_S32NE;
#endif

    CLog::Log(LOGDEBUG, "CActiveAE::OpenSink - %s Initialized:", m_sink->GetName());
    CLog::Log(LOGDEBUG, "  Output Device : %s", m_deviceFriendlyName.c_str());
    CLog::Log(LOGDEBUG, "  Sample Rate   : %d", m_sinkFormat.m_sampleRate);
    CLog::Log(LOGDEBUG, "  Sample Format : %s", CAEUtil::DataFormatToStr(m_sinkFormat.m_dataFormat));
    CLog::Log(LOGDEBUG, "  Channel Count : %d", m_sinkFormat.m_channelLayout.Count());
    CLog::Log(LOGDEBUG, "  Channel Layout: %s", ((std::string)m_sinkFormat.m_channelLayout).c_str());
    CLog::Log(LOGDEBUG, "  Frames        : %d", m_sinkFormat.m_frames);
    CLog::Log(LOGDEBUG, "  Frame Samples : %d", m_sinkFormat.m_frameSamples);
    CLog::Log(LOGDEBUG, "  Frame Size    : %d", m_sinkFormat.m_frameSize);
  }
  else
    CLog::Log(LOGINFO, "CActiveAE::OpenSink - keeping old sink with : %s, %s, %dhz",
                          CAEUtil::DataFormatToStr(m_sinkFormat.m_dataFormat),
                          ((std::string)m_sinkFormat.m_channelLayout).c_str(),
                          m_sinkFormat.m_sampleRate);

  // init sample of silence
  SampleConfig config;
  config.fmt = CActiveAEResample::GetAVSampleFormat(m_sinkFormat.m_dataFormat);
  config.channel_layout = CActiveAEResample::GetAVChannelLayout(m_sinkFormat.m_channelLayout);
  config.channels = m_sinkFormat.m_channelLayout.Count();
  config.sample_rate = m_sinkFormat.m_sampleRate;
  delete m_sampleOfSilence.pkt;
  m_sampleOfSilence.pkt = new CSoundPacket(config, m_sinkFormat.m_frames);
  m_sampleOfSilence.pkt->nb_samples = m_sampleOfSilence.pkt->max_nb_samples;

  // init sample of noise
  delete m_sampleOfNoise.pkt;
  m_sampleOfNoise.pkt = new CSoundPacket(config, m_sinkFormat.m_frames);
  m_sampleOfNoise.pkt->nb_samples = m_sampleOfNoise.pkt->max_nb_samples;
  if (!passthrough)
    GenerateNoise();

  if (m_convertBuffer)
  {
    _aligned_free(m_convertBuffer);
    m_convertBuffer = NULL;
  }
  m_convertFn = NULL;
  m_convertState = CHECK_CONVERT;
}

void CActiveAESink::ReturnBuffers()
{
  Message *msg = NULL;
  CSampleBuffer *samples;
  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == CSinkDataProtocol::SAMPLE)
    {
      samples = *((CSampleBuffer**)msg->data);
      msg->Reply(CSinkDataProtocol::RETURNSAMPLE, &samples, sizeof(CSampleBuffer*));
    }
  }
}

unsigned int CActiveAESink::OutputSamples(CSampleBuffer* samples)
{
  uint8_t *buffer = samples->pkt->data[0];
  unsigned int frames = samples->pkt->nb_samples;
  unsigned int maxFrames;
  int retry = 0;
  int written = 0;
  double sinkDelay = 0.0;

  switch(m_convertState)
  {
  case SKIP_CONVERT:
    break;
  case NEED_CONVERT:
    EnsureConvertBuffer(samples);
    buffer = Convert(samples);
    break;
  case NEED_BYTESWAP:
    Endian_Swap16_buf((uint16_t *)buffer, (uint16_t *)buffer, frames * samples->pkt->config.channels);
    break;
  case CHECK_CONVERT:
    ConvertInit(samples);
    if (m_convertState == NEED_CONVERT)
      buffer = Convert(samples);
    else if (m_convertState == NEED_BYTESWAP)
      Endian_Swap16_buf((uint16_t *)buffer, (uint16_t *)buffer, frames * samples->pkt->config.channels);
    break;
  default:
    break;
  }

  while(frames > 0)
  {
    maxFrames = std::min(frames, m_sinkFormat.m_frames);
    written = m_sink->AddPackets(buffer, maxFrames, true, true);
    if (written == 0)
    {
      Sleep(500*m_sinkFormat.m_frames/m_sinkFormat.m_sampleRate);
      retry++;
      if (retry > 4)
      {
        m_extError = true;
        CLog::Log(LOGERROR, "CActiveAESink::OutputSamples - failed");
        return 0;
      }
      else
        continue;
    }
    frames -= written;
    buffer += written*m_sinkFormat.m_frameSize;
    sinkDelay = m_sink->GetDelay();
    m_stats->UpdateSinkDelay(sinkDelay, samples->pool ? written : 0);
  }
  return sinkDelay*1000;
}

void CActiveAESink::ConvertInit(CSampleBuffer* samples)
{
  if (CActiveAEResample::GetAESampleFormat(samples->pkt->config.fmt) != m_sinkFormat.m_dataFormat)
  {
    m_convertFn = CAEConvert::FrFloat(m_sinkFormat.m_dataFormat);
    if (m_convertBuffer)
      _aligned_free(m_convertBuffer);
    m_convertBufferSampleSize = samples->pkt->max_nb_samples;
    m_convertBuffer = (uint8_t*)_aligned_malloc(samples->pkt->max_nb_samples * m_sinkFormat.m_channelLayout.Count() * m_sinkFormat.m_frameSize, 16);
    memset(m_convertBuffer, 0, samples->pkt->max_nb_samples * m_sinkFormat.m_channelLayout.Count() * m_sinkFormat.m_frameSize);
    m_convertState = NEED_CONVERT;
  }
  else if (AE_IS_RAW(m_requestedFormat.m_dataFormat) && CAEUtil::S16NeedsByteSwap(AE_FMT_S16NE, m_sinkFormat.m_dataFormat))
  {
    m_convertState = NEED_BYTESWAP;
  }
  else
    m_convertState = SKIP_CONVERT;
}

void CActiveAESink::EnsureConvertBuffer(CSampleBuffer* samples)
{
  if (!m_convertBuffer)
    return;

  if (samples->pkt->max_nb_samples <= m_convertBufferSampleSize)
    return;

  _aligned_free(m_convertBuffer);
  m_convertBufferSampleSize = samples->pkt->max_nb_samples;
  m_convertBuffer = (uint8_t*)_aligned_malloc(samples->pkt->max_nb_samples * m_sinkFormat.m_channelLayout.Count() * m_sinkFormat.m_frameSize, 16);
  memset(m_convertBuffer, 0, samples->pkt->max_nb_samples * m_sinkFormat.m_channelLayout.Count() * m_sinkFormat.m_frameSize);
}

uint8_t* CActiveAESink::Convert(CSampleBuffer* samples)
{
  m_convertFn((float*)samples->pkt->data[0], samples->pkt->nb_samples * samples->pkt->config.channels, m_convertBuffer);
  return m_convertBuffer;
}

#define PI 3.1415926536f

void CActiveAESink::GenerateNoise()
{
  int nb_floats = m_sinkFormat.m_frames*m_sinkFormat.m_channelLayout.Count();
  float *noise = new float[nb_floats];

  float R1, R2;
  for(int i=0; i<nb_floats;i++)
  {
    do
    {
      R1 = (float) rand() / (float) RAND_MAX;
      R2 = (float) rand() / (float) RAND_MAX;
    }
    while(R1 == 0.0f);
    
    noise[i] = (float) sqrt( -2.0f * log( R1 )) * cos( 2.0f * PI * R2 ) * 0.00001;
  }

  AEDataFormat fmt = CActiveAEResample::GetAESampleFormat(m_sampleOfNoise.pkt->config.fmt);
  CAEConvert::AEConvertFrFn convertFn = CAEConvert::FrFloat(fmt);
  convertFn(noise, nb_floats, m_sampleOfNoise.pkt->data[0]);
  delete [] noise;
}
