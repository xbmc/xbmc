/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Sinks/AESinkDARWINOSX.h"
#include "cores/AudioEngine/Utils/AERingBuffer.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioHelpers.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioHardware.h"
#include "cores/AudioEngine/Sinks/osx/AEDeviceEnumerationOSX.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"

static void EnumerateDevices(CADeviceList &list)
{
  std::string defaultDeviceName;
  CCoreAudioHardware::GetOutputDeviceName(defaultDeviceName);

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
    //we rather might need the concatination of all streams *sucks*
    if(defaultDeviceName == devEnum.GetMasterDeviceName())
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
    CSingleLock lock(s_devicesLock);
    s_devices = devices;
  }
}

static CADeviceList GetDevices()
{
  CADeviceList list;
  {
    CSingleLock lock(s_devicesLock);
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
    CAEFactory::DeviceChange();
    CLog::Log(LOGDEBUG, "CoreAudio: audiodevicelist changed - done");
  }
  return noErr;
}

////////////////////////////////////////////////////////////////////////////////////////////
CAESinkDARWINOSX::CAESinkDARWINOSX()
: m_latentFrames(0),
  m_outputBufferIndex(0),
  m_outputBitstream(false),
  m_planes(1),
  m_frameSizePerPlane(0),
  m_framesPerSecond(0),
  m_buffer(NULL),
  m_started(false),
  m_render_tick(0),
  m_render_delay(0.0)
{
  // By default, kAudioHardwarePropertyRunLoop points at the process's main thread on SnowLeopard,
  // If your process lacks such a run loop, you can set kAudioHardwarePropertyRunLoop to NULL which
  // tells the HAL to run it's own thread for notifications (which was the default prior to SnowLeopard).
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

bool CAESinkDARWINOSX::Initialize(AEAudioFormat &format, std::string &device)
{
  AudioDeviceID deviceID = 0;
  UInt32 requestedStreamIndex = INT_MAX;
  UInt32 requestedSourceId = INT_MAX;

  CADeviceList devices = GetDevices();
  if (StringUtils::EqualsNoCase(device, "default"))
  {
    CCoreAudioHardware::GetOutputDeviceName(device);
    deviceID = CCoreAudioHardware::GetDefaultOutputDevice();
    CLog::Log(LOGNOTICE, "%s: Opening default device %s", __PRETTY_FUNCTION__, device.c_str());
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
          CLog::Log(LOGNOTICE, "%s pseudo device - requesting stream %d", __FUNCTION__, (unsigned int)requestedStreamIndex);
        if (requestedSourceId != INT_MAX)
          CLog::Log(LOGNOTICE, "%s device - requesting audiosource %d", __FUNCTION__, (unsigned int)requestedSourceId);
        break;
      }
    }
  }

  if (!deviceID)
  {
    CLog::Log(LOGERROR, "%s: Unable to find device %s", __FUNCTION__, device.c_str());
    return false;
  }

  AEDeviceEnumerationOSX devEnum(deviceID);
  AudioStreamBasicDescription outputFormat = { 0 };
  AudioStreamID outputStream = 0;
  UInt32 numOutputChannels = 0;
  EPassthroughMode passthrough = PassthroughModeNone;
  m_planes = 1;
  // after FindSuitableFormatForStream requestedStreamIndex will have a valid index and no INT_MAX anymore ...
  if (devEnum.FindSuitableFormatForStream(requestedStreamIndex, format, outputFormat, passthrough, outputStream))
  {
    numOutputChannels = outputFormat.mChannelsPerFrame;

    if (devEnum.IsPlanar())
    {
      numOutputChannels = std::min((size_t)format.m_channelLayout.Count(), (size_t)devEnum.GetNumPlanes());
      m_planes = numOutputChannels;
      CLog::Log(LOGDEBUG, "%s Found planar audio with %u channels using %u of them.", __FUNCTION__, (unsigned int)devEnum.GetNumPlanes(), (unsigned int)numOutputChannels);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "%s, Unable to find suitable stream", __FUNCTION__);
    return false;
  }


  /* Update our AE format */
  format.m_sampleRate    = outputFormat.mSampleRate;
  
  m_outputBufferIndex = requestedStreamIndex;
  m_outputBitstream   = passthrough == PassthroughModeBitstream;

  std::string formatString;
  CLog::Log(LOGDEBUG, "%s: Selected stream[%u] - id: 0x%04X, Physical Format: %s %s", __FUNCTION__, (unsigned int)m_outputBufferIndex, (unsigned int)outputStream, StreamDescriptionToString(outputFormat, formatString), m_outputBitstream ? "bitstreamed passthrough" : "");

  m_device.Open(deviceID);
  SetHogMode(passthrough != PassthroughModeNone);

  // Configure the output stream object
  m_outputStream.Open(outputStream);

  AudioStreamBasicDescription virtualFormat, previousPhysicalFormat;
  m_outputStream.GetVirtualFormat(&virtualFormat);
  m_outputStream.GetPhysicalFormat(&previousPhysicalFormat);
  CLog::Log(LOGDEBUG, "%s: Previous Virtual Format: %s", __FUNCTION__, StreamDescriptionToString(virtualFormat, formatString));
  CLog::Log(LOGDEBUG, "%s: Previous Physical Format: %s", __FUNCTION__, StreamDescriptionToString(previousPhysicalFormat, formatString));

  m_outputStream.SetPhysicalFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
  m_outputStream.GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "%s: New Virtual Format: %s", __FUNCTION__, StreamDescriptionToString(virtualFormat, formatString));
  CLog::Log(LOGDEBUG, "%s: New Physical Format: %s", __FUNCTION__, StreamDescriptionToString(outputFormat, formatString));

  if (requestedSourceId != INT_MAX && !m_device.SetDataSource(requestedSourceId))
    CLog::Log(LOGERROR, "%s: Error setting requested audio source.", __FUNCTION__);

  m_latentFrames = m_device.GetNumLatencyFrames();
  m_latentFrames += m_outputStream.GetNumLatencyFrames();

  // update the channel map based on the new stream format
  if (passthrough == PassthroughModeNone)
    devEnum.GetAEChannelMap(format.m_channelLayout, numOutputChannels);
   
  /* TODO: Should we use the virtual format to determine our data format? */
  format.m_frameSize     = format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  format.m_frames        = m_device.GetBufferSize();
  format.m_frameSamples  = format.m_frames * format.m_channelLayout.Count();

  m_frameSizePerPlane = format.m_frameSize / m_planes;
  m_framesPerSecond   = format.m_sampleRate;

  if (m_outputBitstream)
  { /* TODO: Do we need this? */
    m_device.SetNominalSampleRate(format.m_sampleRate);
  }

  unsigned int num_buffers = 4;
  m_buffer = new AERingBuffer(num_buffers * format.m_frames * m_frameSizePerPlane, m_planes);
  CLog::Log(LOGDEBUG, "%s: using buffer size: %u (%f ms)", __FUNCTION__, m_buffer->GetMaxSize(), (float)m_buffer->GetMaxSize() / (m_framesPerSecond * m_frameSizePerPlane));

  if (m_outputBitstream)
    format.m_dataFormat = AE_FMT_S16NE;
  else if (passthrough == PassthroughModeNone)
    format.m_dataFormat = (m_planes > 1) ? AE_FMT_FLOATP : AE_FMT_FLOAT;

  // Register for data request callbacks from the driver and start
  m_device.AddIOProc(renderCallback, this);
  m_device.Start();
  return true;
}

