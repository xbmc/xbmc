/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Sinks/AESinkDARWINOSX.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/darwin/CoreAudioHelpers.h"
#include "cores/AudioEngine/Sinks/osx/AEDeviceEnumerationOSX.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioHardware.h"
#include "cores/AudioEngine/Utils/AERingBuffer.h"
#include "threads/SystemClock.h"
#include "utils/MemUtils.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"

#include <mutex>

using namespace std::chrono_literals;

static void EnumerateDevices(CADeviceList &list)
{
  std::string defaultDeviceName;
  CCoreAudioHardware::GetOutputDeviceName(defaultDeviceName);
  AudioDeviceID defaultID = CCoreAudioHardware::GetDefaultOutputDevice();

  CoreAudioDeviceList deviceIDList;
  CCoreAudioHardware::GetOutputDevices(&deviceIDList);
  while (!deviceIDList.empty())
  {
    AudioDeviceID deviceID = deviceIDList.front();

    AEDeviceEnumerationOSX devEnum(deviceID);
    CADeviceList listForDevice = devEnum.GetDeviceInfoList();
    for (UInt32 devIdx = 0; devIdx < listForDevice.size(); devIdx++)
      list.push_back(listForDevice[devIdx]);

    //in the first place of the list add the default device
    //with name "default" - if this is selected
    //we will output to whatever osx claims to be default
    //(allows transition from headphones to speaker and stuff
    //like that)
    //fixme taking the first stream device is wrong here
    //we rather might need the concatenation of all streams *sucks*
    if(defaultID == deviceID && defaultDeviceName == devEnum.GetMasterDeviceName())
    {
      struct CADeviceInstance deviceInstance;
      deviceInstance.audioDeviceId = deviceID;
      deviceInstance.streamIndex = INT_MAX;//don't limit streamidx for the raw device
      deviceInstance.sourceId = INT_MAX;
      CAEDeviceInfo firstDevice = listForDevice.front().second;
      firstDevice.m_deviceName = "default";
      firstDevice.m_displayName = "Default";
      firstDevice.m_displayNameExtra = defaultDeviceName;
      list.insert(list.begin(), std::make_pair(deviceInstance, firstDevice));
    }

    deviceIDList.pop_front();
  }
}

/* static, threadsafe access to the device list */
static CADeviceList     s_devices;
static CCriticalSection s_devicesLock;

static void EnumerateDevices()
{
  CADeviceList devices;
  EnumerateDevices(devices);
  {
    std::unique_lock<CCriticalSection> lock(s_devicesLock);
    s_devices = devices;
  }
}

static CADeviceList GetDevices()
{
  CADeviceList list;
  {
    std::unique_lock<CCriticalSection> lock(s_devicesLock);
    list = s_devices;
  }
  return list;
}

OSStatus deviceChangedCB(AudioObjectID                       inObjectID,
                         UInt32                              inNumberAddresses,
                         const AudioObjectPropertyAddress    inAddresses[],
                         void*                               inClientData)
{
  bool deviceChanged = false;
  static AudioDeviceID oldDefaultDevice = 0;
  AudioDeviceID currentDefaultOutputDevice = 0;

  for (unsigned int i = 0; i < inNumberAddresses; i++)
  {
    switch (inAddresses[i].mSelector)
    {
      case kAudioHardwarePropertyDefaultOutputDevice:
        currentDefaultOutputDevice = CCoreAudioHardware::GetDefaultOutputDevice();
        // This listener is called on every change of the hardware
        // device. So check if the default device has really changed.
        if (oldDefaultDevice != currentDefaultOutputDevice)
        {
          deviceChanged = true;
          oldDefaultDevice = currentDefaultOutputDevice;
        }
        break;
      default:
        deviceChanged = true;
        break;
    }
    if (deviceChanged)
      break;
  }

  if  (deviceChanged)
  {
    CLog::Log(LOGDEBUG, "CoreAudio: audiodevicelist changed - reenumerating");
    IAE* ae = CServiceBroker::GetActiveAE();
    if (ae)
      ae->DeviceChange();
    CLog::Log(LOGDEBUG, "CoreAudio: audiodevicelist changed - done");
  }
  return noErr;
}

