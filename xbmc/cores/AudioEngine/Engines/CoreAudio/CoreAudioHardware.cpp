/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "CoreAudioHardware.h"

#include "CoreAudioAEHAL.h"
#include "utils/log.h"

bool CCoreAudioHardware::GetAutoHogMode()
{
  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyHogModeIsAllowed; 

  UInt32 val = 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, &val); 
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetAutoHogMode: "
      "Unable to get auto 'hog' mode. Error = %s", GetError(ret).c_str());
    return false;
  }
  return (val == 1);
}

void CCoreAudioHardware::SetAutoHogMode(bool enable)
{
  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyHogModeIsAllowed; 

  UInt32 val = enable ? 1 : 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioObjectSetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, size, &val); 
  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioHardware::SetAutoHogMode: "
      "Unable to set auto 'hog' mode. Error = %s", GetError(ret).c_str());
}

AudioStreamBasicDescription* CCoreAudioHardware::FormatsList(AudioStreamID stream)
{
  // Retrieve all the stream formats supported by this output stream

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyPhysicalFormats; 

  UInt32 listSize = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(stream, &propertyAddress, 0, NULL, &listSize); 
  if (ret != noErr)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FormatsList: "
      "Unable to get list size. Error = %s", GetError(ret).c_str());
    return NULL;
  }

  // Space for a terminating ID:
  listSize += sizeof(AudioStreamBasicDescription);
  AudioStreamBasicDescription *list = (AudioStreamBasicDescription*)malloc(listSize);
  if (list == NULL)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::FormatsList: Out of memory?");
    return NULL;
  }

  ret = AudioObjectGetPropertyData(stream, &propertyAddress, 0, NULL, &listSize, list); 
  if (ret != noErr)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FormatsList: "
      "Unable to get list. Error = %s", GetError(ret).c_str());
    free(list);
    return NULL;
  }

  // Add a terminating ID:
  list[listSize/sizeof(AudioStreamID)].mFormatID = 0;

  return list;
}

AudioStreamID* CCoreAudioHardware::StreamsList(AudioDeviceID device)
{
  // Get a list of all the streams on this device
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioDevicePropertyStreams;

  UInt32 listSize;
  OSStatus ret = AudioObjectGetPropertyDataSize(device, &propertyAddress, 0, NULL, &listSize); 
  if (ret != noErr)
    return NULL;

  // Space for a terminating ID:
  listSize += sizeof(AudioStreamID);
  AudioStreamID *list = (AudioStreamID*)malloc(listSize);
  if (list == NULL)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: Out of memory?");
    return NULL;
  }

  propertyAddress.mScope = kAudioDevicePropertyScopeInput;
  ret = AudioObjectGetPropertyData(device, &propertyAddress, 0, NULL, &listSize, list);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: "
      "Unable to get list. Error = %s", GetError(ret).c_str());
    return NULL;
  }

  // Add a terminating ID:
  list[listSize/sizeof(AudioStreamID)] = kAudioHardwareBadStreamError;

  return list;
}

void CCoreAudioHardware::ResetAudioDevices()
{
  // Reset any devices with an AC3 stream back to a Linear PCM
  // so that they can become a default output device
  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyDevices; 

  UInt32 size;
  OSStatus ret = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size); 
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::ResetAudioDevices: ResetAudioDevices - unknown size");
    return;
  }

  AudioDeviceID *devices = (AudioDeviceID*)malloc(size);
  if (!devices)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::ResetAudioDevices: ResetAudioDevices - out of memory?");
    return;
  }
  int numDevices = size / sizeof(AudioDeviceID);
  ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, devices); 
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::ResetAudioDevices: ResetAudioDevices - cannot get device list");
    return;
  }

  for (int i = 0; i < numDevices; i++)
  {
    AudioStreamID *streams = StreamsList(devices[i]);
    if (streams)
    {
      for (int j = 0; streams[j] != kAudioHardwareBadStreamError; j++)
        ResetStream(streams[j]);
      free(streams);
    }
  }
  free(devices);
}

