#ifdef __APPLE__
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "CoreAudio.h"
#include <PlatformDefs.h>
#include <utils/log.h>
#include <math.h>

char* UInt32ToFourCC(UInt32* pVal) // NOT NULL TERMINATED! Modifies input value.
{
  UInt32 inVal = *pVal;
  char* pIn = (char*)&inVal;
  char* fourCC = (char*)pVal;
  fourCC[3] = pIn[0];
  fourCC[2] = pIn[1];
  fourCC[1] = pIn[2];
  fourCC[0] = pIn[3];
  return fourCC;
}

const char* StreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str)
{
  UInt32 formatId = desc.mFormatID;
  char* fourCC = UInt32ToFourCC(&formatId);
  
  switch (desc.mFormatID)
  {
    case kAudioFormatLinearPCM:
      str.Format("[%4.4s] %s%u Channel %u-bit %s (%uHz)", 
                 fourCC,
                 (desc.mFormatFlags & kAudioFormatFlagIsNonMixable) ? "" : "Mixable ",
                 desc.mChannelsPerFrame,
                 desc.mBitsPerChannel,
                 (desc.mFormatFlags & kAudioFormatFlagIsFloat) ? "Floating Point" : "Signed Integer",
                 (UInt32)desc.mSampleRate);
      break;
    case kAudioFormatAC3:
      str.Format("[%4.4s] AC-3/DTS (%uHz)", fourCC, (UInt32)desc.mSampleRate);
      break;
    case kAudioFormat60958AC3:
      str.Format("[%4.4s] AC-3/DTS for S/PDIF (%uHz)", fourCC, (UInt32)desc.mSampleRate);
      break;
    default:
      str.Format("[%4.4s]", fourCC);
      break;
  }
  return str.c_str();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioHardware
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceID CCoreAudioHardware::FindAudioDevice(CStdString searchName)
{
  if (!searchName.length())
    return 0;
  
  UInt32 size = 0;
  AudioDeviceID deviceId = 0;
  OSStatus ret;
 
  if (searchName.Equals("Default Output Device"))
  {
    AudioDeviceID defaultDevice = GetDefaultOutputDevice();
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice: Returning default device [0x%04x].", defaultDevice);
    return defaultDevice;  
  }
  CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice: Searching for device - %s.", searchName.c_str());
  
  // Obtain a list of all available audio devices
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  UInt32 deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  ret = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, pDevices);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioHardware::FindAudioDevice: Unable to retrieve the list of available devices. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    delete[] pDevices;
    return 0; 
  }
  
  // Attempt to locate the requested device
  CStdString deviceName;
  for (UInt32 dev = 0; dev < deviceCount; dev++)
  {
    CCoreAudioDevice device;
    device.Open((pDevices[dev]));
    device.GetName(deviceName);
    UInt32 totalChannels = device.GetTotalOutputChannels();
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice:   Device[0x%04x] - Name: '%s', Total Ouput Channels: %u. ", pDevices[dev], deviceName.c_str(), totalChannels);
    if (searchName.Equals(deviceName))
      deviceId = pDevices[dev];
    if (deviceId)
      break;
  }
  delete[] pDevices;  
  
  return deviceId;
}

AudioDeviceID CCoreAudioHardware::GetDefaultOutputDevice()
{
  UInt32 size = sizeof(AudioDeviceID);
  AudioDeviceID deviceId = 0;
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &deviceId);
  if (ret || !deviceId) // outputDevice is set to 0 if there is no audio device available, or if the default device is set to an encoded format
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetDefaultOutputDevice: Unable to identify default output device. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
    return 0;
  }
  return deviceId;
}

UInt32 CCoreAudioHardware::GetOutputDevices(CoreAudioDeviceList* pList)
{
  if (!pList)
    return 0;
  
  // Obtain a list of all available audio devices
  UInt32 found = 0;
  UInt32 size = 0;
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  UInt32 deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, pDevices);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetOutputDevices: Unable to retrieve the list of available devices. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  else
  {
    for (UInt32 dev = 0; dev < deviceCount; dev++)
    {
      CCoreAudioDevice device(pDevices[dev]);
      if (device.GetTotalOutputChannels() == 0)
        continue;
      found++;
      pList->push_back(pDevices[dev]);
    }
  }
  delete[] pDevices;
  return found;
}

