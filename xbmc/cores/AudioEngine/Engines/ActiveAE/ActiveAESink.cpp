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
#include "cores/AudioEngine/Utils/AEUtil.h"
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

  delete m_convertBuffer.pkt;
  m_convertBuffer.pkt = NULL;
}

AEDeviceType CActiveAESink::GetDeviceType(const std::string &device)
{
  std::string dev = device;
  std::string dri;
  CAESinkFactory::ParseDevice(dev, dri);
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    for (AEDeviceInfoList::iterator itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo& info = *itt2;
      if (info.m_deviceName == dev)
        return info.m_deviceType;
    }
  }
  return AE_DEVTYPE_PCM;
}

bool CActiveAESink::HasPassthroughDevice()
{
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    for (AEDeviceInfoList::iterator itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo& info = *itt2;
      if (info.m_deviceType != AE_DEVTYPE_PCM)
        return true;
    }
  }
  return false;
}

bool CActiveAESink::SupportsFormat(const std::string &device, AEDataFormat format, int samplerate)
{
  std::string dev = device;
  std::string dri;
  CAESinkFactory::ParseDevice(dev, dri);
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    if (dri == itt->m_sinkName)
    {
      for (AEDeviceInfoList::iterator itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
      {
        CAEDeviceInfo& info = *itt2;
        if (info.m_deviceName == dev)
        {
          AEDataFormatList::iterator itt3;
          itt3 = find(info.m_dataFormats.begin(), info.m_dataFormats.end(), format);
          if (itt3 != info.m_dataFormats.end())
          {
            AESampleRateList::iterator itt4;
            itt4 = find(info.m_sampleRates.begin(), info.m_sampleRates.end(), samplerate);
            if (itt4 != info.m_sampleRates.end())
              return true;
            else
              return false;
          }
          else
            return false;
        }
      }
    }
  }
  return false;
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
};

