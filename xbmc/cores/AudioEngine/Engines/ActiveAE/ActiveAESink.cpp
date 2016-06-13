/*
 *      Copyright (C) 2010-2015 Team Kodi
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

#include <sstream>

#include "ActiveAESink.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"
#include "utils/EndianSwap.h"
#include "ActiveAE.h"
#include "cores/AudioEngine/AEResampleFactory.h"

#include "settings/Settings.h"
#include "utils/log.h"

#include <new> // for std::bad_alloc
#include <algorithm>

#ifdef TARGET_POSIX
#include "linux/XMemUtils.h"
#endif

using namespace ActiveAE;

CActiveAESink::CActiveAESink(CEvent *inMsgEvent) :
  CThread("AESink"),
  m_controlPort("SinkControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("SinkDataPort", inMsgEvent, &m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;
  m_sink = nullptr;
  m_stats = nullptr;
  m_volume = 0.0;
  m_packer = nullptr;
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
    m_sink = nullptr;
  }

  delete m_sampleOfSilence.pkt;
  m_sampleOfSilence.pkt = nullptr;

  delete m_packer;
  m_packer = nullptr;
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

bool CActiveAESink::SupportsFormat(const std::string &device, AEAudioFormat &format)
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
            if (itt4 != info.m_sampleRates.end())
              return true;
            else
              return false;
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
  std::string dev = m_device;
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
          return info.m_wantsIECPassthrough;
        }
      }
    }
  }
  return true;
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
            m_extTimeout = 10000;
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
            delete m_sink;
            m_sink = nullptr;
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
        std::string portName = port == nullptr ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "CActiveAESink::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case S_TOP_UNCONFIGURED:
      if (port == nullptr) // timeout
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
            m_sink = nullptr;
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
      else if (port == nullptr) // timeout
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
      else if (port == nullptr) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          m_sink->Deinitialize();
          delete m_sink;
          m_sink = nullptr;
          m_state = S_TOP_CONFIGURED_SUSPEND;
          m_extTimeout = 10000;
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
      if (port == nullptr) // timeout
      {
        switch (signal)
        {
        case CSinkControlProtocol::TIMEOUT:
          OutputSamples(&m_sampleOfSilence);
          if (m_extError)
          {
            m_sink->Deinitialize();
            delete m_sink;
            m_sink = nullptr;
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
  Message *msg = nullptr;
  Protocol *port = nullptr;
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
        msg = nullptr;
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
  while(m_sinkInfoList.empty() && c_retry > 0)
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
  bool passthrough = (m_requestedFormat.m_dataFormat == AE_FMT_RAW);

  CAESinkFactory::ParseDevice(device, driver);
  if (driver.empty() && m_sink)
    driver = m_sink->GetName();

  // iec packing or raw
  if (passthrough)
  {
    m_needIecPack = NeedIECPacking();
    if (m_needIecPack)
    {
      m_packer = new CAEBitstreamPacker();
      m_requestedFormat.m_sampleRate = CAEBitstreamPacker::GetOutputRate(m_requestedFormat.m_streamInfo);
      m_requestedFormat.m_channelLayout = CAEBitstreamPacker::GetOutputChannelMap(m_requestedFormat.m_streamInfo);
    }
  }

  CLog::Log(LOGINFO, "CActiveAESink::OpenSink - initialize sink");

  if (m_sink)
  {
    m_sink->Drain();
    m_sink->Deinitialize();
    delete m_sink;
    m_sink = nullptr;
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
  //! @todo should not be required by ActiveAE
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
  CLog::Log(LOGDEBUG, "  Frame Size    : %d", m_sinkFormat.m_frameSize);

  // init sample of silence
  SampleConfig config;
  config.fmt = CAEUtil::GetAVSampleFormat(m_sinkFormat.m_dataFormat);
  config.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_sinkFormat.m_dataFormat);
  config.dither_bits = CAEUtil::DataFormatToDitherBits(m_sinkFormat.m_dataFormat);
  config.channel_layout = CAEUtil::GetAVChannelLayout(m_sinkFormat.m_channelLayout);
  config.channels = m_sinkFormat.m_channelLayout.Count();
  config.sample_rate = m_sinkFormat.m_sampleRate;

  // init sample of silence/noise
  delete m_sampleOfSilence.pkt;
  m_sampleOfSilence.pkt = new CSoundPacket(config, m_sinkFormat.m_frames);
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
  std::unique_ptr<uint8_t[]> mergebuffer;
  uint8_t* p_mergebuffer = NULL;
  AEDelayStatus status;

  if (m_requestedFormat.m_dataFormat == AE_FMT_RAW)
  {
    if (m_needIecPack)
    {
      if (frames > 0)
      {
        m_packer->Reset();
        if (m_sinkFormat.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
        {
          if (frames == 61440)
          {
            int offset;
            int len;
            m_packer->GetBuffer();
            for (int i=0; i<24; i++)
            {
              offset = i*2560;
              len = (*(buffer[0] + offset+2560-2) << 8) + *(buffer[0] + offset+2560-1);
              m_packer->Pack(m_sinkFormat.m_streamInfo, buffer[0] + offset, len);
            }
          }
          else
          {
            m_extError = true;
            CLog::Log(LOGERROR, "CActiveAESink::OutputSamples - incomplete TrueHD buffer");
            return 0;
          }
        }
        else
          m_packer->Pack(m_sinkFormat.m_streamInfo, buffer[0], frames);
      }
      else if (samples->pkt->pause_burst_ms > 0)
      {
        // construct a pause burst
        m_packer->PackPause(m_sinkFormat.m_streamInfo, samples->pkt->pause_burst_ms);
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
    else
    {
      if (m_sinkFormat.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD && frames == 61440)
      {
        int offset;
        int len;
        unsigned int size = 0;
        mergebuffer.reset(new uint8_t[MAX_IEC61937_PACKET]);
        p_mergebuffer = mergebuffer.get();
        for (int i=0; i<24; i++)
        {
          offset = i*2560;
          len = (*(buffer[0] + offset+2560-2) << 8) + *(buffer[0] + offset+2560-1);
          memcpy(&(mergebuffer.get())[size], buffer[0] + offset, len);
          size += len;
        }
        buffer = &p_mergebuffer;
        totalFrames = size / m_sinkFormat.m_frameSize;
        frames = totalFrames;
      }
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
      Sleep(500*m_sinkFormat.m_frames/m_sinkFormat.m_sampleRate);
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

  float *noise = (float*)_aligned_malloc(nb_floats*sizeof(float), 16);
  if (!noise)
    throw std::bad_alloc();

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

  SampleConfig config = m_sampleOfSilence.pkt->config;
  IAEResample *resampler = CAEResampleFactory::Create(AERESAMPLEFACTORY_QUICK_RESAMPLE);
  resampler->Init(config.channel_layout,
                 config.channels,
                 config.sample_rate,
                 config.fmt,
                 config.bits_per_sample,
                 config.dither_bits,
                 config.channel_layout,
                 config.channels,
                 config.sample_rate,
                 AV_SAMPLE_FMT_FLT,
                 CAEUtil::DataFormatToUsedBits(m_sinkFormat.m_dataFormat),
                 CAEUtil::DataFormatToDitherBits(m_sinkFormat.m_dataFormat),
                 false, false, nullptr, AE_QUALITY_UNKNOWN, false);
  resampler->Resample(m_sampleOfSilence.pkt->data, m_sampleOfSilence.pkt->max_nb_samples,
                     (uint8_t**)&noise, m_sampleOfSilence.pkt->max_nb_samples, 1.0);

  _aligned_free(noise);
  delete resampler;
}

void CActiveAESink::SetSilenceTimer()
{
  if (m_extStreaming)
    m_extSilenceTimeout = XbmcThreads::EndTime::InfiniteValue;
  else if (m_extAppFocused)
    m_extSilenceTimeout = CSettings::GetInstance().GetInt(CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE) * 60000;
  else
    m_extSilenceTimeout = 0;
  m_extSilenceTimer.Set(m_extSilenceTimeout);
}
