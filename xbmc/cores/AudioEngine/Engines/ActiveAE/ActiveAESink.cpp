/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ActiveAESink.h"

#include "ActiveAE.h"
#include "cores/AudioEngine/AEResampleFactory.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/EndianSwap.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <new> // for std::bad_alloc
#include <sstream>

using namespace AE;
using namespace ActiveAE;
using namespace std::chrono_literals;

CActiveAESink::CActiveAESink(CEvent* inMsgEvent)
  : CThread("AESink"),
    m_controlPort("SinkControlPort", inMsgEvent, &m_outMsgEvent),
    m_dataPort("SinkDataPort", inMsgEvent, &m_outMsgEvent),
    m_sink(nullptr),
    m_packer(nullptr)
{
  m_inMsgEvent = inMsgEvent;
  m_stats = nullptr;
  m_volume = 0.0;
  m_streamNoise = true;
}

CActiveAESink::~CActiveAESink() = default;

void CActiveAESink::Start()
{
  if (!IsRunning())
  {
    Create();
    SetPriority(ThreadPriority::ABOVE_NORMAL);
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
    m_sink.reset();
  }

  m_sampleOfSilence.pkt.reset();

  m_packer.reset();

  CAESinkFactory::Cleanup();
}

AEDeviceType CActiveAESink::GetDeviceType(const std::string &device)
{
  const AESinkDevice dev = CAESinkFactory::ParseDevice(device);

  for (auto itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    for (AEDeviceInfoList::iterator itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo& info = *itt2;
      if (info.m_deviceName == dev.name)
        return info.m_deviceType;
    }
  }
  return AE_DEVTYPE_PCM;
}

bool CActiveAESink::HasPassthroughDevice()
{
  for (auto itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    for (AEDeviceInfoList::iterator itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo& info = *itt2;
      if (info.m_deviceType != AE_DEVTYPE_PCM && !info.m_streamTypes.empty())
        return true;
    }
  }
  return false;
}