void CAESinkDARWINOSX::SetHogMode(bool on)
{
  // TODO: Auto hogging sets this for us. Figure out how/when to turn it off or use it
  // It appears that leaving this set will aslo restore the previous stream format when the
  // Application exits. If auto hogging is set and we try to set hog mode, we will deadlock
  // From the SDK docs: "If the AudioDevice is in a non-mixable mode, the HAL will automatically take hog mode on behalf of the first process to start an IOProc."

  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)
  if (on)
  {
    // Auto-Hog does not always un-hog the device when changing back to a mixable mode.
    // Handle this on our own until it is fixed.
    CCoreAudioHardware::SetAutoHogMode(false);
    bool autoHog = CCoreAudioHardware::GetAutoHogMode();
    CLog::Log(LOGDEBUG, " CoreAudioRenderer::InitializeEncoded: "
              "Auto 'hog' mode is set to '%s'.", autoHog ? "On" : "Off");
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
    CSingleLock lock(mutex);
    unsigned int timeout = 900 * frames / m_framesPerSecond;
    if (!m_started)
      timeout = 4500;

    // we are using a timer here for beeing sure for timeouts
    // condvar can be woken spuriously as signaled
    XbmcThreads::EndTime timer(timeout);
    condVar.wait(mutex, timeout);
    if (!m_started && timer.IsTimePast())
    {
      CLog::Log(LOGERROR, "%s engine didn't start in %d ms!", __FUNCTION__, timeout);
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
  unsigned int timeout = 900 * bytes / (m_framesPerSecond * m_frameSizePerPlane);
  while (bytes && maxNumTimeouts > 0)
  {
    CSingleLock lock(mutex);
    XbmcThreads::EndTime timer(timeout);
    condVar.wait(mutex, timeout);

    bytes = m_buffer->GetReadSize();
    // if we timeout and don't
    // consum bytes - decrease maxNumTimeouts
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
      CLog::Log(LOGWARNING, "DARWINOSX: %sflow (%u vs %u bytes)", got > wanted ? "over" : "under", got, wanted);
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
          int16_t src;
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