bool CCoreAudioHardware::GetAutoHogMode()
{
  UInt32 val = 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyHogModeIsAllowed, &size, &val);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetAutoHogMode: Unable to get auto 'hog' mode. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return (val == 1);
}

void CCoreAudioHardware::SetAutoHogMode(bool enable)
{
  UInt32 val = enable ? 1 : 0;
  OSStatus ret = AudioHardwareSetProperty(kAudioHardwarePropertyHogModeIsAllowed, sizeof(val), &val);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioHardware::SetAutoHogMode: Unable to set auto 'hog' mode. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioDevice::CCoreAudioDevice()  : 
  m_DeviceId(0),
  m_Started(false),
  m_Hog(-1),
  m_MixerRestore(-1),
  m_IoProc(NULL),
  m_SampleRateRestore(0.0f)
{
  
}

CCoreAudioDevice::CCoreAudioDevice(AudioDeviceID deviceId) : 
  m_DeviceId(deviceId),
  m_Started(false),
  m_Hog(-1),
  m_MixerRestore(-1),
  m_IoProc(NULL),
  m_SampleRateRestore(0.0f)
{
  
}

CCoreAudioDevice::~CCoreAudioDevice()
{
  Close();
}

bool CCoreAudioDevice::Open(AudioDeviceID deviceId)
{
  m_DeviceId = deviceId;
  return true;
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::Open: Opened device 0x%04x", m_DeviceId);
}

void CCoreAudioDevice::Close()
{
  if (!m_DeviceId)
    return;
  
  Stop(); // Stop the device if it was started
  
  RemoveIOProc(); // Unregister the IOProc if we have one
  
  SetHogStatus(false);
  if (m_MixerRestore > -1) // We changed the mixer status
    SetMixingSupport((m_MixerRestore ? true : false));
  m_MixerRestore = -1;
  
  if (m_SampleRateRestore != 0.0f)
  {
    CLog::Log(LOGDEBUG,  "CCoreAudioUnit::Close: Restoring original nominal samplerate.");    
    SetNominalSampleRate(m_SampleRateRestore);
  }
  
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::Close: Closed device 0x%04x", m_DeviceId);
  m_DeviceId = 0;
  m_IoProc = NULL;
  
}

void CCoreAudioDevice::Start()
{
  if (!m_DeviceId || m_Started) 
    return;
  
  OSStatus ret = AudioDeviceStart(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Start: Unable to start device. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
  else
    m_Started = true;
}

void CCoreAudioDevice::Stop()
{
  if (!m_DeviceId || !m_Started)
    return;
  
  OSStatus ret = AudioDeviceStop(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Stop: Unable to stop device. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
  m_Started = false;
}

bool CCoreAudioDevice::AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData)
{
  if (!m_DeviceId || m_IoProc) // Only one IOProc at a time
    return false;
  
  OSStatus ret = AudioDeviceAddIOProc(m_DeviceId, ioProc, pCallbackData);  
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::Stop: Unable to add IOProc. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  m_IoProc = ioProc;
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::AddIOProc: IOProc set for device 0x%04x", m_DeviceId);
  return true;
}

void CCoreAudioDevice::RemoveIOProc()
{
  if (!m_DeviceId || !m_IoProc)
    return;
  
  Stop();

  OSStatus ret = AudioDeviceRemoveIOProc(m_DeviceId, m_IoProc);  
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::RemoveIOProc: Unable to remove IOProc. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
  else
    CLog::Log(LOGDEBUG, "CCoreAudioDevice::AddIOProc: IOProc removed for device 0x%04x", m_DeviceId);
  m_IoProc = NULL; // Clear the reference no matter what
}

const char* CCoreAudioDevice::GetName(CStdString& name)
{
  if (!m_DeviceId)
    return NULL;

  UInt32 size = 0;
  AudioDeviceGetPropertyInfo(m_DeviceId,0, false, kAudioDevicePropertyDeviceName, &size, NULL); // TODO: Change to kAudioObjectPropertyObjectName
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyDeviceName, &size, name.GetBufferSetLength(size));  
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetName: Unable to get device name - id: 0x%04x Error = 0x%08x (%4.4s)", m_DeviceId, ret, CONVERT_OSSTATUS(ret));
    return NULL;
  }
  return name.c_str();
}