////////////////////////////////////////////////////////////////////////////////////////////
CAESinkDARWINOSX::CAESinkDARWINOSX()
{
  // By default, kAudioHardwarePropertyRunLoop points at the process's main thread on SnowLeopard,
  // If your process lacks such a run loop, you can set kAudioHardwarePropertyRunLoop to NULL which
  // tells the HAL to run its own thread for notifications (which was the default prior to SnowLeopard).
  // So tell the HAL to use its own thread for similar behavior under all supported versions of OSX.
  CFRunLoopRef theRunLoop = NULL;
  AudioObjectPropertyAddress theAddress = {
    kAudioHardwarePropertyRunLoop,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };
  OSStatus theError = AudioObjectSetPropertyData(kAudioObjectSystemObject,
                                                 &theAddress, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
  if (theError != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioAE::constructor: kAudioHardwarePropertyRunLoop error.");
  }
  CCoreAudioDevice::RegisterDeviceChangedCB(true, deviceChangedCB, this);
  CCoreAudioDevice::RegisterDefaultOutputDeviceChangedCB(true, deviceChangedCB, this);
}

CAESinkDARWINOSX::~CAESinkDARWINOSX()
{
  CCoreAudioDevice::RegisterDeviceChangedCB(false, deviceChangedCB, this);
  CCoreAudioDevice::RegisterDefaultOutputDeviceChangedCB(false, deviceChangedCB, this);
}

void CAESinkDARWINOSX::Register()
{
  AE::AESinkRegEntry reg;
  reg.sinkName = "DARWINOSX";
  reg.createFunc = CAESinkDARWINOSX::Create;
  reg.enumerateFunc = CAESinkDARWINOSX::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(reg);
}

std::unique_ptr<IAESink> CAESinkDARWINOSX::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkDARWINOSX>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

bool CAESinkDARWINOSX::Initialize(AEAudioFormat &format, std::string &device)
{
  AudioDeviceID deviceID = 0;
  UInt32 requestedStreamIndex = INT_MAX;
  UInt32 requestedSourceId = INT_MAX;
  bool passthrough = false;

  // this sink needs IEC packing for RAW data
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    format.m_dataFormat= AE_FMT_S16NE;
    passthrough = true;
  }

  CADeviceList devices = GetDevices();
  if (StringUtils::EqualsNoCase(device, "default"))
  {
    CCoreAudioHardware::GetOutputDeviceName(device);
    deviceID = CCoreAudioHardware::GetDefaultOutputDevice();
    CLog::Log(LOGINFO, "{}: Opening default device {}", __PRETTY_FUNCTION__, device);
  }
  else
  {
    for (size_t i = 0; i < devices.size(); i++)
    {
      if (device == devices[i].second.m_deviceName)
      {
        const struct CADeviceInstance &deviceInstance = devices[i].first;
        deviceID = deviceInstance.audioDeviceId;
        requestedStreamIndex = deviceInstance.streamIndex;
        requestedSourceId = deviceInstance.sourceId;
        if (requestedStreamIndex != INT_MAX)
          CLog::Log(LOGINFO, "{} pseudo device - requesting stream {}", __FUNCTION__,
                    (unsigned int)requestedStreamIndex);
        if (requestedSourceId != INT_MAX)
          CLog::Log(LOGINFO, "{} device - requesting audiosource {}", __FUNCTION__,
                    (unsigned int)requestedSourceId);
        break;
      }
    }
  }

  if (!deviceID)
  {
    CLog::Log(LOGERROR, "{}: Unable to find device {}", __FUNCTION__, device);
    return false;
  }

  AEDeviceEnumerationOSX devEnum(deviceID);
  AudioStreamBasicDescription outputFormat = {};
  AudioStreamID outputStream = 0;
  UInt32 numOutputChannels = 0;
  m_planes = 1;
  // after FindSuitableFormatForStream requestedStreamIndex will have a valid index and no INT_MAX anymore ...
  if (devEnum.FindSuitableFormatForStream(requestedStreamIndex, format, false, outputFormat, outputStream))
  {
    numOutputChannels = outputFormat.mChannelsPerFrame;

    if (devEnum.IsPlanar())
    {
      numOutputChannels = std::min((size_t)format.m_channelLayout.Count(), (size_t)devEnum.GetNumPlanes());
      m_planes = numOutputChannels;
      CLog::Log(LOGDEBUG, "{} Found planar audio with {} channels using {} of them.", __FUNCTION__,
                (unsigned int)devEnum.GetNumPlanes(), (unsigned int)numOutputChannels);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "{}, Unable to find suitable stream", __FUNCTION__);
    return false;
  }

  AudioStreamBasicDescription outputFormatVirt = {};
  AudioStreamID outputStreamVirt = 0;
  UInt32 numOutputChannelsVirt = 0;
  if (passthrough)
  {
    if (devEnum.FindSuitableFormatForStream(requestedStreamIndex, format, true, outputFormatVirt, outputStreamVirt))
    {
      numOutputChannelsVirt = outputFormatVirt.mChannelsPerFrame;
    }
    else
    {
      CLog::Log(LOGERROR, "{}, Unable to find suitable virtual stream", __FUNCTION__);
      //return false;
      numOutputChannelsVirt = 0;
    }
  }

  /* Update our AE format */
  format.m_sampleRate    = outputFormat.mSampleRate;

  m_outputBufferIndex = requestedStreamIndex;

  // if we are in passthrough but didn't have a matching
  // virtual format - enable bitstream which deals with
  // backconverting from float to 16bit
  if (passthrough && numOutputChannelsVirt == 0)
  {
    m_outputBitstream = true;
    CLog::Log(LOGDEBUG, "{}: Bitstream passthrough with float -> int16 conversion enabled",
              __FUNCTION__);
  }

  std::string formatString;
  CLog::Log(LOGDEBUG, "{}: Selected stream[{}] - id: {:#04X}, Physical Format: {} {}", __FUNCTION__,
            (unsigned int)m_outputBufferIndex, (unsigned int)outputStream,
            StreamDescriptionToString(outputFormat, formatString),
            passthrough ? "passthrough" : "");

  m_device.Open(deviceID);
  SetHogMode(passthrough);

  // Configure the output stream object
  m_outputStream.Open(outputStream);

  AudioStreamBasicDescription virtualFormat, previousPhysicalFormat;
  m_outputStream.GetVirtualFormat(&virtualFormat);
  m_outputStream.GetPhysicalFormat(&previousPhysicalFormat);
  CLog::Log(LOGDEBUG, "{}: Previous Virtual Format: {}", __FUNCTION__,
            StreamDescriptionToString(virtualFormat, formatString));
  CLog::Log(LOGDEBUG, "{}: Previous Physical Format: {}", __FUNCTION__,
            StreamDescriptionToString(previousPhysicalFormat, formatString));

  m_outputStream.SetPhysicalFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
  if (passthrough && numOutputChannelsVirt > 0)
    m_outputStream.SetVirtualFormat(&outputFormatVirt);

  m_outputStream.GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "{}: New Virtual Format: {}", __FUNCTION__,
            StreamDescriptionToString(virtualFormat, formatString));
  CLog::Log(LOGDEBUG, "{}: New Physical Format: {}", __FUNCTION__,
            StreamDescriptionToString(outputFormat, formatString));

  if (requestedSourceId != INT_MAX && !m_device.SetDataSource(requestedSourceId))
    CLog::Log(LOGERROR, "{}: Error setting requested audio source.", __FUNCTION__);

  m_latentFrames = m_device.GetNumLatencyFrames();
  m_latentFrames += m_outputStream.GetNumLatencyFrames();

  // update the channel map based on the new stream format
  devEnum.GetAEChannelMap(format.m_channelLayout, numOutputChannels);

  //! @todo Should we use the virtual format to determine our data format?
  format.m_frameSize     = format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  format.m_frames        = m_device.GetBufferSize();

  m_frameSizePerPlane = format.m_frameSize / m_planes;
  m_framesPerSecond   = format.m_sampleRate;

  unsigned int num_buffers = 4;
  m_buffer = new AERingBuffer(num_buffers * format.m_frames * m_frameSizePerPlane, m_planes);
  CLog::Log(LOGDEBUG, "{}: using buffer size: {} ({:f} ms)", __FUNCTION__, m_buffer->GetMaxSize(),
            (float)m_buffer->GetMaxSize() / (m_framesPerSecond * m_frameSizePerPlane));

  if (!passthrough)
    format.m_dataFormat = (m_planes > 1) ? AE_FMT_FLOATP : AE_FMT_FLOAT;

  // Register for data request callbacks from the driver and start
  m_device.AddIOProc(renderCallback, this);
  m_device.Start();
  return true;
}