bool CActiveAESink::SupportsFormat(const std::string &device, AEAudioFormat &format)
{
  const AESinkDevice dev = CAESinkFactory::ParseDevice(device);

  for (auto itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    if (dev.driver == itt->m_sinkName)
    {
      for (auto itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
      {
        CAEDeviceInfo& info = *itt2;
        if (info.m_deviceName == dev.name)
        {
          bool isRaw = format.m_dataFormat == AE_FMT_RAW;
          bool formatExists = false;

          // PCM sample rate
          unsigned int samplerate = format.m_sampleRate;

          if (isRaw && info.m_wantsIECPassthrough)
          {
            switch (format.m_streamInfo.m_type)
            {
              case CAEStreamInfo::STREAM_TYPE_EAC3:
                samplerate = 192000;
                break;

              case CAEStreamInfo::STREAM_TYPE_TRUEHD:
                if (format.m_streamInfo.m_sampleRate == 48000 || format.m_streamInfo.m_sampleRate == 96000 || format.m_streamInfo.m_sampleRate == 192000)
                  samplerate = 192000;
                else
                  samplerate = 176400;
                break;

              case CAEStreamInfo::STREAM_TYPE_DTSHD:
              case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
                samplerate = 192000;
                break;

              default:
                break;
            }
            AEDataTypeList::iterator iit3;
            iit3 = find(info.m_streamTypes.begin(), info.m_streamTypes.end(), format.m_streamInfo.m_type);
            formatExists = (iit3 != info.m_streamTypes.end());
          }
          else if (isRaw && !info.m_wantsIECPassthrough)
          {
            samplerate = 48000;
            AEDataTypeList::iterator iit3;
            iit3 = find(info.m_streamTypes.begin(), info.m_streamTypes.end(), format.m_streamInfo.m_type);
            formatExists = (iit3 != info.m_streamTypes.end());
          }
          else // PCM case
          {
            AEDataFormatList::iterator itt3;
            itt3 = find(info.m_dataFormats.begin(), info.m_dataFormats.end(), format.m_dataFormat);
            formatExists = (itt3 != info.m_dataFormats.end());
          }

          // check if samplerate is available
          if (formatExists)
          {
            AESampleRateList::iterator itt4;
            itt4 = find(info.m_sampleRates.begin(), info.m_sampleRates.end(), samplerate);
            return itt4 != info.m_sampleRates.end();
          }
          else // format is not existent
          {
            return false;
          }
        }
      }
    }
  }
  return false;
}

bool CActiveAESink::NeedIECPacking()
{
  const AESinkDevice dev = CAESinkFactory::ParseDevice(m_device);

  for (auto itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    if (dev.driver == itt->m_sinkName)
    {
      for (auto itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
      {
        CAEDeviceInfo& info = *itt2;
        if (info.m_deviceName == dev.name)
        {
          return info.m_wantsIECPassthrough;
        }
      }
    }
  }
  return true;
}

bool CActiveAESink::DeviceExist(std::string driver, const std::string& device)
{
  if (driver.empty() && m_sink)
    driver = m_sink->GetName();

  for (const auto& itt : m_sinkInfoList)
  {
    if (itt.m_sinkName != driver)
      continue;

    for (const CAEDeviceInfo& info : itt.m_deviceInfoList)
    {
      if (info.m_deviceName == device)
        return true;
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
          data = reinterpret_cast<SinkConfig*>(msg->data);
          if (data)
          {
            m_requestedFormat = data->format;
            m_stats = data->stats;
            m_device = *(data->device);
          }
          m_extError = false;
          m_extSilenceTimer.Set(0ms);
          m_extStreaming = false;
          ReturnBuffers();
          OpenSink();

          if (!m_extError)
          {
            SinkReply reply;
            reply.format = m_sinkFormat;
            //! @todo
            //! use max raw packet size, for now use max size of an IEC packed packet
            //! maxIECPpacket > maxRawPacket
            //! for raw packets frameSize is set to 1
            if (m_requestedFormat.m_dataFormat == AE_FMT_RAW)
            {
              reply.format.m_frames = 61440;
            }
            reply.cacheTotal = m_sink->GetCacheTotal();
            reply.latency = m_sink->GetLatency();
            reply.hasVolume = m_sink->HasVolume();
            m_state = S_TOP_CONFIGURED_IDLE;
            m_extTimeout = 10s;
            m_sinkLatency = (int64_t)(reply.latency * 1000);
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
            m_sink.reset();
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
          m_extTimeout = 0ms;
          return;

        case CSinkControlProtocol::STREAMING:
          m_extStreaming = *(bool*)msg->data;
          return;

        case CSinkControlProtocol::SETSILENCETIMEOUT:
          m_silenceTimeOut = std::chrono::minutes(*reinterpret_cast<int*>(msg->data));
          return;

        case CSinkControlProtocol::SETNOISETYPE:
          m_streamNoise = *(bool*)msg->data;
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
          m_extTimeout = 0ms;
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == nullptr ? "timer" : port->portName;
        CLog::Log(LOGWARNING,
                  "CActiveAESink::{} - signal: {} form port: {} not handled for state: {}",
                  __FUNCTION__, signal, portName, m_state);
      }
      return;

    case S_TOP_UNCONFIGURED:
      if (port == nullptr) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          m_extTimeout = 1000ms;
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
          samples = *((CSampleBuffer**)msg->data);
          CThread::Sleep(std::chrono::milliseconds(1000 * samples->pkt->nb_samples /
                                                   samples->pkt->config.sample_rate));
          msg->Reply(CSinkDataProtocol::RETURNSAMPLE, &samples, sizeof(CSampleBuffer*));
          m_extTimeout = 0ms;
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
          m_extTimeout = 0ms;
          return;
        case CSinkControlProtocol::VOLUME:
          m_volume = *(float*)msg->data;
          m_sink->SetVolume(m_volume);
          return;

        case CSinkControlProtocol::SETNOISETYPE:
        {
          bool streamNoise = *(bool*)msg->data;
          if (streamNoise != m_streamNoise)
          {
            m_streamNoise = streamNoise;
            GenerateNoise();
          }
        }
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
          m_extTimeout = 10s;
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
            m_sink.reset();
            m_state = S_TOP_CONFIGURED_SUSPEND;
            m_extTimeout = 0ms;
          }
          else
          {
            m_state = S_TOP_CONFIGURED_PLAY;
            m_extTimeout = std::chrono::milliseconds(delay / 2);
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
          m_extTimeout = 0ms;
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
          if (!m_extError)
          {
            OutputSamples(&m_sampleOfSilence);
            m_state = S_TOP_CONFIGURED_PLAY;
            m_extTimeout = 0ms;
            m_bStateMachineSelfTrigger = true;
          }
          else
          {
            m_state = S_TOP_UNCONFIGURED;
          }
          return;
        case CSinkDataProtocol::DRAIN:
          msg->Reply(CSinkDataProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == nullptr) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          m_extTimeout = 10s;
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
          m_extTimeout = 0ms;
          m_bStateMachineSelfTrigger = true;
          return;
        default:
          break;
        }
      }
      else if (port == nullptr) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          m_sink->Deinitialize();
          m_sink.reset();
          m_state = S_TOP_CONFIGURED_SUSPEND;
          m_extTimeout = 10s;
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED_PLAY:
      if (port == nullptr) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          if (!m_extSilenceTimer.IsTimePast())
          {
            m_state = S_TOP_CONFIGURED_SILENCE;
            m_extTimeout = 0ms;
          }
          else
          {
            m_sink->Drain();
            m_state = S_TOP_CONFIGURED_IDLE;
            if (m_extAppFocused)
              m_extTimeout = 10s;
            else
              m_extTimeout = 0ms;
          }
          return;
        default:
          break;
        }
      }
      break;

    case S_TOP_CONFIGURED_SILENCE:
      if (port == nullptr) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          OutputSamples(&m_sampleOfSilence);
          if (m_extError)
          {
            m_sink->Deinitialize();
            m_sink.reset();
            m_state = S_TOP_CONFIGURED_SUSPEND;
          }
          else
            m_state = S_TOP_CONFIGURED_PLAY;
          m_extTimeout = 0ms;
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "CActiveAESink::{} - no valid state: {}", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void CActiveAESink::Process()
{
  Message *msg = nullptr;
  Protocol *port = nullptr;
  bool gotMsg;
  XbmcThreads::EndTime<> timer;

  m_state = S_TOP_UNCONFIGURED;
  m_extTimeout = 1000ms;
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
        msg = nullptr;
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
        msg = nullptr;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.Wait(m_extTimeout))
    {
      m_extTimeout = timer.GetTimeLeft();
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
        msg = nullptr;
      }
    }
  }
}