UInt32 CCoreAudioDevice::GetTotalOutputChannels()
{
  if (!m_DeviceId)
    return 0;
  UInt32 channels = 0;
	UInt32 size = 0;
  AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyStreamConfiguration, &size, NULL);
  AudioBufferList* pList = (AudioBufferList*)malloc(size);
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyStreamConfiguration, &size, pList); 
  if (!ret)
    for(UInt32 buffer = 0; buffer < pList->mNumberBuffers; ++buffer)
      channels += pList->mBuffers[buffer].mNumberChannels;
  else
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetTotalOutputChannels: Unable to get total device output channels - id: 0x%04x Error = 0x%08x (%4.4s)", m_DeviceId, ret, CONVERT_OSSTATUS(ret));
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::GetTotalOutputChannels: Found %u channels in %u buffers", channels, pList->mNumberBuffers);
  free(pList);
	return channels;  
}

bool CCoreAudioDevice::GetStreams(AudioStreamIdList* pList)
{
  if (!pList || !m_DeviceId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyStreams, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 streamCount = propertySize / sizeof(AudioStreamID);
  AudioStreamID* pStreamList = new AudioStreamID[streamCount];
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyStreams, &propertySize, pStreamList);
  if (!ret)
  {
    for (UInt32 stream = 0; stream < streamCount; stream++)
      pList->push_back(pStreamList[stream]);
  }
  delete[] pStreamList;
  return (ret == noErr);  
}


bool CCoreAudioDevice::IsRunning()
{
  UInt32 isRunning = false;
  UInt32 size = sizeof(isRunning);
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyDeviceIsRunning, &size, &isRunning);
  if (ret)
    return false;
  return (isRunning != 0);
}

bool CCoreAudioDevice::SetHogStatus(bool hog)
{
  // According to Jeff Moore (Core Audio, Apple), Setting kAudioDevicePropertyHogMode
  // is a toggle and the only way to tell if you do get hog mode is to compare
  // the returned pid against getpid, if the match, you have hog mode, if not you don't.
  if (!m_DeviceId)
    return false;
  
  if (hog)
  {
    if (m_Hog == -1) // Not already set
    {
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Setting 'hog' status on device 0x%04x", m_DeviceId);
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(m_Hog), &m_Hog);
      if (ret || m_Hog != getpid())
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to set 'hog' status. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
        return false;
      }
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Successfully set 'hog' status on device 0x%04x", m_DeviceId);
    }
  }
  else
  {
    if (m_Hog > -1) // Currently Set
    {
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Releasing 'hog' status on device 0x%04x", m_DeviceId);
      pid_t hogPid = -1;
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(hogPid), &hogPid);
      if (ret || hogPid == getpid())
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to release 'hog' status. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
        return false;
      }
      m_Hog = hogPid; // Reset internal state
    }
  }
  return true;
}

pid_t CCoreAudioDevice::GetHogStatus()
{
  if (!m_DeviceId)
    return false;
  
  pid_t hogPid = -1;
  UInt32 size = sizeof(hogPid);
  AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyHogMode, &size, &hogPid);

  return hogPid;
}

bool CCoreAudioDevice::SetMixingSupport(bool mix)
{
  if (!m_DeviceId)
    return false;
  int restore = -1;
  if (m_MixerRestore == -1) // This is our first change to this setting. Store the original setting for restore
    restore = (GetMixingSupport() ? 1 : 0);
  UInt32 mixEnable = mix ? 1 : 0;
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetMixingSupport: %sabling mixing for device 0x%04x",mix ? "En" : "Dis",  m_DeviceId);
  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertySupportsMixing, sizeof(mixEnable), &mixEnable);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetMixingSupport: Unable to set MixingSupport to %s. Error = 0x%08x (%4.4s)", mix ? "'On'" : "'Off'", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  if (m_MixerRestore == -1) 
    m_MixerRestore = restore;
  return true;
}