void CCoreAudioHardware::ResetStream(AudioStreamID stream)
{
  // Find the streams current physical format
  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyPhysicalFormat; 

  AudioStreamBasicDescription currentFormat;
  UInt32 paramSize = sizeof(currentFormat);
  OSStatus ret = AudioObjectGetPropertyData(stream, &propertyAddress, 0, NULL, &paramSize, &currentFormat);
  if (ret != noErr)
    return;

  // If it's currently AC-3/SPDIF then reset it to some mixable format
  if (currentFormat.mFormatID == 'IAC3' ||
      currentFormat.mFormatID == kAudioFormat60958AC3)
  {
    AudioStreamBasicDescription *formats = CCoreAudioHardware::FormatsList(stream);
    bool streamReset = false;

    if (!formats)
      return;

    for (int i = 0; !streamReset && formats[i].mFormatID != 0; i++)
    {
      if (formats[i].mFormatID == kAudioFormatLinearPCM)
      {
        ret = AudioObjectSetPropertyData(stream, &propertyAddress, 0, NULL, sizeof(formats[i]), &(formats[i])); 
        if (ret != noErr)
        {
          CLog::Log(LOGDEBUG, "CCoreAudioHardware::ResetStream: "
            "Unable to retrieve the list of available devices. Error = %s", GetError(ret).c_str());
          continue;
        }
        else
        {
          streamReset = true;
          Sleep(10);
        }
      }
    }
    free(formats);
  }
}

AudioDeviceID CCoreAudioHardware::FindAudioDevice(const std::string &searchName)
{
  AudioDeviceID deviceId = 0;

  if (!searchName.length())
    return deviceId;

  std::string searchNameLowerCase = searchName;
  std::transform(searchNameLowerCase.begin(), searchNameLowerCase.end(), searchNameLowerCase.begin(), ::tolower );
  if (searchNameLowerCase.compare("default") == 0)
  {
    AudioDeviceID defaultDevice = GetDefaultOutputDevice();
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice: "
      "Returning default device [0x%04x].", (uint)defaultDevice);
    return defaultDevice;
  }
  CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice: "
    "Searching for device - %s.", searchName.c_str());

  // Obtain a list of all available audio devices
  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyDevices; 

  UInt32 size = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size); 
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::FindAudioDevice: "
      "Unable to retrieve the size of the list of available devices. Error = %s", GetError(ret).c_str());
    return 0;
  }

  size_t deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, pDevices);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::FindAudioDevice: "
      "Unable to retrieve the list of available devices. Error = %s", GetError(ret).c_str());
    delete[] pDevices;
    return 0;
  }

  // Attempt to locate the requested device
  std::string deviceName;
  for (size_t dev = 0; dev < deviceCount; dev++)
  {
    CCoreAudioDevice device;
    device.Open((pDevices[dev]));
    deviceName = device.GetName();
    std::transform( deviceName.begin(), deviceName.end(), deviceName.begin(), ::tolower );
    if (searchNameLowerCase.compare(deviceName) == 0)
      deviceId = pDevices[dev];
    if (deviceId)
      break;
  }
  delete[] pDevices;

  return deviceId;
}

AudioDeviceID CCoreAudioHardware::GetDefaultOutputDevice()
{
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;

  AudioDeviceID deviceId = 0;
  UInt32 size = sizeof(AudioDeviceID);
  OSStatus ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, &deviceId);
  // outputDevice is set to 0 if there is no audio device available
  // or if the default device is set to an encoded format
  if (ret != noErr || !deviceId) 
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetDefaultOutputDevice:"
      " Unable to identify default output device. Error = %s", GetError(ret).c_str());
    return 0;
  }

  return deviceId;
}

void CCoreAudioHardware::GetOutputDeviceName(std::string& name)
{
  name = "Default";
  AudioDeviceID deviceId = GetDefaultOutputDevice();

  if (deviceId)
  {
    AudioObjectPropertyAddress  propertyAddress;
    propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
    propertyAddress.mSelector = kAudioObjectPropertyName;

    CFStringRef theDeviceName = NULL;
    UInt32 propertySize = sizeof(CFStringRef);
    OSStatus ret = AudioObjectGetPropertyData(deviceId, &propertyAddress, 0, NULL, &propertySize, &theDeviceName); 
    if (ret != noErr)
      return;

    const char *cstr = CFStringGetCStringPtr(theDeviceName, kCFStringEncodingUTF8);
    if (cstr)
      name = cstr;
    CFRelease(theDeviceName);
  }
}

UInt32 CCoreAudioHardware::GetOutputDevices(CoreAudioDeviceList *pList)
{
  UInt32 found = 0;
  if (!pList)
    return found;

  // Obtain a list of all available audio devices
  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyDevices; 

  UInt32 size = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size); 
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetOutputDevices:"
      " Unable to retrieve the size of the list of available devices. Error = %s", GetError(ret).c_str());
    return found;
  }

  size_t deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, pDevices);
  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetOutputDevices:"
      " Unable to retrieve the list of available devices. Error = %s", GetError(ret).c_str());
  else
  {
    for (size_t dev = 0; dev < deviceCount; dev++)
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