void CActiveAESink::EnumerateSinkList(bool force, std::string driver)
{
  if (!m_sinkInfoList.empty() && !force)
    return;

  if (!CAESinkFactory::HasSinks())
    return;

  std::vector<AE::AESinkInfo> tmpList(m_sinkInfoList);

  unsigned int c_retry = 4;
  m_sinkInfoList.clear();

  if (!driver.empty())
  {
    for (auto const& info : tmpList)
    {
      if (info.m_sinkName != driver)
        m_sinkInfoList.push_back(info);
    }
  }

  CAESinkFactory::EnumerateEx(m_sinkInfoList, false, driver);
  while (m_sinkInfoList.empty() && c_retry > 0)
  {
    CLog::Log(LOGINFO, "No Devices found - retry: {}", c_retry);
    CThread::Sleep(1500ms);
    c_retry--;
    // retry the enumeration
    CAESinkFactory::EnumerateEx(m_sinkInfoList, true, driver);
  }
  CLog::Log(LOGINFO, "Found {} Lists of Devices", m_sinkInfoList.size());
  PrintSinks(driver);
}

void CActiveAESink::PrintSinks(std::string& driver)
{
  for (auto itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    if (!driver.empty() && itt->m_sinkName != driver)
      continue;

    CLog::Log(LOGINFO, "Enumerated {} devices:", itt->m_sinkName);
    int count = 0;
    for (auto itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
    {
      CLog::Log(LOGINFO, "    Device {}", ++count);
      CAEDeviceInfo& info = *itt2;
      std::stringstream ss((std::string)info);
      std::string line;
      while(std::getline(ss, line, '\n'))
        CLog::Log(LOGINFO, "        {}", line);
    }
  }
}

std::string CActiveAESink::ValidateOuputDevice(const std::string& device, bool passthrough) const
{
  if (m_sinkInfoList.empty())
    return {};

  const AESinkDevice dev = CAESinkFactory::ParseDevice(device);

  // find exact match of deviceName in same driver
  if (!dev.driver.empty() && !dev.name.empty())
  {
    for (const auto& sink : m_sinkInfoList)
    {
      if (sink.m_sinkName != dev.driver)
        continue;

      for (const auto& d : sink.m_deviceInfoList)
      {
        if (passthrough && (d.m_deviceType == AE_DEVTYPE_PCM || d.m_onlyPCM))
          continue;

        if (!passthrough && d.m_onlyPassthrough)
          continue;

        if (d.m_deviceName == dev.name)
          return d.ToDeviceString(sink.m_sinkName);
      }
    }
  }

  // find same friendly name on other device in same driver
  if (!dev.driver.empty() && !dev.friendlyName.empty())
  {
    for (const auto& sink : m_sinkInfoList)
    {
      if (sink.m_sinkName != dev.driver)
        continue;

      for (const auto& d : sink.m_deviceInfoList)
      {
        if (passthrough && (d.m_deviceType == AE_DEVTYPE_PCM || d.m_onlyPCM))
          continue;

        if (!passthrough && d.m_onlyPassthrough)
          continue;

        if (d.GetFriendlyName() == dev.friendlyName)
          return d.ToDeviceString(sink.m_sinkName);
      }
    }
  }

  std::string firstDevice;

  // find default device of same driver or first device of same driver
  if (!dev.driver.empty())
  {
    for (const auto& sink : m_sinkInfoList)
    {
      if (sink.m_sinkName != dev.driver)
        continue;

      for (const auto& d : sink.m_deviceInfoList)
      {
        if (passthrough && (d.m_deviceType == AE_DEVTYPE_PCM || d.m_onlyPCM))
          continue;

        if (!passthrough && d.m_onlyPassthrough)
          continue;

        if (firstDevice.empty())
          firstDevice = d.ToDeviceString(sink.m_sinkName);

        if (d.m_deviceName.find("default") != std::string::npos)
          return d.ToDeviceString(sink.m_sinkName);
      }

      if (!firstDevice.empty())
        break;
    }
  }

  // return first device of same driver
  if (!firstDevice.empty())
    return firstDevice;

  firstDevice.clear();

  // find the default of any driver or first of any driver
  for (const auto& sink : m_sinkInfoList)
  {
    for (const auto& d : sink.m_deviceInfoList)
    {
      if (passthrough && (d.m_deviceType == AE_DEVTYPE_PCM || d.m_onlyPCM))
        continue;

      if (!passthrough && d.m_onlyPassthrough)
        continue;

      if (firstDevice.empty())
        firstDevice = d.ToDeviceString(sink.m_sinkName);

      if (d.m_deviceName.find("default") != std::string::npos)
        return d.ToDeviceString(sink.m_sinkName);
    }
  }

  // return first device of any driver or empty
  return firstDevice;
}

void CActiveAESink::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  EnumerateSinkList(false, "");

  for (auto itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      const CAEDeviceInfo& devInfo = *itt2;
      if (passthrough && devInfo.m_deviceType == AE_DEVTYPE_PCM)
        continue;

      // filters devices that should not be shown in the list
      // of AUDIO DEVICES or AUDIO PASSTHROUGH DEVICES
      // according to the capabilities informed by each sink
      if (devInfo.m_onlyPassthrough && !passthrough)
        continue;

      if (devInfo.m_onlyPCM && passthrough)
        continue;

      const std::string device = devInfo.ToDeviceString(sinkInfo.m_sinkName);

      std::stringstream ss;

      /* add the sink name if we have more then one sink type */
      if (m_sinkInfoList.size() > 1)
        ss << sinkInfo.m_sinkName << ": ";

      ss << devInfo.m_displayName;
      if (!devInfo.m_displayNameExtra.empty())
        ss << ", " << devInfo.m_displayNameExtra;

      devices.emplace_back(ss.str(), device);
    }
  }
}