bool CCoreAudioDevice::GetMixingSupport()
{
  if (!m_DeviceId)
    return false;
  UInt32 val = 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertySupportsMixing, &size, &val);
  if (ret)
    return false;
  return (val > 0);
}

bool CCoreAudioDevice::GetPreferredChannelLayout(CoreAudioChannelList* pChannelMap)
{
  if (!pChannelMap || !m_DeviceId)
    return false;

  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyPreferredChannelLayout, &propertySize, &writable);
  if (ret)
    return false;
  
  // kAudioChannelLabel_Unknown = -1 (0xffffffff)
  // kAudioChannelLabel_Unused = 0
  // kAudioChannelLabel_Left = 1
  // kAudioChannelLabel_Right = 2
  // ...
  
  void* pBuf = malloc(propertySize);
  AudioChannelLayout* pLayout = (AudioChannelLayout*)pBuf;
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyPreferredChannelLayout, &propertySize, pBuf);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetPreferredChannelLayout: Unable to retrieve preferred channel layout. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  else
  {
    if(pLayout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
    {
      for (UInt32 i = 0; i < pLayout->mNumberChannelDescriptions; i++)
      {
        if (pLayout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_Unknown)
          pChannelMap->push_back(i + 1); // TODO: This is not the best way to handle unknown/unconfigured speaker layouts
        else
          pChannelMap->push_back(pLayout->mChannelDescriptions[i].mChannelLabel); // Will be one of kAudioChannelLabel_xxx
      }
    }
    else
    {
      // TODO: Determine if a method that uses a channel bitmap is also necessary
      free(pLayout);
      return false;
    }
  } 

  free(pLayout);
  return (ret == noErr);  
}

bool CCoreAudioDevice::GetDataSources(CoreAudioDataSourceList* pList)
{
  if (!pList || !m_DeviceId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyDataSources, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 sources = propertySize / sizeof(UInt32);
  UInt32* pSources = new UInt32[sources];
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyDataSources, &propertySize, pSources);
  if (!ret)
    for (UInt32 i = 0; i < sources; i++)
      pList->push_back(pSources[i]);;
  delete[] pSources;
  return (!ret);    
}

Float64 CCoreAudioDevice::GetNominalSampleRate()
{
  if (!m_DeviceId)
    return 0.0f;
  
  Float64 sampleRate = 0.0f;
  UInt32 size = sizeof(Float64);
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyNominalSampleRate, &size, &sampleRate);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetNominalSampleRate: Unable to retrieve current device sample rate. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return 0.0f;
  }
  return sampleRate;
}

bool CCoreAudioDevice::SetNominalSampleRate(Float64 sampleRate)
{
  if (!m_DeviceId || sampleRate == 0.0f)
    return false;
  
  Float64 currentRate = GetNominalSampleRate();
  if (currentRate == sampleRate)
    return true; //No need to change
    
  UInt32 size = sizeof(Float64);
  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyNominalSampleRate, size, &sampleRate);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetNominalSampleRate: Unable to set current device sample rate to %0.0f. Error = 0x%08x (%4.4s)", (float)sampleRate, ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  CLog::Log(LOGDEBUG,  "CCoreAudioUnit::SetNominalSampleRate: Changed device sample rate from %0.0f to %0.0f.", (float)currentRate, (float)sampleRate);
  if (m_SampleRateRestore == 0.0f)
    m_SampleRateRestore = currentRate;
  
  return true;
}

