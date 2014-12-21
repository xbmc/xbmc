/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "CoreAudioHelpers.h"
#include "utils/log.h"
#include "osx/DarwinUtils.h"

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

void CCoreAudioHardware::ResetAudioDevices()
{
  CLog::Log(LOGDEBUG, "CCoreAudioHardware::ResetAudioDevices resetting our devices to LPCM");
  CoreAudioDeviceList list;
  if (GetOutputDevices(&list))
  {
    for (CoreAudioDeviceList::iterator it = list.begin(); it != list.end(); ++it)
    {
      CCoreAudioDevice device = *it;

      AudioStreamIdList streams;
      if (device.GetStreams(&streams))
      {
        CLog::Log(LOGDEBUG, "CCoreAudioHardware::ResetAudioDevices %lu streams for device %s", streams.size(), device.GetName().c_str());
        for (AudioStreamIdList::iterator ait = streams.begin(); ait != streams.end(); ++ait)
          ResetStream(*ait);
      }
    }
  }
}

void CCoreAudioHardware::ResetStream(AudioStreamID streamId)
{
  CCoreAudioStream stream;
  stream.Open(streamId);
  
  AudioStreamBasicDescription desc;
  if (stream.GetPhysicalFormat(&desc))
  {
    if (desc.mFormatID == 'IAC3' || desc.mFormatID == kAudioFormat60958AC3)
    {
      CLog::Log(LOGDEBUG, "CCoreAudioHardware::ResetStream stream 0x%x is in encoded format.. setting to LPCM", (unsigned int)streamId);

      StreamFormatList availableFormats;
      if (stream.GetAvailablePhysicalFormats(&availableFormats))
      {
        for (StreamFormatList::iterator fmtIt = availableFormats.begin(); fmtIt != availableFormats.end() ; ++fmtIt)
        {
          AudioStreamRangedDescription fmtDesc = *fmtIt;
          if (fmtDesc.mFormat.mFormatID == kAudioFormatLinearPCM)
          {
            AudioStreamBasicDescription newFmt = fmtDesc.mFormat;

            if (stream.SetPhysicalFormat(&newFmt))
              break;
          }
        }
      }
    }
  }

  stream.Close(false);
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
  AudioDeviceID deviceId = 0;
  static AudioDeviceID lastDeviceId = 0;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  
  UInt32 size = sizeof(AudioDeviceID);
  OSStatus ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, &deviceId);

  // outputDevice is set to 0 if there is no audio device available
  // or if the default device is set to an encoded format
  if (ret != noErr || !deviceId) 
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetDefaultOutputDevice:"
      " Unable to identify default output device. Error = %s", GetError(ret).c_str());
    // if there was no error and no deviceId was returned
    // return the last known default device
    if (ret == noErr && !deviceId)
      return lastDeviceId;
    else
      return 0;
  }
  
  lastDeviceId = deviceId;

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

    CDarwinUtils::CFStringRefToUTF8String(theDeviceName, name);

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