void CActiveAESink::GetDeviceFriendlyName(const std::string& device)
{
  m_deviceFriendlyName = "Device not found";
  /* Match the device and find its friendly name */
  for (auto itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
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
}

void CActiveAESink::OpenSink()
{
  bool passthrough = (m_requestedFormat.m_dataFormat == AE_FMT_RAW);

  AESinkDevice dev = CAESinkFactory::ParseDevice(m_device);

  if (dev.driver.empty() && m_sink)
    dev.driver = m_sink->GetName();

  // iec packing or raw
  if (passthrough)
  {
    m_needIecPack = NeedIECPacking();
    if (m_needIecPack)
    {
      m_packer = std::make_unique<CAEBitstreamPacker>();
      m_requestedFormat.m_sampleRate = CAEBitstreamPacker::GetOutputRate(m_requestedFormat.m_streamInfo);
      m_requestedFormat.m_channelLayout = CAEBitstreamPacker::GetOutputChannelMap(m_requestedFormat.m_streamInfo);
    }
  }

  CLog::Log(LOGINFO, "CActiveAESink::OpenSink - initialize sink");

  if (m_sink)
  {
    m_sink->Drain();
    m_sink->Deinitialize();
    m_sink.reset();
  }

  // get the display name of the device
  GetDeviceFriendlyName(dev.name);

  // if we already have a driver, prepend it to the device string
  std::string device = dev.driver.empty() ? dev.name : dev.driver + ":" + dev.name;

  // WARNING: this changes format and does not use passthrough
  m_sinkFormat = m_requestedFormat;
  CLog::Log(LOGDEBUG, "CActiveAESink::OpenSink - trying to open device {}", device);
  m_sink = CAESinkFactory::Create(device, m_sinkFormat);

  // try first device in out list
  if (!m_sink && !m_sinkInfoList.empty())
  {
    dev.driver = m_sinkInfoList.front().m_sinkName;
    dev.name = m_sinkInfoList.front().m_deviceInfoList.front().m_deviceName;
    GetDeviceFriendlyName(dev.name);
    device = dev.driver.empty() ? dev.name : dev.driver + ":" + dev.name;
    m_sinkFormat = m_requestedFormat;
    CLog::Log(LOGDEBUG, "CActiveAESink::OpenSink - trying to open device {}", device);
    m_sink = CAESinkFactory::Create(device, m_sinkFormat);
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

  CLog::Log(LOGDEBUG, "CActiveAESink::OpenSink - {} Initialized:", m_sink->GetName());
  CLog::Log(LOGDEBUG, "  Output Device : {}", m_deviceFriendlyName);
  CLog::Log(LOGDEBUG, "  Sample Rate   : {}", m_sinkFormat.m_sampleRate);
  CLog::Log(LOGDEBUG, "  Sample Format : {}", CAEUtil::DataFormatToStr(m_sinkFormat.m_dataFormat));
  CLog::Log(LOGDEBUG, "  Channel Count : {}", m_sinkFormat.m_channelLayout.Count());
  CLog::Log(LOGDEBUG, "  Channel Layout: {}", ((std::string)m_sinkFormat.m_channelLayout));
  CLog::Log(LOGDEBUG, "  Frames        : {}", m_sinkFormat.m_frames);
  CLog::Log(LOGDEBUG, "  Frame Size    : {}", m_sinkFormat.m_frameSize);

  // init sample of silence
  SampleConfig config;
  config.fmt = CAEUtil::GetAVSampleFormat(m_sinkFormat.m_dataFormat);
  config.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_sinkFormat.m_dataFormat);
  config.dither_bits = CAEUtil::DataFormatToDitherBits(m_sinkFormat.m_dataFormat);
  config.channel_layout = CAEUtil::GetAVChannelLayout(m_sinkFormat.m_channelLayout);
  config.channels = m_sinkFormat.m_channelLayout.Count();
  config.sample_rate = m_sinkFormat.m_sampleRate;

  // init sample of silence/noise
  m_sampleOfSilence.pkt = std::make_unique<CSoundPacket>(config, m_sinkFormat.m_frames);
  m_sampleOfSilence.pkt->nb_samples = m_sampleOfSilence.pkt->max_nb_samples;
  if (!passthrough)
    GenerateNoise();
  else
  {
    m_sampleOfSilence.pkt->nb_samples = 0;
    m_sampleOfSilence.pkt->pause_burst_ms = m_sinkFormat.m_streamInfo.GetDuration();
  }

  m_swapState = CHECK_SWAP;
}

void CActiveAESink::ReturnBuffers()
{
  Message *msg = nullptr;
  CSampleBuffer *samples;
  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == CSinkDataProtocol::SAMPLE)
    {
      samples = *((CSampleBuffer**)msg->data);
      msg->Reply(CSinkDataProtocol::RETURNSAMPLE, &samples, sizeof(CSampleBuffer*));
    }
    msg->Release();
  }
}