UInt32 CCoreAudioDevice::GetNumLatencyFrames()
{
  UInt32 i_param, i_param_size, num_latency_frames = 0;
  if (!m_DeviceId)
    return 0;  

  i_param_size = sizeof(uint32_t);

  // number of frames of latency in the AudioDevice
  if (noErr == AudioDeviceGetProperty(m_DeviceId, 0, false, 
    kAudioDevicePropertyLatency, &i_param_size, &i_param))
  {
    num_latency_frames += i_param;
  }
 
  // number of frames in the IO buffers
  if (noErr == AudioDeviceGetProperty(m_DeviceId, 0, false,
    kAudioDevicePropertyBufferFrameSize, &i_param_size, &i_param))
  {
    num_latency_frames += i_param;
  }
 
  // number for frames in ahead the current hardware position that is safe to do IO
  if (noErr == AudioDeviceGetProperty(m_DeviceId, 0, false, 
    kAudioDevicePropertySafetyOffset, &i_param_size, &i_param))
 	{
    num_latency_frames += i_param;
  }
  
  return(num_latency_frames);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioStream
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioStream::CCoreAudioStream() :
m_StreamId(0)
{
  m_OriginalVirtualFormat.mFormatID = 0;
  m_OriginalPhysicalFormat.mFormatID = 0;
}

CCoreAudioStream::~CCoreAudioStream()
{
  Close();
}

bool CCoreAudioStream::Open(AudioStreamID streamId)
{
  m_StreamId = streamId;
  CLog::Log(LOGDEBUG, "CCoreAudioStream::Open: Opened stream 0x%04x.", m_StreamId);
  return true;
}

// TODO: Should it even be possible to change both the physical and virtual formats, since the devices do it themselves?
void CCoreAudioStream::Close()
{
  if (!m_StreamId)
    return;
  
  // Revert any format changes we made
  if (m_OriginalVirtualFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Restoring original virtual format for stream 0x%04x.", m_StreamId);
    SetVirtualFormat(&m_OriginalVirtualFormat);
  }
  if (m_OriginalPhysicalFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Restoring original physical format for stream 0x%04x.", m_StreamId);
    SetPhysicalFormat(&m_OriginalPhysicalFormat);
  }
  
  m_OriginalPhysicalFormat.mFormatID = 0;
  m_OriginalVirtualFormat.mFormatID = 0;
  CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Closed stream 0x%04x.", m_StreamId);
  m_StreamId = 0;
}

UInt32 CCoreAudioStream::GetDirection()
{
  if (!m_StreamId)
    return 0;
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyDirection, &size, &val);
  if (ret)
    return 0;
  return val;
}

UInt32 CCoreAudioStream::GetTerminalType()
{
  if (!m_StreamId)
    return 0;
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyTerminalType, &size, &val);
  if (ret)
    return 0;
  return val;  
}

UInt32 CCoreAudioStream::GetNumLatencyFrames()
{
  UInt32 i_param, i_param_size, num_latency_frames = 0;
  if (!m_StreamId)
    return 0;

  i_param_size = sizeof(uint32_t);

  // number of frames of latency in the AudioStream
  if (noErr == AudioStreamGetProperty(m_StreamId, 0, 
    kAudioStreamPropertyLatency, &i_param_size, &i_param))
  {
    num_latency_frames += i_param;
  }

  return(num_latency_frames);
}

bool CCoreAudioStream::GetVirtualFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyVirtualFormat, &size, pDesc);
  if (ret)
    return false;
  return true;
}

bool CCoreAudioStream::SetVirtualFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  if (!m_OriginalVirtualFormat.mFormatID)
  {
    if (!GetVirtualFormat(&m_OriginalVirtualFormat)) // Store the original format (as we found it) so that it can be restored later
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to retrieve current virtual format for stream 0x%04x.", m_StreamId);
      return false;
    }
  }
  OSStatus ret = AudioStreamSetProperty(m_StreamId, NULL, 0, kAudioStreamPropertyVirtualFormat, sizeof(AudioStreamBasicDescription), pDesc);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to set virtual format for stream 0x%04x. Error = 0x%08x (%4.4s)", m_StreamId, ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;   
}

bool CCoreAudioStream::GetPhysicalFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyPhysicalFormat, &size, pDesc);
  if (ret)
    return false;
  return true;   
}

