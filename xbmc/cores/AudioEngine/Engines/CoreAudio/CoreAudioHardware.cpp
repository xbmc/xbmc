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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "CoreAudioHardware.h"

#include "CoreAudioAEHAL.h"
#include "utils/log.h"

// AudioHardwareGetProperty and friends are deprecated,
// turn off the warning spew.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

bool CCoreAudioHardware::GetAutoHogMode()
{
  UInt32 val = 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyHogModeIsAllowed, &size, &val);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetAutoHogMode: "
      "Unable to get auto 'hog' mode. Error = %s", GetError(ret).c_str());
    return false;
  }
  return (val == 1);
}

void CCoreAudioHardware::SetAutoHogMode(bool enable)
{
  UInt32 val = enable ? 1 : 0;
  OSStatus ret = AudioHardwareSetProperty(kAudioHardwarePropertyHogModeIsAllowed, sizeof(val), &val);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioHardware::SetAutoHogMode: "
      "Unable to set auto 'hog' mode. Error = %s", GetError(ret).c_str());
}

AudioStreamBasicDescription* CCoreAudioHardware::FormatsList(AudioStreamID stream)
{
  // This is deprecated for kAudioStreamPropertyAvailablePhysicalFormats,
  // but compiling on 10.3 requires the older constant
  AudioDevicePropertyID p = kAudioStreamPropertyPhysicalFormats;

  UInt32 listSize;
  // Retrieve all the stream formats supported by this output stream
  OSStatus ret = AudioStreamGetPropertyInfo(stream, 0, p, &listSize, NULL);
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

  ret = AudioStreamGetProperty(stream, 0, p, &listSize, list);
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
  UInt32 listSize;
  OSStatus ret = AudioDeviceGetPropertyInfo(device, 0, FALSE,
    kAudioDevicePropertyStreams, &listSize, NULL);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: "
      "Unable to get list size. Error = %s", GetError(ret).c_str());
    return NULL;
  }

  // Space for a terminating ID:
  listSize += sizeof(AudioStreamID);
  AudioStreamID *list = (AudioStreamID*)malloc(listSize);
  if (list == NULL)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: Out of memory?");
    return NULL;
  }

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeInput;
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioDevicePropertyStreams;
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
  UInt32 size;
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  AudioDeviceID *devices = (AudioDeviceID*)malloc(size);
  if (!devices)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::ResetAudioDevices: ResetAudioDevices - out of memory?");
    return;
  }
  int numDevices = size / sizeof(AudioDeviceID);
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
  // Find the streams current physical format
  AudioStreamBasicDescription currentFormat;
  UInt32 paramSize = sizeof(currentFormat);
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
    {
      if (formats[i].mFormatID == kAudioFormatLinearPCM)
      {
        OSStatus ret = AudioStreamSetProperty(stream, NULL, 0, kAudioStreamPropertyPhysicalFormat, sizeof(formats[i]), &(formats[i]));
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
  UInt32 size = 0;
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  size_t deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, pDevices);
  if (ret)
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
    UInt32 totalChannels = device.GetTotalOutputChannels();
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice: "
#if __LP64__
      "Device[0x%04x]"
#else
      "Device[0x%04lx]"
#endif
      " - Name: '%s', Total Ouput Channels: %u. ",
      pDevices[dev], deviceName.c_str(), (uint)totalChannels);

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
  UInt32 size = sizeof(AudioDeviceID);
  AudioDeviceID deviceId = 0;
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  OSStatus ret = AudioObjectGetPropertyData(kAudioObjectSystemObject,
    &propertyAddress, 0, NULL, &size, &deviceId);
  // outputDevice is set to 0 if there is no audio device available
  // or if the default device is set to an encoded format
  if (ret || !deviceId) 
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetDefaultOutputDevice:"
      " Unable to identify default output device. Error = %s", GetError(ret).c_str());
    return 0;
  }

  return deviceId;
}

void CCoreAudioHardware::GetOutputDeviceName(std::string& name)
{
  AudioDeviceID deviceId = GetDefaultOutputDevice();

  if(deviceId)
  {
    UInt32 size = 0;
    // TODO: Change to kAudioObjectPropertyObjectName
    AudioDeviceGetPropertyInfo(deviceId,0, false,
      kAudioDevicePropertyDeviceName, &size, NULL);
    char *m_buffer = (char*)malloc(size);

    OSStatus ret = AudioDeviceGetProperty(deviceId, 0, false,
      kAudioDevicePropertyDeviceName, &size, m_buffer);
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

UInt32 CCoreAudioHardware::GetOutputDevices(CoreAudioDeviceList *pList)
{
  UInt32 found = 0;
  if (!pList)
    return found;

  // Obtain a list of all available audio devices
  UInt32 size = 0;
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  size_t deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioHardwarePropertyDevices;
  OSStatus ret = AudioObjectGetPropertyData(kAudioObjectSystemObject,
    &propertyAddress, 0, NULL, &size, pDevices);
  if (ret)
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