unsigned int CActiveAESink::OutputSamples(CSampleBuffer* samples)
{
  uint8_t **buffer = samples->pkt->data;
  uint8_t *packBuffer;
  unsigned int frames = samples->pkt->nb_samples;
  unsigned int totalFrames = frames;
  unsigned int maxFrames;
  int retry = 0;
  unsigned int written = 0;
  AEDelayStatus status;

  if (m_requestedFormat.m_dataFormat == AE_FMT_RAW)
  {
    bool skipSwap = false;
    if (m_needIecPack)
    {
      if (frames > 0)
      {
        m_packer->Reset();
        m_packer->Pack(m_sinkFormat.m_streamInfo, buffer[0], frames);
      }
      else if (samples->pkt->pause_burst_ms > 0)
      {
        // construct a pause burst if we have already output valid audio
        bool burst = m_extStreaming && (m_packer->GetBuffer()[0] != 0);
        if (!m_packer->PackPause(m_sinkFormat.m_streamInfo, samples->pkt->pause_burst_ms, burst))
          skipSwap = true;
      }
      else
        m_packer->Reset();

      unsigned int size = m_packer->GetSize();
      packBuffer = m_packer->GetBuffer();
      buffer = &packBuffer;
      totalFrames = size / m_sinkFormat.m_frameSize;
      frames = totalFrames;

      switch(m_swapState)
      {
        case SKIP_SWAP:
          break;
        case NEED_BYTESWAP:
          if (!skipSwap)
            Endian_Swap16_buf((uint16_t *)buffer[0], (uint16_t *)buffer[0], size / 2);
          break;
        case CHECK_SWAP:
          SwapInit(samples);
          if (m_swapState == NEED_BYTESWAP)
            Endian_Swap16_buf((uint16_t *)buffer[0], (uint16_t *)buffer[0], size / 2);
          break;
        default:
          break;
      }
    }
    else // Android IEC packer (RAW)
    {
      if (samples->pkt->pause_burst_ms > 0)
      {
        m_sink->AddPause(samples->pkt->pause_burst_ms);
        m_sink->GetDelay(status);
        m_stats->UpdateSinkDelay(status, samples->pool ? 1 : 0);
        return status.delay * 1000;
      }
    }
  }

  int framesOrPackets;

  while (frames > 0)
  {
    maxFrames = std::min(frames, m_sinkFormat.m_frames);
    written = m_sink->AddPackets(buffer, maxFrames, totalFrames - frames);
    if (written == 0)
    {
      CThread::Sleep(
          std::chrono::milliseconds(500 * m_sinkFormat.m_frames / m_sinkFormat.m_sampleRate));
      retry++;
      if (retry > 4)
      {
        m_extError = true;
        CLog::Log(LOGERROR, "CActiveAESink::OutputSamples - failed");
        status.SetDelay(0);
        framesOrPackets = frames;
        if (m_requestedFormat.m_dataFormat == AE_FMT_RAW)
          framesOrPackets = 1;
        m_stats->UpdateSinkDelay(status, samples->pool ? framesOrPackets : 0);
        return 0;
      }
      else
        continue;
    }
    else if (written > maxFrames)
    {
      m_extError = true;
      CLog::Log(LOGERROR, "CActiveAESink::OutputSamples - sink returned error");
      status.SetDelay(0);
      framesOrPackets = frames;
      if (m_requestedFormat.m_dataFormat == AE_FMT_RAW)
        framesOrPackets = 1;
      m_stats->UpdateSinkDelay(status, samples->pool ? framesOrPackets : 0);
      return 0;
    }
    frames -= written;

    m_sink->GetDelay(status);

    if (m_requestedFormat.m_dataFormat != AE_FMT_RAW)
      m_stats->UpdateSinkDelay(status, samples->pool ? written : 0);
  }

  if (m_requestedFormat.m_dataFormat == AE_FMT_RAW)
    m_stats->UpdateSinkDelay(status, samples->pool ? 1 : 0);

  return status.delay * 1000;
}