bool CCoreAudioStream::SetPhysicalFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  if (!m_OriginalPhysicalFormat.mFormatID)
  {
    if (!GetPhysicalFormat(&m_OriginalPhysicalFormat)) // Store the original format (as we found it) so that it can be restored later
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetPhysicalFormat: Unable to retrieve current physical format for stream 0x%04x.", m_StreamId);
      return false;
    }
  }  
  OSStatus ret = AudioStreamSetProperty(m_StreamId, NULL, 0, kAudioStreamPropertyPhysicalFormat, sizeof(AudioStreamBasicDescription), pDesc);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to set physical format for stream 0x%04x. Error = 0x%08x (%4.4s)", m_StreamId, ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;   
}

bool CCoreAudioStream::GetAvailableVirtualFormats(StreamFormatList* pList)
{
  if (!pList || !m_StreamId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioStreamGetPropertyInfo(m_StreamId, 0, kAudioStreamPropertyAvailableVirtualFormats, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 formatCount = propertySize / sizeof(AudioStreamRangedDescription);
  AudioStreamRangedDescription* pFormatList = new AudioStreamRangedDescription[formatCount];
  ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyAvailableVirtualFormats, &propertySize, pFormatList);
  if (!ret)
  {
    for (UInt32 format = 0; format < formatCount; format++)
      pList->push_back(pFormatList[format]);
  }
  delete[] pFormatList;
  return (ret == noErr);
}

bool CCoreAudioStream::GetAvailablePhysicalFormats(StreamFormatList* pList)
{
  if (!pList || !m_StreamId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioStreamGetPropertyInfo(m_StreamId, 0, kAudioStreamPropertyAvailablePhysicalFormats, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 formatCount = propertySize / sizeof(AudioStreamRangedDescription);
  AudioStreamRangedDescription* pFormatList = new AudioStreamRangedDescription[formatCount];
  ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyAvailablePhysicalFormats, &propertySize, pFormatList);
  if (!ret)
  {
    for (UInt32 format = 0; format < formatCount; format++)
      pList->push_back(pFormatList[format]);
  }
  delete[] pFormatList;
  return (ret == noErr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioUnit
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioUnit::CCoreAudioUnit() :
  m_Initialized(false),
  m_Component(NULL)
{
  
}

CCoreAudioUnit::~CCoreAudioUnit() 
{
  Close();
}

bool CCoreAudioUnit::Open(ComponentDescription desc)
{
  if (m_Component)
    Close();
  
  // Find the required Component
	Component outputComp = FindNextComponent(NULL, &desc);
	if (outputComp == NULL)  // Unable to find the AudioUnit we requested
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::Open: Unable to locate AudioUnit Component.");
    return false;
  }
	
  // Create an instance of the AudioUnit Component
  OSStatus ret = OpenAComponent(outputComp, &m_Component);
	if (ret) // Unable to open AudioUnit
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::Open: Unable to open AudioUnit Component. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false; 
  }
  return true;
}

bool CCoreAudioUnit::Open(OSType type, OSType subType, OSType manufacturer)
{
  ComponentDescription desc;
  desc.componentType = type;
  desc.componentSubType = subType;
  desc.componentManufacturer = manufacturer;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  return Open(desc);
}


void CCoreAudioUnit::Close()
{
  if (m_Initialized)
    AudioUnitUninitialize(m_Component);
  if (m_Component)
    CloseComponent(m_Component);
  m_Initialized = false;
  m_Component = 0;
}

bool CCoreAudioUnit::Initialize()
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitInitialize(m_Component);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::Initialize: Unable to Initialize AudioUnit. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false; 
  } 
  m_Initialized = true;
  return true;
}


bool CCoreAudioUnit::GetInputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputFormat: Unable to get AudioUnit input format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CCoreAudioUnit::GetOutputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputFormat: Unable to get AudioUnit output format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CCoreAudioUnit::SetInputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, pDesc, sizeof(AudioStreamBasicDescription));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetInputFormat: Unable to set AudioUnit input format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;  
}

bool CCoreAudioUnit::SetOutputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, pDesc, sizeof(AudioStreamBasicDescription));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetInputFormat: Unable to set AudioUnit output format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;  
}

