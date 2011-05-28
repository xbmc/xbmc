/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __arm__
#include "CoreAudioAEHALOSX.h"

#include <PlatformDefs.h>
#include <math.h>
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "system.h"
#include "CoreAudioAE.h"
#include "AEUtil.h"
#include "AEFactory.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioHardware
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


AudioStreamBasicDescription *CCoreAudioHardware::FormatsList(AudioStreamID stream)
{
  OSStatus ret;
  AudioStreamBasicDescription *list;
  UInt32 listSize;
  AudioDevicePropertyID p;
  
  
  // This is deprecated for kAudioStreamPropertyAvailablePhysicalFormats,
  // but compiling on 10.3 requires the older constant
  p = kAudioStreamPropertyPhysicalFormats;
  
  // Retrieve all the stream formats supported by this output stream
  ret = AudioStreamGetPropertyInfo(stream, 0, p, &listSize, NULL);
  if (ret != noErr)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FormatsList: Unable to get list size. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return NULL;
  }
  
  // Space for a terminating ID:
  listSize += sizeof(AudioStreamBasicDescription);
  list = (AudioStreamBasicDescription *)malloc(listSize);
  
  if (list == NULL)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::FormatsList: Out of memory?");
    return NULL;
  }
  
  ret = AudioStreamGetProperty(stream, 0, p, &listSize, list);
  if (ret != noErr)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FormatsList: Unable to get list. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    free(list);
    return NULL;
  }
  
  // Add a terminating ID:
  list[listSize/sizeof(AudioStreamID)].mFormatID = 0;
  
  return list;
}

/**
 * Get a list of all the streams on this device
 */
AudioStreamID *CCoreAudioHardware::StreamsList(AudioDeviceID device)
{
  OSStatus ret;
  UInt32 listSize;
  AudioStreamID *list;
  
  
  ret = AudioDeviceGetPropertyInfo(device, 0, FALSE,
                                   kAudioDevicePropertyStreams,
                                   &listSize, NULL);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: Unable to get list size. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return NULL;
  }
  
  // Space for a terminating ID:
  listSize += sizeof(AudioStreamID);
  list = (AudioStreamID *)malloc(listSize);
  
  if (list == NULL)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: Out of memory?");
    return NULL;
  }
  
  ret = AudioDeviceGetProperty(device, 0, FALSE,
                               kAudioDevicePropertyStreams,
                               &listSize, list);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: Unable to get list. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return NULL;
  }
  
  // Add a terminating ID:
  list[listSize/sizeof(AudioStreamID)] = kAudioHardwareBadStreamError;
  
  return list;
}

/**
 * Reset any devices with an AC3 stream back to a Linear PCM
 * so that they can become a default output device
 */
void CCoreAudioHardware::ResetAudioDevices()
{
  AudioDeviceID *devices;
  int numDevices;
  UInt32 size;
  
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  devices = (AudioDeviceID*)malloc(size);
  if (!devices)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::ResetAudioDevices: ResetAudioDevices - out of memory?");
    return;
  }
  numDevices = size / sizeof(AudioDeviceID);
  AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
  
  for (int i = 0; i < numDevices; i++)
  {
    AudioStreamID *streams;
    
    streams = StreamsList(devices[i]);
    for (int j = 0; streams[j] != kAudioHardwareBadStreamError; j++)
      ResetStream(streams[j]);
    
    free(streams);
  }
  free(devices);
}