void CAESinkDARWINOSX::SetHogMode(bool on)
{
  //! @todo Auto hogging sets this for us. Figure out how/when to turn it off or use it
  //! It appears that leaving this set will also restore the previous stream format when the
  //! Application exits. If auto hogging is set and we try to set hog mode, we will deadlock
  //! From the SDK docs: "If the AudioDevice is in a non-mixable mode, the HAL will automatically take hog mode on behalf of the first process to start an IOProc."
  //!
  //! Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  //! If it is attempted after the format change, there is a high likelihood of a deadlock
  //! We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)
  if (on)
  {
    // Auto-Hog does not always un-hog the device when changing back to a mixable mode.
    // Handle this on our own until it is fixed.
    CCoreAudioHardware::SetAutoHogMode(false);
    bool autoHog = CCoreAudioHardware::GetAutoHogMode();
    CLog::Log(LOGDEBUG,
              " CoreAudioRenderer::InitializeEncoded: "
              "Auto 'hog' mode is set to '{}'.",
              autoHog ? "On" : "Off");
    if (autoHog)
      return;
  }
  m_device.SetHogStatus(on);
  m_device.SetMixingSupport(!on);
}

void CAESinkDARWINOSX::Deinitialize()
{
  m_device.Stop();
  m_device.RemoveIOProc();

  m_outputStream.Close();
  m_device.Close();
  if (m_buffer)
  {
    delete m_buffer;
    m_buffer = NULL;
  }
  m_outputBufferIndex = 0;
  m_outputBitstream = false;
  m_planes = 1;

  m_started = false;
}