bool CCoreAudioUnit::SetRenderProc(AURenderCallback callback, void* pClientData)
{
  if (!m_Component)
    return false;
  
  AURenderCallbackStruct callbackInfo;
	callbackInfo.inputProc = callback; // Function to be called each time the AudioUnit needs data
	callbackInfo.inputProcRefCon = pClientData; // Pointer to be returned in the callback proc
	OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callbackInfo, sizeof(AURenderCallbackStruct));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetRenderProc: Unable to set AudioUnit render callback. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}


bool CCoreAudioUnit::SetMaxFramesPerSlice(UInt32 maxFrames)
{
  if (!m_Component)
    return false;
  
	OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetMaxFramesPerSlice: Unable to set AudioUnit max frames per slice. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAUOutputDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: The channel map setter/getter are inefficient

CAUOutputDevice::CAUOutputDevice()
{
  
}

CAUOutputDevice::~CAUOutputDevice()
{
  
}

bool CAUOutputDevice::SetCurrentDevice(AudioDeviceID deviceId)
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &deviceId, sizeof(AudioDeviceID));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentDevice: Unable to set current device. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false; 
  }  
  return true;
}

bool CAUOutputDevice::GetInputChannelMap(CoreAudioChannelList* pChannelMap)
{
  if (!m_Component)
    return false;
  
  UInt32 size = 0;
  Boolean writable = false;
  AudioUnitGetPropertyInfo(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, &size, &writable);
  UInt32 channels = size/sizeof(SInt32);
  SInt32* pMap = new SInt32[channels];
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, &size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputChannelMap: Unable to retrieve AudioUnit input channel map. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  else
    for (UInt32 i = 0; i < channels; i++)
      pChannelMap->push_back(pMap[i]);  
  delete[] pMap;
  return (!ret);
}

bool CAUOutputDevice::SetInputChannelMap(CoreAudioChannelList* pChannelMap)
{
	// The number of array elements must match the number of output channels provided by the device
  if (!m_Component || !pChannelMap)
    return false;
  UInt32 channels = pChannelMap->size();
  UInt32 size = sizeof(SInt32) * channels;
  SInt32* pMap = new SInt32[channels];
  for (UInt32 i = 0; i < channels; i++)
    pMap[i] = (*pChannelMap)[i];
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: Unable to get current device's buffer size. ErrCode = Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  delete[] pMap;
  return (!ret);
}

void CAUOutputDevice::Start()
{
  // TODO: Check component status
  if (m_Component && m_Initialized)
    AudioOutputUnitStart(m_Component);  
}

void CAUOutputDevice::Stop()
{
  // TODO: Check component status
  if (m_Component && m_Initialized)
    AudioOutputUnitStop(m_Component);    
}

Float32 CAUOutputDevice::GetCurrentVolume()
{
  if (!m_Component)
    return 0.0f;
  
  Float32 volPct = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_Component,  kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &volPct);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetCurrentVolume: Unable to get AudioUnit volume. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return 0.0f;
  }
  return volPct;
}

bool CAUOutputDevice::SetCurrentVolume(Float32 vol)
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitSetParameter(m_Component, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentVolume: Unable to set AudioUnit volume. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CAUOutputDevice::IsRunning()
{
  if (!m_Component)
    return false;
  
  UInt32 isRunning = 0;
  UInt32 size = sizeof(isRunning);
  AudioUnitGetProperty(m_Component, kAudioOutputUnitProperty_IsRunning, kAudioUnitScope_Global, 0, &isRunning, &size);
  return (isRunning != 0);
}

UInt32 CAUOutputDevice::GetBufferFrameSize()
{
  if (!m_Component)
    return 0;
  
  UInt32 size = sizeof(UInt32);
  UInt32 bufferSize = 0;
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, &bufferSize, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: Unable to get current device's buffer size. ErrCode = Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return 0;
  }
  return bufferSize;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAUMatrixMixer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CAUMatrixMixer::CAUMatrixMixer()
{
  
}

CAUMatrixMixer::~CAUMatrixMixer()
{
  
}



#endif