void CCoreAudioHardware::ResetStream(AudioStreamID stream)
{
  AudioStreamBasicDescription currentFormat;
  OSStatus ret;
  UInt32 paramSize;
  
  // Find the streams current physical format
  paramSize = sizeof(currentFormat);
  AudioStreamGetProperty(stream, 0, kAudioStreamPropertyPhysicalFormat,
                         &paramSize, &currentFormat);
  
  // If it's currently AC-3/SPDIF then reset it to some mixable format
  if (currentFormat.mFormatID == 'IAC3' ||
      currentFormat.mFormatID == kAudioFormat60958AC3)
  {
    AudioStreamBasicDescription *formats = CCoreAudioHardware::FormatsList(stream);
    bool streamReset = false;
    
    
    if (!formats)
      return;
    
    for (int i = 0; !streamReset && formats[i].mFormatID != 0; i++)
      if (formats[i].mFormatID == kAudioFormatLinearPCM)
      {
        ret = AudioStreamSetProperty(stream, NULL, 0, kAudioStreamPropertyPhysicalFormat, sizeof(formats[i]), &(formats[i]));
        if (ret != noErr)
        {
          CLog::Log(LOGDEBUG, "CCoreAudioHardware::ResetStream: Unable to retrieve the list of available devices. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
          continue;
        }
        else
        {
          streamReset = true;
          sleep(10); // For the change to take effect
        }
      }
    
    free(formats);
  }
}

AudioDeviceID CCoreAudioHardware::FindAudioDevice(CStdString searchName)
{
  if (!searchName.length())
    return 0;
  
  UInt32 size = 0;
  AudioDeviceID deviceId = 0;
  OSStatus ret;
	
  if (searchName.Equals("default"))
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

void CCoreAudioHardware::GetOutputDeviceName(CStdString& name)
{
  UInt32 size = 0;
  char *m_buffer;
  AudioDeviceID deviceId = GetDefaultOutputDevice();
  
  if(deviceId)
  {
    AudioDeviceGetPropertyInfo(deviceId,0, false, kAudioDevicePropertyDeviceName, &size, NULL); // TODO: Change to kAudioObjectPropertyObjectName
    m_buffer = (char *)malloc(size);
    
    OSStatus ret = AudioDeviceGetProperty(deviceId, 0, false, kAudioDevicePropertyDeviceName, &size, m_buffer);  
    if (ret && !m_buffer)
    {
      name ="Default";
    }
    else 
    {
      name = m_buffer;
      free(m_buffer);
    }
  }
  else 
  {
    name = "Default";
  }
	
  
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioDevice::CCoreAudioDevice()  : 
m_DeviceId(0),
m_Started(false),
m_HogPid(-1),
m_MixerRestore(-1),
m_IoProc(NULL),
m_ObjectListenerProc(NULL),
m_SampleRateRestore(0.0f)
{
  
}

CCoreAudioDevice::CCoreAudioDevice(AudioDeviceID deviceId) : 
m_DeviceId(deviceId),
m_Started(false),
m_HogPid(-1),
m_MixerRestore(-1),
m_IoProc(NULL),
m_ObjectListenerProc(NULL),
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
  m_ObjectListenerProc = NULL;
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

void CCoreAudioDevice::RemoveObjectListenerProc(AudioObjectPropertyListenerProc callback, void* pClientData) {
  if (!m_DeviceId)
    return;
  
  AudioObjectPropertyAddress audioProperty;
  audioProperty.mSelector = kAudioObjectPropertySelectorWildcard;
  audioProperty.mScope = kAudioObjectPropertyScopeWildcard;
  audioProperty.mElement = kAudioObjectPropertyElementWildcard;
  
  OSStatus ret = AudioObjectRemovePropertyListener(m_DeviceId, &audioProperty, callback, pClientData);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::RemoveObjectListenerProc: Unable to set ObjectListener callback. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  }
  m_ObjectListenerProc = NULL;
}

bool CCoreAudioDevice::SetObjectListenerProc(AudioObjectPropertyListenerProc callback, void* pClientData)
{
  if (!m_DeviceId || m_ObjectListenerProc) // Only one ObjectListener at a time
    return false;
  
  AudioObjectPropertyAddress audioProperty;
  audioProperty.mSelector = kAudioObjectPropertySelectorWildcard;
  audioProperty.mScope = kAudioObjectPropertyScopeWildcard;
  audioProperty.mElement = kAudioObjectPropertyElementWildcard;
  
  OSStatus ret = AudioObjectAddPropertyListener(m_DeviceId, &audioProperty, callback, pClientData);
  
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetObjectListenerProc: Unable to remove ObjectListener callback. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  
  m_ObjectListenerProc = callback;
  return true;
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

OSStatus CCoreAudioDevice::SetAudioProperty(AudioObjectID id,
																						AudioObjectPropertySelector selector,
																						UInt32 inDataSize, void *inData)
{
	AudioObjectPropertyAddress property_address;
	
	property_address.mSelector = selector;
	property_address.mScope    = kAudioObjectPropertyScopeGlobal;
	property_address.mElement  = kAudioObjectPropertyElementMaster;
	
	return AudioObjectSetPropertyData(id, &property_address, 0, NULL, inDataSize, inData);
}

Boolean CCoreAudioDevice::IsAudioPropertySettable(AudioObjectID id,
																									AudioObjectPropertySelector selector,
																									Boolean *outData)
{
	AudioObjectPropertyAddress property_address;
	
	property_address.mSelector = selector;
	property_address.mScope    = kAudioObjectPropertyScopeGlobal;
	property_address.mElement  = kAudioObjectPropertyElementMaster;
	
	return AudioObjectIsPropertySettable(id, &property_address, outData);
}

UInt32 CCoreAudioDevice::GetAudioPropertyArray(AudioObjectID id,
																							 AudioObjectPropertySelector selector,
																							 AudioObjectPropertyScope scope,
																							 void **outData)
{
	OSStatus err;
	AudioObjectPropertyAddress property_address;
	UInt32 i_param_size;
	
	property_address.mSelector = selector;
	property_address.mScope    = scope;
	property_address.mElement  = kAudioObjectPropertyElementMaster;
	
	err = AudioObjectGetPropertyDataSize(id, &property_address, 0, NULL, &i_param_size);
	
	if (err != noErr)
		return 0;
	
	*outData = malloc(i_param_size);
	
	
	err = AudioObjectGetPropertyData(id, &property_address, 0, NULL, &i_param_size, *outData);
	
	if (err != noErr) {
		free(*outData);
		return 0;
	}
	
	return i_param_size;
}

UInt32 CCoreAudioDevice::GetGlobalAudioPropertyArray(AudioObjectID id,
																										 AudioObjectPropertySelector selector,
																										 void **outData)
{
	return GetAudioPropertyArray(id, selector, kAudioObjectPropertyScopeGlobal, outData);
}

OSStatus CCoreAudioDevice::GetAudioPropertyString(AudioObjectID id,
																									AudioObjectPropertySelector selector,
																									char **outData)
{
	OSStatus err;
	AudioObjectPropertyAddress property_address;
	UInt32 i_param_size;
	CFStringRef string;
	CFIndex string_length;
	
	property_address.mSelector = selector;
	property_address.mScope    = kAudioObjectPropertyScopeGlobal;
	property_address.mElement  = kAudioObjectPropertyElementMaster;
	
	i_param_size = sizeof(CFStringRef);
	err = AudioObjectGetPropertyData(id, &property_address, 0, NULL, &i_param_size, &string);
	if (err != noErr)
		return err;
	
	string_length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(string),
																										kCFStringEncodingASCII);
	*outData = (char *)malloc(string_length + 1);
	CFStringGetCString(string, *outData, string_length + 1, kCFStringEncodingASCII);
	
	CFRelease(string);
	
	return err;
}

OSStatus CCoreAudioDevice::GetAudioProperty(AudioObjectID id,
																						AudioObjectPropertySelector selector,
																						UInt32 outSize, void *outData)
{
	AudioObjectPropertyAddress property_address;
	
	property_address.mSelector = selector;
	property_address.mScope    = kAudioObjectPropertyScopeGlobal;
	property_address.mElement  = kAudioObjectPropertyElementMaster;
	
	return AudioObjectGetPropertyData(id, &property_address, 0, NULL, &outSize, outData);
}

bool CCoreAudioDevice::SetHogStatus(bool hog)
{
	OSStatus ret;
	
  if (!m_DeviceId)
    return false;
  
	if(!hog && (m_HogPid != -1)) 
	{
		CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Releas 'hog' mode on device 0x%04x", m_DeviceId);
		
		m_HogPid = -1;
		ret = SetAudioProperty(m_DeviceId, kAudioDevicePropertyHogMode, sizeof(m_HogPid), &m_HogPid);
		
		if(ret) {
			CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable relese 'hog' mode. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
			return false;
		}
	} 
	
	if(hog)
	{
		CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Set 'hog' mode on device 0x%04x", m_DeviceId);
		
    ret = GetAudioProperty(m_DeviceId, kAudioDevicePropertyHogMode, sizeof(pid_t), &m_HogPid);
    
    if (ret != noErr) {
      CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable read 'hog' mode. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
      return false;
    }
    
    if(m_HogPid != getpid()) {
      CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Device used by another process.");
      return false;
    }
		
		m_HogPid = getpid();
    
		ret = SetAudioProperty(m_DeviceId, kAudioDevicePropertyHogMode, sizeof(pid_t), &m_HogPid);
		
		if(ret) {
			CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable set 'hog' mode. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
			m_HogPid = -1;
			return false;
		}
	}
	
  return true;
}

bool CCoreAudioDevice::SetMixingSupport(UInt32 mix)
{
	OSStatus ret;
	Boolean writeable = false;
	
  if (!m_DeviceId)
    return false;
	
	ret = IsAudioPropertySettable(m_DeviceId, kAudioDevicePropertySupportsMixing, &writeable);	
	
	if(!writeable) {
		CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetMixingSupport: Mixing not writable on device 0x%04x", m_DeviceId);
		return false;
	}
	
	ret = GetAudioProperty(m_DeviceId, kAudioDevicePropertySupportsMixing, sizeof(UInt32), &mix);	
	
	if (ret != noErr) {
		
		m_MixerRestore = (GetMixingSupport() ? 1 : 0);
		
		ret = SetAudioProperty(m_DeviceId, kAudioDevicePropertySupportsMixing, sizeof(UInt32), &mix);
		
	}
	
	if(ret != noErr) {
		m_MixerRestore = -1;
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetMixingSupport: Unable to set MixingSupport to %s. Error = 0x%08x (%4.4s)", mix ? "'On'" : "'Off'", ret, CONVERT_OSSTATUS(ret));
		return false;
	}
	
	CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetMixingSupport: %sabling mixing for device 0x%04x",mix ? "En" : "Dis",  m_DeviceId);		
	return true;
}

bool CCoreAudioDevice::GetMixingSupport()
{
  if (!m_DeviceId)
    return false;
  UInt32 mix = 0;
	
  OSStatus ret = GetAudioProperty(m_DeviceId, kAudioDevicePropertySupportsMixing, sizeof(UInt32), &mix);
	
  if (ret == noErr)
    return false;
	
  return (mix > 0);
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
  if (m_Initialized && m_Component)
  {
		
    AudioUnitUninitialize(m_Component);
    AudioUnitReset(m_Component, kAudioUnitScope_Input, NULL);
    CloseComponent(m_Component);
  }
  
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


bool CCoreAudioUnit::GetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus)
{
  if (!m_Component || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioUnitProperty_StreamFormat, scope, bus, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetFormat: Unable to get AudioUnit format. Bus : %d Scope : %d : Error = 0x%08x (%4.4s)", bus, scope, ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CCoreAudioUnit::SetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus)
{
  if (!m_Component || !pDesc)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_StreamFormat, scope, bus, pDesc, sizeof(AudioStreamBasicDescription));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetFormat: Unable to set AudioUnit format. Bus : %d Scope : %d : Error = 0x%08x (%4.4s)", bus, scope, ret, CONVERT_OSSTATUS(ret));
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
  m_DeviceId = 0;
}

CAUOutputDevice::~CAUOutputDevice()
{
  
}

bool CAUOutputDevice::SetCurrentDevice(AudioDeviceID deviceId)
{
  if (!m_Component)
    return false;
  
  OSStatus ret;
	
  m_DeviceId = 0;
  
	ret = AudioUnitSetProperty(m_Component, 
														 kAudioOutputUnitProperty_CurrentDevice, 
														 kAudioUnitScope_Global, 
														 kOutputBus, 
														 &deviceId, 
														 sizeof(deviceId));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentDevice: Unable to set current device. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false; 
  }
  
  m_DeviceId = deviceId;
  return true;
}

bool CAUOutputDevice::GetInputChannelMap(std::list<SInt32> &pChannelMap)
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
      pChannelMap.push_back(pMap[i]); 
	
  delete[] pMap;
  return (!ret);
}

bool CAUOutputDevice::SetInputChannelMap(std::list<SInt32> &pChannelMap)
{
	// The number of array elements must match the number of output channels provided by the device
  if (!m_Component)
    return false;
	
  UInt32 channels = pChannelMap.size();
  UInt32 size = sizeof(SInt32) * channels;
  SInt32* pMap = new SInt32[channels];
	int i = 0;
	
	for(std::list<SInt32>::iterator itt = pChannelMap.begin(); itt != pChannelMap.end(); ++itt) {
		pMap[i] = *itt;
		i++;
	}
	
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetInputChannelMap: Unable to set AudioUnit input channel map. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  delete[] pMap;
  return (!ret);
}

bool CAUOutputDevice::SetOutputChannelMap(std::list<SInt32> &pChannelMap)
{
	// The number of array elements must match the number of output channels provided by the device
  if (!m_Component)
    return false;
	
  UInt32 channels = pChannelMap.size();
  UInt32 size = sizeof(SInt32) * channels;
  SInt32* pMap = new SInt32[channels];
	int i = 0;
	
	for(std::list<SInt32>::iterator itt = pChannelMap.begin(); itt != pChannelMap.end(); ++itt) {
		pMap[i] = *itt;
		i++;
	}
	
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Global, 1, pMap, size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetOutputChannelMap: Unable to set AudioUnit ouput channel map. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
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
  if (!m_Component && !m_DeviceId)
    return false;
	
  OSStatus    ret;
  UInt32      size;
	Boolean			canset	= false;
  
  size = sizeof canset;
	
  ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyVolumeScalar, &size, &canset);
  
	if(ret == noErr && canset==true) {
		size = sizeof vol;
		ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyVolumeScalar, size, &vol);
		
    if(!ret)
    {
      CLog::Log(LOGDEBUG, "CCoreAudioUnit::SetCurrentVolume: Set AudioUnit Device[0x%04x] volume to %f using kAudioDevicePropertyVolumeScalar", m_Component, vol);
      return true;
    }
  }
  
  ret = AudioUnitSetParameter(m_Component, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentVolume: Unable to set AudioUnit volume. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  CLog::Log(LOGDEBUG, "CCoreAudioUnit::SetCurrentVolume: Set AudioUnit Device[0x%04x] volume to %f uing kHALOutputParam_Volume", m_Component, vol);
	
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

UInt32 CAUOutputDevice::SetBufferFrameSize(UInt32 frames)
{
  if (!m_Component)
    return 0;
  
  UInt32 size = sizeof(UInt32);
  UInt32 bufferSize = 0;
	
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, 
                                      &frames, size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetBufferFrameSize: Unable to set device's buffer size to %d. ErrCode = Error = 0x%08x (%4.4s)", frames, ret, CONVERT_OSSTATUS(ret));
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioAEHALOSX
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCoreAudioAEHALOSX::CCoreAudioAEHALOSX() :
	m_Initialized(false),
	m_Passthrough(false),
	m_BytesPerFrame(0),
	m_BytesPerSec(0),
	m_NumLatencyFrames(0),
	m_OutputBufferIndex(0)
{
	m_AUOutput      = new CAUOutputDevice;
	m_MixerUnit			= new CCoreAudioUnit;
	m_AudioDevice		= new CCoreAudioDevice;
	m_OutputStream	= new CCoreAudioStream;	
}

CCoreAudioAEHALOSX::~CCoreAudioAEHALOSX()
{
  delete m_AUOutput;
	delete m_MixerUnit;
	delete m_AudioDevice;
	delete m_OutputStream;
}

bool CCoreAudioAEHALOSX::InitializePCM(AEAudioFormat &format, CStdString &device, unsigned int bps)
{ 
  // Create the MatrixMixer AudioUnit Component
  if (!m_MixerUnit->Open(kAudioUnitType_Mixer, kAudioUnitSubType_MatrixMixer, kAudioUnitManufacturer_Apple))
    return false;
  
  // Set the input stream format for the AudioUnit (this is what is being sent to us)
  AudioStreamBasicDescription inputFormat;
  inputFormat.mFormatID = kAudioFormatLinearPCM;                  //  Data encoding format
  inputFormat.mFormatFlags = /*kAudioFormatFlagsNativeEndian | */ kLinearPCMFormatFlagIsPacked;
  
  switch(format.m_dataFormat) {
    case AE_FMT_FLOAT:
      inputFormat.mFormatFlags |= kAudioFormatFlagIsFloat;
      break;
    default:
      inputFormat.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
  }
  
#ifdef __BIG_ENDIAN__
  inputFormat.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
  
  inputFormat.mChannelsPerFrame = format.m_channelCount;          // Number of interleaved audiochannels
  inputFormat.mSampleRate = (Float64)format.m_sampleRate;         //  the sample rate of the audio stream
  inputFormat.mBitsPerChannel =  bps;                             // Number of bits per sample, per channel
  inputFormat.mBytesPerFrame = (bps>>3) * format.m_channelCount;    // Size of a frame == 1 sample per channel    
  inputFormat.mFramesPerPacket = 1;                               // The smallest amount of indivisible data. Always 1 for uncompressed audio   
  inputFormat.mBytesPerPacket = inputFormat.mBytesPerFrame * inputFormat.mFramesPerPacket;
  inputFormat.mReserved = 0;
  
  AudioStreamBasicDescription inputDesc_end;
  CStdString formatString;
	
  m_AUOutput->GetFormat(&inputDesc_end, kAudioUnitScope_Output, kInputBus);
	
  if (!m_AUOutput->SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
    return false;
	
  if (!m_AUOutput->SetFormat(&inputFormat, kAudioUnitScope_Output, kInputBus))
    return false;
	
  m_BytesPerFrame = inputFormat.mBytesPerFrame;
  m_BytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame;      // 1 sample per channel per frame
  
  return true;
}

bool CCoreAudioAEHALOSX::InitializePCMEncoded(AEAudioFormat &format, CStdString &device, unsigned int bps)
{
  m_AudioDevice->SetHogStatus(true); // Prevent any other application from using this device.
  m_AudioDevice->SetMixingSupport(false); // Try to disable mixing support. Effectiveness depends on the device.
  
  // Set the Sample Rate as defined by the spec.
  m_AudioDevice->SetNominalSampleRate((float)format.m_sampleRate);
  
  if (!InitializePCM(format, device, bps))
    return false;
  
  return true;  
}

bool CCoreAudioAEHALOSX::InitializeEncoded(AudioDeviceID outputDevice, AEAudioFormat &format, unsigned int bps)
{  
  CStdString formatString;
  AudioStreamBasicDescription outputFormat = {0};
  AudioStreamID outputStream = 0;
  bool bFound = false;
  
  // Fetch a list of the streams defined by the output device
  AudioStreamIdList streams;
  UInt32  streamIndex = 0;
  m_AudioDevice->GetStreams(&streams);
  
  m_OutputBufferIndex = 0;
  
  while (!streams.empty())
  {
    // Get the next stream
    CCoreAudioStream stream;
    stream.Open(streams.front());
    streams.pop_front(); // We copied it, now we are done with it
    
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Found %s stream - id: 0x%04X, Terminal Type: 0x%04lX",
              stream.GetDirection() ? "Input" : "Output",
              stream.GetId(),
              stream.GetTerminalType());
    
    // Probe physical formats
    StreamFormatList physicalFormats;
    stream.GetAvailablePhysicalFormats(&physicalFormats);
    while (!physicalFormats.empty())
    {
      AudioStreamRangedDescription& desc = physicalFormats.front();
      CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded:    Considering Physical Format: %s", StreamDescriptionToString(desc.mFormat, formatString));
      if (desc.mFormat.mFormatID == kAudioFormat60958AC3 || desc.mFormat.mFormatID == 'IAC3')
      {
        outputFormat = desc.mFormat; // Select this format
        m_OutputBufferIndex = streamIndex;
        outputStream = stream.GetId();
        
        /* Adjust samplerate */
        outputFormat.mChannelsPerFrame = format.m_channelCount;
        outputFormat.mSampleRate = (Float64)format.m_sampleRate;
        stream.SetPhysicalFormat(&outputFormat);
				
        m_AUOutput->SetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus);
        
        bFound = true;
        
        break;
      }
      physicalFormats.pop_front();
    }
    
    if (bFound)
      break; // We found a suitable format. No need to continue.
		
    streamIndex++;
  }
  
  if (!bFound) // No match found
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Unable to identify suitable output format.");
    return false;
  }
  
  m_BytesPerSec = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3) * outputFormat.mSampleRate; // mBytesPerFrame is 0 for a cac3 stream  
  m_BytesPerFrame = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3);
  
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Selected stream[%lu] - id: 0x%04lX, Physical Format: %s (%lu Bytes/sec.)", m_OutputBufferIndex, outputStream, StreamDescriptionToString(outputFormat, formatString), m_BytesPerSec);
  
  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)
  
  m_AudioDevice->SetHogStatus(true); // Hog the device if it is not set to be done automatically
  m_AudioDevice->SetMixingSupport(false); // Try to disable mixing. If we cannot, it may not be a problem
  
  m_NumLatencyFrames = m_AudioDevice->GetNumLatencyFrames();
  
  // Configure the output stream object
  m_OutputStream->Open(outputStream); // This is the one we will keep
  AudioStreamBasicDescription previousFormat;
  m_OutputStream->GetPhysicalFormat(&previousFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Previous Physical Format: %s (%lu Bytes/sec.)", StreamDescriptionToString(previousFormat, formatString), m_BytesPerSec);
  m_OutputStream->SetPhysicalFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
  m_NumLatencyFrames += m_OutputStream->GetNumLatencyFrames();
  
  // Register for data request callbacks from the driver
  m_AudioDevice->AddIOProc(m_ae->RenderCallbackDirect, m_ae);
  
  return true;
}