void CActiveAESink::SwapInit(CSampleBuffer* samples)
{
  if ((m_requestedFormat.m_dataFormat == AE_FMT_RAW) && CAEUtil::S16NeedsByteSwap(AE_FMT_S16NE, m_sinkFormat.m_dataFormat))
  {
    m_swapState = NEED_BYTESWAP;
  }
  else
    m_swapState = SKIP_SWAP;
}

#define PI 3.1415926536f

void CActiveAESink::GenerateNoise()
{
  int nb_floats = m_sampleOfSilence.pkt->max_nb_samples;
  nb_floats *= m_sampleOfSilence.pkt->config.channels;
  size_t size = nb_floats*sizeof(float);

  float *noise = static_cast<float*>(KODI::MEMORY::AlignedMalloc(size, 32));
  if (!noise)
    throw std::bad_alloc();

  if (!m_streamNoise)
    memset(noise, 0, size);
  else
  {
    float R1, R2;
    for(int i = 0; i < nb_floats; i++)
    {
      do
      {
        R1 = (float) rand() / (float) RAND_MAX;
        R2 = (float) rand() / (float) RAND_MAX;
      }
      while(R1 == 0.0f);

      noise[i] = sqrt( -2.0f * log( R1 )) * cos( 2.0f * PI * R2 ) * 0.00001f;
    }
  }

  SampleConfig config = m_sampleOfSilence.pkt->config;
  std::unique_ptr<IAEResample> resampler =
      CAEResampleFactory::Create(AERESAMPLEFACTORY_QUICK_RESAMPLE);

  SampleConfig dstConfig, srcConfig;
  dstConfig.channel_layout = config.channel_layout;
  dstConfig.channels = config.channels;
  dstConfig.sample_rate = config.sample_rate;
  dstConfig.fmt = config.fmt;
  dstConfig.bits_per_sample = config.bits_per_sample;
  dstConfig.dither_bits = config.dither_bits;

  srcConfig.channel_layout = config.channel_layout;
  srcConfig.channels = config.channels;
  srcConfig.sample_rate = config.sample_rate;
  srcConfig.fmt = AV_SAMPLE_FMT_FLT;
  srcConfig.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_sinkFormat.m_dataFormat);
  srcConfig.dither_bits = CAEUtil::DataFormatToDitherBits(m_sinkFormat.m_dataFormat);

  resampler->Init(dstConfig, srcConfig, false, false, M_SQRT1_2, nullptr, AE_QUALITY_UNKNOWN, false,
                  0.0);

  resampler->Resample(m_sampleOfSilence.pkt->data, m_sampleOfSilence.pkt->max_nb_samples,
                     (uint8_t**)&noise, m_sampleOfSilence.pkt->max_nb_samples, 1.0);

  KODI::MEMORY::AlignedFree(noise);
}

void CActiveAESink::SetSilenceTimer()
{
  if (m_extStreaming)
    m_extSilenceTimeout = XbmcThreads::EndTime<decltype(m_extSilenceTimeout)>::Max();
  else if (m_extAppFocused) // handles no playback/GUI and playback in pause and seek
  {
    m_extSilenceTimeout = m_silenceTimeOut;
  }
  else
  {
    m_extSilenceTimeout = 0ms;
  }

  m_extSilenceTimer.Set(m_extSilenceTimeout);
}

void CActiveAESink::HotSwapSink(std::unique_ptr<IAESink> newSink)
{
  std::lock_guard<std::mutex> lock(m_sinkMutex); // ensure thread safety
  if (m_sink)
  {
    m_sink->Drain();
    m_sink->Deinitialize();
  }
  m_sink = std::move(newSink);
  if (m_sink)
  {
    std::string device = m_sink->GetName();
    m_device = device;
    m_deviceFriendlyName = device;
    AEAudioFormat format;
    m_sink->Initialize(format, device);
    CLog::Log(LOGINFO, "CActiveAESink::HotSwapSink - audio sink swapped to: {}", device);
  }
}