void CAESinkDARWINOSX::GetDelay(AEDelayStatus& status)
{
  /* lockless way of guaranteeing consistency of tick/delay/buffer,
   * this work since render callback is short and quick and higher
   * priority compared to this thread, unsigned int are assumed
   * aligned and having atomic read/write */
  unsigned int size;
  CAESpinLock lock(m_render_locker);
  do
  {
    status.tick  = m_render_tick;
    status.delay = m_render_delay;
    if(m_buffer)
      size = m_buffer->GetReadSize();
    else
      size = 0;

  } while(lock.retry());

  status.delay += (double)size / (double)m_frameSizePerPlane / (double)m_framesPerSecond;
  status.delay += (double)m_latentFrames / (double)m_framesPerSecond;
}

double CAESinkDARWINOSX::GetCacheTotal()
{
  return (double)m_buffer->GetMaxSize() / (double)(m_frameSizePerPlane * m_framesPerSecond);
}

CCriticalSection mutex;
XbmcThreads::ConditionVariable condVar;

unsigned int CAESinkDARWINOSX::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (m_buffer->GetWriteSize() < frames * m_frameSizePerPlane)
  { // no space to write - wait for a bit
    std::unique_lock<CCriticalSection> lock(mutex);
    auto timeout = std::chrono::milliseconds(900 * frames / m_framesPerSecond);
    if (!m_started)
      timeout = 4500ms;

    // we are using a timer here for being sure for timeouts
    // condvar can be woken spuriously as signaled
    XbmcThreads::EndTime<> timer(timeout);
    condVar.wait(mutex, timeout);
    if (!m_started && timer.IsTimePast())
    {
      CLog::Log(LOGERROR, "{} engine didn't start in {} ms!", __FUNCTION__, timeout.count());
      return INT_MAX;
    }
  }

  unsigned int write_frames = std::min(frames, m_buffer->GetWriteSize() / m_frameSizePerPlane);
  if (write_frames)
  {
    for (unsigned int i = 0; i < m_buffer->NumPlanes(); i++)
      m_buffer->Write(data[i] + offset * m_frameSizePerPlane, write_frames * m_frameSizePerPlane, i);
  }
  return write_frames;
}