bool CCoreAudioAEHALOSX::Initialize(IAE *ae, bool passThrough, AEAudioFormat &format, CStdString &device)
{ 
	m_ae = (CCoreAudioAE *)ae;

	if(!m_ae)
		return false;
	
	m_Passthrough = passThrough;
	
  int bPassthrough = m_Passthrough;
  unsigned int bps = CAEUtil::DataFormatToBits(format.m_dataFormat);;
  
  if (format.m_channelCount == 0)
  {
    CLog::Log(LOGERROR, "CCoreAudioAEHALOSX::Initialize - Unable to open the requested channel layout");
    return false;
  }
  
  // Reset all the devices to a default 'non-hog' and mixable format.
  // If we don't do this we may be unable to find the Default Output device.
  // (e.g. if we crashed last time leaving it stuck in AC-3 mode)
  CCoreAudioHardware::ResetAudioDevices();
  
  device.Replace("CoreAudio:", "");
  
  AudioDeviceID outputDevice = CCoreAudioHardware::FindAudioDevice(device);
  
  if (!outputDevice) // Fall back to the default device if no match is found
  {
    CLog::Log(LOGWARNING, "CCoreAudioAEHALOSX::Initialize: Unable to locate configured device, falling-back to the system default.");
    outputDevice = CCoreAudioHardware::GetDefaultOutputDevice();
    if (!outputDevice) // Not a lot to be done with no device. TODO: Should we just grab the first existing device?
      return false;
  }
  
  // Attach our output object to the device
  m_AudioDevice->Open(outputDevice);
  
  // If this is a passthrough (AC3/DTS) stream, attempt to handle it natively
  if (bPassthrough)
  {
    m_Passthrough = InitializeEncoded(outputDevice, format, bps);
    Sleep(100);
  }
  
  // If this is a PCM stream, or we failed to handle a passthrough stream natively,
  // prepare the standard interleaved PCM interface
  if (!m_Passthrough)
  {    
    // Create the Output AudioUnit Component
    if (!m_AUOutput->Open(kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple))
      return false;
    
    // Hook the Ouput AudioUnit to the selected device
    if (!m_AUOutput->SetCurrentDevice(outputDevice))
      return false;
    
    // If we are here and this is a passthrough stream, native handling failed.
    // Try to handle it as IEC61937 data over straight PCM (DD-Wav)
    bool configured = false;
    if (bPassthrough)
    {
      CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::Initialize: No suitable AC3 output format found. Attempting DD-Wav.");
      configured = InitializePCMEncoded(format, device, bps);
      // TODO: wait for audio device startup
      Sleep(100);
    }
    else
    {
      // Standard PCM data
      configured = InitializePCM(format, device, bps);
      // TODO: wait for audio device startup
      Sleep(100);
    }
    
    if (!configured) // No suitable output format was able to be configured
      return false;
    
    // Configure the maximum number of frames that the AudioUnit will ask to process at one time.
    // If this is not called, there is no guarantee that the callback will ever be called.
    m_AUOutput->SetBufferFrameSize(512);
    
    // Setup the callback function that the AudioUnit will use to request data  
    if (!m_AUOutput->SetRenderProc(m_ae->RenderCallback, m_ae))
      return false;
		
    // Initialize the Output AudioUnit
    if (!m_AUOutput->Initialize())
      return false;
    
    // Initialize the Output AudioUnit
    //if (!m_MixerUnit.Initialize())
    //  return false;
    
    // Log some information about the stream
    AudioStreamBasicDescription inputDesc_end, outputDesc_end;
    CStdString formatString;
    m_AUOutput->GetFormat(&inputDesc_end, kAudioUnitScope_Output, kInputBus);
		
    if(inputDesc_end.mChannelsPerFrame != format.m_channelCount) {
      CLog::Log(LOGERROR, "CCoreAudioAEHALOSX::Initialize: Output channel count does not match input channel count. in %d : out %d. Please Correct your speaker layout setting.", 
                format.m_channelCount, inputDesc_end.mChannelsPerFrame);      
      format.m_channelCount = inputDesc_end.mChannelsPerFrame;
      if(!InitializePCM(format, device, bps))
      {
        CLog::Log(LOGERROR, "CCoreAudioAEHALOSX::Initialize: Reinit with right channel count failed"); 
        return false;
      }
      Sleep(100);
    }
		
    m_AUOutput->GetFormat(&inputDesc_end, kAudioUnitScope_Output, kInputBus);
    m_AUOutput->GetFormat(&outputDesc_end, kAudioUnitScope_Input, kOutputBus);
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::Initialize: Input Stream Format %s", StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::Initialize: Output Stream Format % s", StreamDescriptionToString(outputDesc_end, formatString));    
    
  }
	
  m_NumLatencyFrames = m_AudioDevice->GetNumLatencyFrames();
  
  // set the format parameters
  format.m_frameSize    = m_BytesPerFrame;
  
	m_Initialized = true;
	
  return true;
}