int SINK_parentStates[] = {
    -1,
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    2, //TOP_CONFIGURED_SUSPEND
    2, //TOP_CONFIGURED_IDLE
    2, //TOP_CONFIGURED_PLAY
    2, //TOP_CONFIGURED_SILENCE
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
            m_device = *(data->device);
          }
          m_extError = false;
          m_extSilenceTimer = 0;
          m_extStreaming = false;
          ReturnBuffers();
          OpenSink();

          if (!m_extError)
          {
            SinkReply reply;
            reply.format = m_sinkFormat;
            reply.cacheTotal = m_sink->GetCacheTotal();
            reply.latency = m_sink->GetLatency();
            reply.hasVolume = m_sink->HasVolume();
            m_state = S_TOP_CONFIGURED_IDLE;
            m_extTimeout = 10000;
            msg->Reply(CSinkControlProtocol::ACC, &reply, sizeof(SinkReply));
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

        case CSinkControlProtocol::APPFOCUSED:
          m_extAppFocused = *(bool*)msg->data;
          SetSilenceTimer();
          m_extTimeout = 0;
          return;

        case CSinkControlProtocol::STREAMING:
          m_extStreaming = *(bool*)msg->data;
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
        case CSinkControlProtocol::STREAMING:
          m_extStreaming = *(bool*)msg->data;
          SetSilenceTimer();
          if (!m_extSilenceTimer.IsTimePast())
          {
            m_state = S_TOP_CONFIGURED_SILENCE;
          }
          m_extTimeout = 0;
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
            m_extSilenceTimer.Set(m_extSilenceTimeout);
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
        case CSinkControlProtocol::STREAMING:
          m_extStreaming = *(bool*)msg->data;
          SetSilenceTimer();
          m_extTimeout = 0;
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
          OutputSamples(&m_sampleOfSilence);
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
          OutputSamples(&m_sampleOfSilence);
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
          if (!m_extSilenceTimer.IsTimePast())
          {
            m_state = S_TOP_CONFIGURED_SILENCE;
            m_extTimeout = 0;
          }
          else
          {
            m_sink->Drain();
            m_state = S_TOP_CONFIGURED_IDLE;
            if (m_extAppFocused)
              m_extTimeout = 10000;
            else
              m_extTimeout = 0;
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
          if (m_extError)
          {
            m_sink->Deinitialize();
            delete m_sink;
            m_sink = NULL;
            m_state = S_TOP_CONFIGURED_SUSPEND;
          }
          else
            m_state = S_TOP_CONFIGURED_PLAY;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "CActiveAESink::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void CActiveAESink::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;
  XbmcThreads::EndTime timer;

  m_state = S_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;
  m_extAppFocused = true;

  while (!m_bStop)
  {
    gotMsg = false;
    timer.Set(m_extTimeout);

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
      m_extTimeout = timer.MillisLeft();
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

void CActiveAESink::EnumerateSinkList(bool force)
{
  if (!m_sinkInfoList.empty() && !force)
    return;

  unsigned int c_retry = 4;
  m_sinkInfoList.clear();
  CAESinkFactory::EnumerateEx(m_sinkInfoList);
  while(m_sinkInfoList.size() == 0 && c_retry > 0)
  {
    CLog::Log(LOGNOTICE, "No Devices found - retry: %d", c_retry);
    Sleep(1500);
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
  EnumerateSinkList(false);

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
  EnumerateSinkList(false);

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
  // we need a copy of m_device here because ParseDevice and CreateDevice write back
  // into this variable
  std::string device = m_device;
  std::string driver;
  bool passthrough = AE_IS_RAW(m_requestedFormat.m_dataFormat);

  CAESinkFactory::ParseDevice(device, driver);
  if (driver.empty() && m_sink)
    driver = m_sink->GetName();

  CLog::Log(LOGINFO, "CActiveAESink::OpenSink - initialize sink");

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
  CLog::Log(LOGDEBUG, "CActiveAESink::OpenSink - trying to open device %s", device.c_str());
  m_sink = CAESinkFactory::Create(device, m_sinkFormat, passthrough);

  // try first device in out list
  if (!m_sink && !m_sinkInfoList.empty())
  {
    driver = m_sinkInfoList.front().m_sinkName;
    device = m_sinkInfoList.front().m_deviceInfoList.front().m_deviceName;
    GetDeviceFriendlyName(device);
    if (!driver.empty())
      device = driver + ":" + device;
    m_sinkFormat = m_requestedFormat;
    CLog::Log(LOGDEBUG, "CActiveAESink::OpenSink - trying to open device %s", device.c_str());
    m_sink = CAESinkFactory::Create(device, m_sinkFormat, passthrough);
  }

  // open NULL sink
  // TODO: should not be required by ActiveAE
  if (!m_sink)
  {
    device = "NULL:NULL";
    m_sinkFormat = m_requestedFormat;
    CLog::Log(LOGDEBUG, "CActiveAESink::OpenSink - open NULL sink");
    m_sink = CAESinkFactory::Create(device, m_sinkFormat, passthrough);
  }

  if (!m_sink)
  {
    CLog::Log(LOGERROR, "CActiveAESink::OpenSink - no sink was returned");
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

  CLog::Log(LOGDEBUG, "CActiveAESink::OpenSink - %s Initialized:", m_sink->GetName());
  CLog::Log(LOGDEBUG, "  Output Device : %s", m_deviceFriendlyName.c_str());
  CLog::Log(LOGDEBUG, "  Sample Rate   : %d", m_sinkFormat.m_sampleRate);
  CLog::Log(LOGDEBUG, "  Sample Format : %s", CAEUtil::DataFormatToStr(m_sinkFormat.m_dataFormat));
  CLog::Log(LOGDEBUG, "  Channel Count : %d", m_sinkFormat.m_channelLayout.Count());
  CLog::Log(LOGDEBUG, "  Channel Layout: %s", ((std::string)m_sinkFormat.m_channelLayout).c_str());
  CLog::Log(LOGDEBUG, "  Frames        : %d", m_sinkFormat.m_frames);
  CLog::Log(LOGDEBUG, "  Frame Samples : %d", m_sinkFormat.m_frameSamples);
  CLog::Log(LOGDEBUG, "  Frame Size    : %d", m_sinkFormat.m_frameSize);

  // init sample of silence
  SampleConfig config;
  config.fmt = CActiveAEResample::GetAVSampleFormat(m_sinkFormat.m_dataFormat);
  config.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_sinkFormat.m_dataFormat);
  config.channel_layout = CActiveAEResample::GetAVChannelLayout(m_sinkFormat.m_channelLayout);
  config.channels = m_sinkFormat.m_channelLayout.Count();
  config.sample_rate = m_sinkFormat.m_sampleRate;

  // init sample of silence/noise
  delete m_sampleOfSilence.pkt;
  m_sampleOfSilence.pkt = new CSoundPacket(config, m_sinkFormat.m_frames);
  m_sampleOfSilence.pkt->nb_samples = m_sampleOfSilence.pkt->max_nb_samples;
  if (!passthrough)
    GenerateNoise();

  delete m_convertBuffer.pkt;
  m_convertBuffer.pkt = NULL;
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
  uint8_t **buffer = samples->pkt->data;
  unsigned int frames = samples->pkt->nb_samples;
  unsigned int maxFrames;
  int retry = 0;
  unsigned int written = 0;
  double sinkDelay = 0.0;

  switch(m_convertState)
  {
  case SKIP_CONVERT:
    break;
  case NEED_CONVERT:
    EnsureConvertBuffer(samples);
    Convert(samples);
    buffer = m_convertBuffer.pkt->data;
    break;
  case NEED_BYTESWAP:
    Endian_Swap16_buf((uint16_t *)buffer[0], (uint16_t *)buffer[0], frames * samples->pkt->config.channels);
    break;
  case CHECK_CONVERT:
    ConvertInit(samples);
    if (m_convertState == NEED_CONVERT)
    {
      Convert(samples);
      buffer = m_convertBuffer.pkt->data;
    }
    else if (m_convertState == NEED_BYTESWAP)
      Endian_Swap16_buf((uint16_t *)buffer[0], (uint16_t *)buffer[0], frames * samples->pkt->config.channels);
    break;
  default:
    break;
  }

  while(frames > 0)
  {
    maxFrames = std::min(frames, m_sinkFormat.m_frames);
    written = m_sink->AddPackets(buffer, maxFrames, samples->pkt->nb_samples-frames);
    if (written == 0)
    {
      Sleep(500*m_sinkFormat.m_frames/m_sinkFormat.m_sampleRate);
      retry++;
      if (retry > 4)
      {
        m_extError = true;
        CLog::Log(LOGERROR, "CActiveAESink::OutputSamples - failed");
        m_stats->UpdateSinkDelay(0, frames);
        return 0;
      }
      else
        continue;
    }
    else if (written > maxFrames)
    {
      m_extError = true;
      CLog::Log(LOGERROR, "CActiveAESink::OutputSamples - sink returned error");
      m_stats->UpdateSinkDelay(0, samples->pool ? maxFrames : 0);
      return 0;
    }
    frames -= written;
    sinkDelay = m_sink->GetDelay();
    m_stats->UpdateSinkDelay(sinkDelay, samples->pool ? written : 0);
  }
  return sinkDelay*1000;
}

void CActiveAESink::ConvertInit(CSampleBuffer* samples)
{
  if (CActiveAEResample::GetAESampleFormat(samples->pkt->config.fmt, samples->pkt->config.bits_per_sample) != m_sinkFormat.m_dataFormat)
  {
    m_convertFn = CAEConvert::FrFloat(m_sinkFormat.m_dataFormat);
    EnsureConvertBuffer(samples);
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
  if (m_convertBuffer.pkt && samples->pkt->max_nb_samples <= m_convertBuffer.pkt->max_nb_samples)
    return;

  delete m_convertBuffer.pkt;

  SampleConfig config;
  config.fmt = CActiveAEResample::GetAVSampleFormat(m_sinkFormat.m_dataFormat);
  config.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_sinkFormat.m_dataFormat);
  config.channel_layout = CActiveAEResample::GetAVChannelLayout(m_sinkFormat.m_channelLayout);
  config.channels = m_sinkFormat.m_channelLayout.Count();
  config.sample_rate = m_sinkFormat.m_sampleRate;

  m_convertBuffer.pkt = new CSoundPacket(config, m_sinkFormat.m_frames);
}

void CActiveAESink::Convert(CSampleBuffer* samples)
{
  if (samples->pkt->planes != m_convertBuffer.pkt->planes)
    assert(0);

  int planes = samples->pkt->planes;
  for (int i=0; i<planes; i++)
  {
    m_convertFn((float*)samples->pkt->data[0], samples->pkt->nb_samples * samples->pkt->config.channels / planes, m_convertBuffer.pkt->data[i]);
  }
}

#define PI 3.1415926536f

void CActiveAESink::GenerateNoise()
{
  int nb_floats = m_sampleOfSilence.pkt->max_nb_samples;
  if (!AE_IS_PLANAR(m_sinkFormat.m_dataFormat))
    nb_floats *= m_sampleOfSilence.pkt->config.channels;

  float *noise = (float*)_aligned_malloc(nb_floats*sizeof(float), 16);

  float R1, R2;
  for(int i=0; i<nb_floats;i++)
  {
    do
    {
      R1 = (float) rand() / (float) RAND_MAX;
      R2 = (float) rand() / (float) RAND_MAX;
    }
    while(R1 == 0.0f);
    
    noise[i] = (float) sqrt( -2.0f * log( R1 )) * cos( 2.0f * PI * R2 ) * 0.00001f;
  }

  AEDataFormat fmt = CActiveAEResample::GetAESampleFormat(m_sampleOfSilence.pkt->config.fmt, m_sampleOfSilence.pkt->config.bits_per_sample);
  if (AE_IS_PLANAR(fmt))
  {
    switch(fmt)
    {
    case AE_FMT_U8P:
      fmt = AE_FMT_U8;
      break;
    case AE_FMT_S16NEP:
      fmt = AE_FMT_S16NE;
      break;
    case AE_FMT_S32NEP:
      fmt = AE_FMT_S32NE;
      break;
    case AE_FMT_S24NE4P:
      fmt = AE_FMT_S24NE4;
      break;
    case AE_FMT_FLOATP:
      fmt = AE_FMT_FLOAT;
      break;
    case AE_FMT_DOUBLEP:
      fmt = AE_FMT_DOUBLE;
      break;
    default:
      assert(0);
      break;
    }
  }
  CAEConvert::AEConvertFrFn convertFn = CAEConvert::FrFloat(fmt);

  for (int i=0; i<m_sampleOfSilence.pkt->planes; i++)
  {
    convertFn(noise, nb_floats, m_sampleOfSilence.pkt->data[i]);
  }
  _aligned_free(noise);
}

void CActiveAESink::SetSilenceTimer()
{
  if (m_extStreaming)
    m_extSilenceTimeout = XbmcThreads::EndTime::InfiniteValue;
  else if (m_extAppFocused)
    m_extSilenceTimeout = CSettings::Get().GetInt("audiooutput.streamsilence") * 60000;
  else
    m_extSilenceTimeout = 0;
  m_extSilenceTimer.Set(m_extSilenceTimeout);
}