void CAESinkDARWINOSX::Drain()
{
  int bytes = m_buffer->GetReadSize();
  int totalBytes = bytes;
  int maxNumTimeouts = 3;
  auto timeout = std::chrono::milliseconds(900 * bytes / (m_framesPerSecond * m_frameSizePerPlane));
  while (bytes && maxNumTimeouts > 0)
  {
    std::unique_lock<CCriticalSection> lock(mutex);
    XbmcThreads::EndTime<> timer(timeout);
    condVar.wait(mutex, timeout);

    bytes = m_buffer->GetReadSize();
    // if we timeout and don't
    // consume bytes - decrease maxNumTimeouts
    if (timer.IsTimePast() && bytes == totalBytes)
      maxNumTimeouts--;
    totalBytes = bytes;
  }
}

void CAESinkDARWINOSX::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  EnumerateDevices();
  list.clear();
  for (CADeviceList::const_iterator i = s_devices.begin(); i != s_devices.end(); ++i)
    list.push_back(i->second);
}

inline void LogLevel(unsigned int got, unsigned int wanted)
{
  static unsigned int lastReported = INT_MAX;
  if (got != wanted)
  {
    if (got != lastReported)
    {
      CLog::Log(LOGWARNING, "DARWINOSX: {}flow ({} vs {} bytes)", got > wanted ? "over" : "under",
                got, wanted);
      lastReported = got;
    }
  }
  else
    lastReported = INT_MAX; // indicate we were good at least once
}

OSStatus CAESinkDARWINOSX::renderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData)
{
  CAESinkDARWINOSX *sink = (CAESinkDARWINOSX*)inClientData;

  sink->m_render_locker.enter(); /* grab lock */
  sink->m_started = true;
  if (outOutputData->mNumberBuffers)
  {
    //planar always starts at outputbuffer/streamidx 0
    unsigned int startIdx = sink->m_buffer->NumPlanes() == 1 ? sink->m_outputBufferIndex : 0;
    unsigned int endIdx = startIdx + sink->m_buffer->NumPlanes();

    /* NOTE: We assume that the buffers are all the same size... */
    if (sink->m_outputBitstream)
    {
      /* HACK for bitstreaming AC3/DTS via PCM.
       We reverse the float->S16LE conversion done in the stream or device */
      static const float mul = 1.0f / (INT16_MAX + 1);

      size_t wanted = outOutputData->mBuffers[0].mDataByteSize / sizeof(float) * sizeof(int16_t);
      size_t bytes = std::min((size_t)sink->m_buffer->GetReadSize(), wanted);
      for (unsigned int j = 0; j < bytes / sizeof(int16_t); j++)
      {
        for (unsigned int i = startIdx; i < endIdx; i++)
        {
          int16_t src = 0;
          sink->m_buffer->Read((unsigned char *)&src, sizeof(int16_t), i);
          if (i < outOutputData->mNumberBuffers && outOutputData->mBuffers[i].mData)
          {
            float *dest = (float *)outOutputData->mBuffers[i].mData;
            dest[j] = src * mul;
          }
        }
      }
      LogLevel(bytes, wanted);
    }
    else
    {
      /* buffers appear to come from CA already zero'd, so just copy what is wanted */
      unsigned int wanted = outOutputData->mBuffers[0].mDataByteSize;
      unsigned int bytes = std::min(sink->m_buffer->GetReadSize(), wanted);
      for (unsigned int i = startIdx; i < endIdx; i++)
      {
        if (i < outOutputData->mNumberBuffers && outOutputData->mBuffers[i].mData)
          sink->m_buffer->Read((unsigned char *)outOutputData->mBuffers[i].mData, bytes, i);
        else
          sink->m_buffer->Read(NULL, bytes, i);
      }
      LogLevel(bytes, wanted);
    }

    // tell the sink we're good for more data
    condVar.notifyAll();
  }

  sink->m_render_delay = (double)(inOutputTime->mHostTime - inNow->mHostTime) / CurrentHostFrequency();
  sink->m_render_tick  = inNow->mHostTime;
  sink->m_render_locker.leave();
  return noErr;
}