void CCoreAudioAEHALOSX::Deinitialize()
{
  if(!m_Initialized)
    return;
	
  if (m_Passthrough)
    m_AudioDevice->RemoveIOProc();
  else
    m_AUOutput->SetRenderProc(nil, nil);
	
  m_AUOutput->Close();
  m_OutputStream->Close();
  Sleep(10);
  m_AudioDevice->Close();
  Sleep(100);
  
  m_BytesPerSec = 0;
  m_BytesPerFrame = 0;
	m_BytesPerSec = 0;
	m_NumLatencyFrames = 0;
	m_OutputBufferIndex = 0;
	
  m_BytesPerSec = 0;
  
  m_Initialized = false;
	m_Passthrough = false;
  
  CLog::Log(LOGINFO, "CCoreAudioAEHALOSX::Deinitialize: Audio device has been closed.");
}

void CCoreAudioAEHALOSX::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  CoreAudioDeviceList deviceList;
  CCoreAudioHardware::GetOutputDevices(&deviceList);
  
  CStdString defaultDeviceName;
  CCoreAudioHardware::GetOutputDeviceName(defaultDeviceName);
  
  CStdString deviceName;
  for (int i = 0; !deviceList.empty(); i++)
  {
    CCoreAudioDevice device(deviceList.front());
    device.GetName(deviceName);

    CStdString deviceName_Internal = CStdString("CoreAudio:") + deviceName;
    devices.push_back(AEDevice(deviceName, deviceName_Internal));

    printf("deviceName_Internal %s\n", deviceName_Internal.c_str());
		
    deviceList.pop_front();
  }	
}

void CCoreAudioAEHALOSX::Stop()
{
  if(!m_Initialized)
    return;
	
  if (m_Passthrough)
    m_AudioDevice->Stop();
  else
    m_AUOutput->Stop();
}

bool CCoreAudioAEHALOSX::Start()
{
  if(!m_Initialized)
    return false;
  
  if (m_Passthrough)
    m_AudioDevice->Start();
  else
    m_AUOutput->Start();

	return true;
}

float CCoreAudioAEHALOSX::GetDelay()
{   
  float delay;
	
  delay += (float)(m_NumLatencyFrames * m_BytesPerFrame) / m_BytesPerSec;
	
  return delay;
}


#endif
