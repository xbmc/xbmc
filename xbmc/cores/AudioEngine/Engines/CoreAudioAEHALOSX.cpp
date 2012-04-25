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
#include "utils/SystemInfo.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"

const char* g_ChannelLabels[] =
{
  "Unused", // kAudioChannelLabel_Unused
  "Left", // kAudioChannelLabel_Left
  "Right", // kAudioChannelLabel_Right
  "Center", // kAudioChannelLabel_Center
  "LFE",  // kAudioChannelLabel_LFEScreen
  "Side Left", // kAudioChannelLabel_LeftSurround
  "Side Right", // kAudioChannelLabel_RightSurround
  "Left Center", // kAudioChannelLabel_LeftCenter
  "Right Center", // kAudioChannelLabel_RightCenter
  "Back Center", // kAudioChannelLabel_CenterSurround
  "Back Left", // kAudioChannelLabel_LeftSurroundDirect
  "Back Right", // kAudioChannelLabel_RightSurroundDirect
  "Top Center", // kAudioChannelLabel_TopCenterSurround
  "Top Back Left", // kAudioChannelLabel_VerticalHeightLeft
  "Top Back Center", // kAudioChannelLabel_VerticalHeightCenter
  "Top Back Right", // kAudioChannelLabel_VerticalHeightRight
};

const AudioChannelLabel g_LabelMap[] =
{
  kAudioChannelLabel_Unused, // PCM_FRONT_LEFT,
  kAudioChannelLabel_Left, // PCM_FRONT_LEFT,
  kAudioChannelLabel_Right, //  PCM_FRONT_RIGHT,
  kAudioChannelLabel_Center, //  PCM_FRONT_CENTER,
  kAudioChannelLabel_LFEScreen, //  PCM_LOW_FREQUENCY,
  kAudioChannelLabel_LeftSurroundDirect, //  PCM_BACK_LEFT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_RightSurroundDirect, //  PCM_BACK_RIGHT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_LeftCenter, //  PCM_FRONT_LEFT_OF_CENTER,
  kAudioChannelLabel_RightCenter, //  PCM_FRONT_RIGHT_OF_CENTER,
  kAudioChannelLabel_CenterSurround, //  PCM_BACK_CENTER,
  kAudioChannelLabel_LeftSurround, //  PCM_SIDE_LEFT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_RightSurround, //  PCM_SIDE_RIGHT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_VerticalHeightLeft, //  PCM_TOP_FRONT_LEFT,
  kAudioChannelLabel_VerticalHeightRight, //  PCM_TOP_FRONT_RIGHT,
  kAudioChannelLabel_VerticalHeightCenter, //  PCM_TOP_FRONT_CENTER,
  kAudioChannelLabel_TopCenterSurround, //  PCM_TOP_CENTER,
  kAudioChannelLabel_TopBackLeft, //  PCM_TOP_BACK_LEFT,
  kAudioChannelLabel_TopBackRight, //  PCM_TOP_BACK_RIGHT,
  kAudioChannelLabel_TopBackCenter //  PCM_TOP_BACK_CENTER
};

const AudioChannelLayoutTag g_LayoutMap[] =
{
  kAudioChannelLayoutTag_Stereo, // PCM_LAYOUT_2_0 = 0,
  kAudioChannelLayoutTag_Stereo, // PCM_LAYOUT_2_0 = 0,
  kAudioChannelLayoutTag_DVD_4, // PCM_LAYOUT_2_1,
  kAudioChannelLayoutTag_MPEG_3_0_A, // PCM_LAYOUT_3_0,
  kAudioChannelLayoutTag_DVD_10, // PCM_LAYOUT_3_1,
  kAudioChannelLayoutTag_DVD_3, // PCM_LAYOUT_4_0,
  kAudioChannelLayoutTag_DVD_6, // PCM_LAYOUT_4_1,
  kAudioChannelLayoutTag_MPEG_5_0_A, // PCM_LAYOUT_5_0,
  kAudioChannelLayoutTag_MPEG_5_1_A, // PCM_LAYOUT_5_1,
  kAudioChannelLayoutTag_AudioUnit_7_0, // PCM_LAYOUT_7_0, ** This layout may be incorrect...no content to testß˚ **
  kAudioChannelLayoutTag_MPEG_7_1_A, // PCM_LAYOUT_7_1
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioHardware
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CCoreAudioHardware::GetAutoHogMode()
{
  UInt32 val = 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyHogModeIsAllowed, &size, &val);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetAutoHogMode: Unable to get auto 'hog' mode. Error = %s", GetError(ret).c_str());
    return false;
  }
  return (val == 1);
}

void CCoreAudioHardware::SetAutoHogMode(bool enable)
{
  UInt32 val = enable ? 1 : 0;
  OSStatus ret = AudioHardwareSetProperty(kAudioHardwarePropertyHogModeIsAllowed, sizeof(val), &val);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioHardware::SetAutoHogMode: Unable to set auto 'hog' mode. Error = %s", GetError(ret).c_str());
}

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
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FormatsList: Unable to get list size. Error = %s", GetError(ret).c_str());
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
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FormatsList: Unable to get list. Error = %s", GetError(ret).c_str());
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
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: Unable to get list size. Error = %s", GetError(ret).c_str());
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
    CLog::Log(LOGERROR, "CCoreAudioHardware::StreamsList: Unable to get list. Error = %s", GetError(ret).c_str());
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
    {
      if (formats[i].mFormatID == kAudioFormatLinearPCM)
      {
        ret = AudioStreamSetProperty(stream, NULL, 0, kAudioStreamPropertyPhysicalFormat, sizeof(formats[i]), &(formats[i]));
        if (ret != noErr)
        {
          CLog::Log(LOGDEBUG, "CCoreAudioHardware::ResetStream: Unable to retrieve the list of available devices. Error = %s", GetError(ret).c_str());
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

AudioDeviceID CCoreAudioHardware::FindAudioDevice(std::string searchName)
{
  if (!searchName.length())
    return 0;

  UInt32 size = 0;
  AudioDeviceID deviceId = 0;
  OSStatus ret;
  std::string searchNameLowerCase = searchName;

  std::transform( searchNameLowerCase.begin(), searchNameLowerCase.end(), searchNameLowerCase.begin(), ::tolower );
  if (searchNameLowerCase.compare("default") == 0)
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
    CLog::Log(LOGERROR, "CCoreAudioHardware::FindAudioDevice: Unable to retrieve the list of available devices. Error = %s", GetError(ret).c_str());
    delete[] pDevices;
    return 0;
  }

  // Attempt to locate the requested device
  std::string deviceName;
  for (UInt32 dev = 0; dev < deviceCount; dev++)
  {
    CCoreAudioDevice device;
    device.Open((pDevices[dev]));
    deviceName = device.GetName();
    UInt32 totalChannels = device.GetTotalOutputChannels();
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice:   Device[0x%04x] - Name: '%s', Total Ouput Channels: %u. ", pDevices[dev], deviceName.c_str(), totalChannels);

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

  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &deviceId);
  if (ret || !deviceId) // outputDevice is set to 0 if there is no audio device available, or if the default device is set to an encoded format
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetDefaultOutputDevice: Unable to identify default output device. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return deviceId;
}

void CCoreAudioHardware::GetOutputDeviceName(std::string& name)
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
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetOutputDevices: Unable to retrieve the list of available devices. Error = %s", GetError(ret).c_str());
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
  m_DeviceId            (0        ),
  m_Started             (false    ),
  m_HogPid              (-1       ),
  m_MixerRestore        (-1       ),
  m_IoProc              (NULL     ),
  m_ObjectListenerProc  (NULL     ),
  m_SampleRateRestore   (0.0f     ),
  m_frameSize           (0        ),
  m_OutputBufferIndex   (0        ),
  m_pSource             (NULL     )
{
}

CCoreAudioDevice::CCoreAudioDevice(AudioDeviceID deviceId) :
  m_DeviceId            (deviceId ),
  m_Started             (false    ),
  m_HogPid              (-1       ),
  m_MixerRestore        (-1       ),
  m_IoProc              (NULL     ),
  m_ObjectListenerProc  (NULL     ),
  m_SampleRateRestore   (0.0f     ),
  m_frameSize           (0        ),
  m_OutputBufferIndex   (0        ),
  m_pSource             (NULL     )
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

  // Unregister the IOProc if we have one
  if (m_IoProc)
    SetInputSource(NULL, 0, 0);

  SetHogStatus(false);
  if (m_MixerRestore > -1) // We changed the mixer status
    SetMixingSupport((m_MixerRestore ? true : false));
  m_MixerRestore = -1;

  if (m_SampleRateRestore != 0.0f)
  {
    CLog::Log(LOGDEBUG,  "CCoreAudioDevice::Close: Restoring original nominal samplerate.");
    SetNominalSampleRate(m_SampleRateRestore);
  }

  CLog::Log(LOGDEBUG, "CCoreAudioDevice::Close: Closed device 0x%04x", m_DeviceId);
  m_DeviceId = 0;
  m_IoProc = NULL;
  m_ObjectListenerProc = NULL;
  m_pSource = NULL;
}

void CCoreAudioDevice::Start()
{
  if (!m_DeviceId || m_Started)
    return;

  OSStatus ret = AudioDeviceStart(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Start: Unable to start device. Error = %s", GetError(ret).c_str());
  else
    m_Started = true;
}

void CCoreAudioDevice::Stop()
{
  if (!m_DeviceId || !m_Started)
    return;

  OSStatus ret = AudioDeviceStop(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Stop: Unable to stop device. Error = %s", GetError(ret).c_str());
  m_Started = false;
}

void CCoreAudioDevice::RemoveObjectListenerProc(AudioObjectPropertyListenerProc callback, void* pClientData)
{
  if (!m_DeviceId)
    return;

  AudioObjectPropertyAddress audioProperty;
  audioProperty.mSelector = kAudioObjectPropertySelectorWildcard;
  audioProperty.mScope = kAudioObjectPropertyScopeWildcard;
  audioProperty.mElement = kAudioObjectPropertyElementWildcard;

  OSStatus ret = AudioObjectRemovePropertyListener(m_DeviceId, &audioProperty, callback, pClientData);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::RemoveObjectListenerProc: Unable to set ObjectListener callback. Error = %s", GetError(ret).c_str());
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
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetObjectListenerProc: Unable to remove ObjectListener callback. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_ObjectListenerProc = callback;
  return true;
}

bool CCoreAudioDevice::SetInputSource(ICoreAudioSource* pSource, unsigned int frameSize, unsigned int outputBufferIndex)
{
  m_pSource             = pSource;
  m_frameSize           = frameSize;
  m_OutputBufferIndex   = outputBufferIndex;

  if (pSource)
    return AddIOProc();
  else
    return RemoveIOProc();
}

bool CCoreAudioDevice::AddIOProc()
{
  if (!m_DeviceId || m_IoProc) // Only one IOProc at a time
    return false;

  OSStatus ret = AudioDeviceCreateIOProcID(m_DeviceId, DirectRenderCallback, this, &m_IoProc);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::AddIOProc: Unable to add IOProc. Error = %s", GetError(ret).c_str());
    m_IoProc = NULL;
    return false;
  }

  Start();

  CLog::Log(LOGDEBUG, "CCoreAudioDevice::AddIOProc: IOProc 0x%08x set for device 0x%04x", m_IoProc, m_DeviceId);
  return true;
}

bool CCoreAudioDevice::RemoveIOProc()
{
  if (!m_DeviceId || !m_IoProc)
    return false;

  Stop();

  OSStatus ret = AudioDeviceDestroyIOProcID(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::RemoveIOProc: Unable to remove IOProc. Error = %s", GetError(ret).c_str());
  else
    CLog::Log(LOGDEBUG, "CCoreAudioDevice::RemoveIOProc: IOProc 0x%08x removed for device 0x%04x", m_IoProc, m_DeviceId);
  m_IoProc = NULL; // Clear the reference no matter what

  m_pSource = NULL;

  Sleep(100);

  return true;
}

std::string CCoreAudioDevice::GetName()
{
  std::string name;
  if (!m_DeviceId)
    return NULL;

  UInt32 size = 0;
  AudioDeviceGetPropertyInfo(m_DeviceId,0, false, kAudioDevicePropertyDeviceName, &size, NULL); // TODO: Change to kAudioObjectPropertyObjectName
  char *buff = new char[size];
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyDeviceName, &size, buff);
  name.assign(buff, size-1);
  delete [] buff;
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetName: Unable to get device name - id: 0x%04x. Error = %s", GetError(ret).c_str());
    return NULL;
  }
  return name;
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
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetTotalOutputChannels: Unable to get total device output channels - id: 0x%04x. Error = %s", m_DeviceId, GetError(ret).c_str());
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
    if (m_HogPid == -1) // Not already set
    {
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Setting 'hog' status on device 0x%04x", (unsigned int)m_DeviceId);
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(m_HogPid), &m_HogPid);
      if (ret || m_HogPid != getpid())
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to set 'hog' status. Error = %s", GetError(ret).c_str());
        return false;
      }
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: "
                "Successfully set 'hog' status on device 0x%04x", (unsigned int)m_DeviceId);
    }
  }
  else
  {
    if (m_HogPid > -1) // Currently Set
    {
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: "
                "Releasing 'hog' status on device 0x%04x", (unsigned int)m_DeviceId);
      pid_t hogPid = -1;
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(hogPid), &hogPid);
      if (ret || hogPid == getpid())
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to release 'hog' status. Error = %s", GetError(ret).c_str());
        return false;
      }
      m_HogPid = hogPid; // Reset internal state
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

bool CCoreAudioDevice::SetMixingSupport(UInt32 mix)
{
  if (!m_DeviceId)
    return false;

  if (!GetMixingSupport())
    return false;

  int restore = -1;
  if (m_MixerRestore == -1) // This is our first change to this setting. Store the original setting for restore
    restore = (GetMixingSupport() ? 1 : 0);
  UInt32 mixEnable = mix ? 1 : 0;
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetMixingSupport: "
            "%sabling mixing for device 0x%04x", mix ? "En" : "Dis", (unsigned int)m_DeviceId);
  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertySupportsMixing, sizeof(mixEnable), &mixEnable);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetMixingSupport: Unable to set MixingSupport to %s. Error = %s", mix ? "'On'" : "'Off'", GetError(ret).c_str());
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

  OSStatus  ret;
  UInt32    size;
  Boolean   writable = false;
  UInt32    mix = 0;

  size = sizeof(writable);
  ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, FALSE, kAudioDevicePropertySupportsMixing, &size, &writable);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SupportsMixing: Unable to get propertyinfo mixing support. Error = %s", GetError(ret).c_str());
    writable = false;
  }

  if (writable)
  {
    size = sizeof(mix);
    ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertySupportsMixing, &size, &mix);
    if (ret)
      mix = 0;
  }

  CLog::Log(LOGERROR, "CCoreAudioDevice::SupportsMixing: Device mixing support : %s.", mix ? "'Yes'" : "'No'");

  return (mix > 0);
}

bool CCoreAudioDevice::SetCurrentVolume(Float32 vol)
{
  if (!m_DeviceId)
    return false;

  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kHALOutputParam_Volume, sizeof(Float32), &vol);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetCurrentVolume: Unable to set AudioUnit volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioDevice::GetPreferredChannelLayout(CCoreAudioChannelLayout& layout)
{
  if (!m_DeviceId)
    return false;

  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false,
                                            kAudioDevicePropertyPreferredChannelLayout, &propertySize, &writable);
  if (ret)
    return false;

  void* pBuf = malloc(propertySize);
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false,
                               kAudioDevicePropertyPreferredChannelLayout, &propertySize, pBuf);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetPreferredChannelLayout: "
              "Unable to retrieve preferred channel layout. Error = %s", GetError(ret).c_str());
  else
    layout.CopyLayout(*((AudioChannelLayout*)pBuf)); // Copy the result into the caller's instance
  free(pBuf);
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
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetNominalSampleRate: Unable to retrieve current device sample rate. Error = %s", GetError(ret).c_str());
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
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetNominalSampleRate: Unable to set current device sample rate to %0.0f. Error = %s", (float)sampleRate, ret, GetError(ret).c_str());
    return false;
  }
  CLog::Log(LOGDEBUG,  "CCoreAudioDevice::SetNominalSampleRate: Changed device sample rate from %0.0f to %0.0f.", (float)currentRate, (float)sampleRate);
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

  return (num_latency_frames);
}

UInt32 CCoreAudioDevice::GetBufferSize()
{
  if (!m_DeviceId)
    return false;

  UInt32 size = 0;
  UInt32 propertySize = sizeof(size);
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false,
                                        kAudioDevicePropertyBufferFrameSize, &propertySize, &size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetBufferSize: Unable to retrieve buffer size. Error = %s", GetError(ret).c_str());
  return size;
}

bool CCoreAudioDevice::SetBufferSize(UInt32 size)
{
  if (!m_DeviceId)
    return false;

  UInt32 propertySize = sizeof(size);
  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false,
                                        kAudioDevicePropertyBufferFrameSize, propertySize, &size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetBufferSize: Unable to set buffer size. Error = %s", GetError(ret).c_str());

  if (GetBufferSize() != size)
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetBufferSize: Buffer size change not applied.");
  else
    CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetBufferSize: Set buffer size to %d", (int)size);

  return (ret == noErr);
}

OSStatus CCoreAudioDevice::DirectRenderCallback(AudioDeviceID inDevice,
                                                const AudioTimeStamp* inNow,
                                                const AudioBufferList* inInputData,
                                                const AudioTimeStamp* inInputTime,
                                                AudioBufferList* outOutputData,
                                                const AudioTimeStamp* inOutputTime,
                                                void* inClientData)
{
  OSStatus ret = noErr;
  CCoreAudioDevice *audioDevice = (CCoreAudioDevice*)inClientData;

  if (audioDevice->m_pSource && audioDevice->m_frameSize)
  {
    UInt32 frames = outOutputData->mBuffers[audioDevice->m_OutputBufferIndex].mDataByteSize / audioDevice->m_frameSize;
    ret = audioDevice->m_pSource->Render(NULL, inInputTime, 0, frames, outOutputData);
  }
  else
  {
    outOutputData->mBuffers[audioDevice->m_OutputBufferIndex].mDataByteSize = 0;
  }

  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioStream
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioStream::CCoreAudioStream() :
  m_StreamId  (0    )
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

  std::string formatString;

  // Revert any format changes we made
  if (m_OriginalVirtualFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Restoring original virtual format for stream 0x%04x. (%s)", m_StreamId, StreamDescriptionToString(m_OriginalVirtualFormat, formatString));
    AudioStreamBasicDescription setFormat = m_OriginalVirtualFormat;
    SetVirtualFormat(&setFormat);
  }
  if (m_OriginalPhysicalFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Restoring original physical format for stream 0x%04x. (%s)", m_StreamId, StreamDescriptionToString(m_OriginalPhysicalFormat, formatString));
    AudioStreamBasicDescription setFormat = m_OriginalPhysicalFormat;
    SetPhysicalFormat(&setFormat);
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

  return (num_latency_frames);
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

  std::string formatString;

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
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to set virtual format for stream 0x%04x. Error = %s", m_StreamId, GetError(ret).c_str());
    return false;
  }

  /* The AudioStreamSetProperty is not only asynchronious,
   * it is also not Atomic, in its behaviour.
   * Therefore we check 5 times before we really give up.
   * FIXME: failing isn't actually implemented yet. */
  for (int i = 0; i < 10; ++i)
  {
    AudioStreamBasicDescription checkVirtualFormat;
    if (!GetVirtualFormat(&checkVirtualFormat))
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to retrieve current physical format for stream 0x%04x.", m_StreamId);
      return false;
    }
    if (checkVirtualFormat.mSampleRate == pDesc->mSampleRate &&
        checkVirtualFormat.mFormatID == pDesc->mFormatID &&
        checkVirtualFormat.mFramesPerPacket == pDesc->mFramesPerPacket)
    {
      /* The right format is now active. */
      CLog::Log(LOGDEBUG, "CCoreAudioStream::SetVirtualFormat: Virtual format for stream 0x%04x. now active (%s)", m_StreamId, StreamDescriptionToString(checkVirtualFormat, formatString));
      break;
    }
    Sleep(100);
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

  std::string formatString;

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
    CLog::Log(LOGERROR, "CCoreAudioStream::SetPhysicalFormat: Unable to set physical format for stream 0x%04x. Error = %s", m_StreamId, GetError(ret).c_str());
    return false;
  }

  /* The AudioStreamSetProperty is not only asynchronious,
   * it is also not Atomic, in its behaviour.
   * Therefore we check 5 times before we really give up.
   * FIXME: failing isn't actually implemented yet. */
  for(int i = 0; i < 10; ++i)
  {
    AudioStreamBasicDescription checkPhysicalFormat;
    if (!GetPhysicalFormat(&checkPhysicalFormat))
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetPhysicalFormat: Unable to retrieve current physical format for stream 0x%04x.", m_StreamId);
      return false;
    }
    if (checkPhysicalFormat.mSampleRate == pDesc->mSampleRate &&
        checkPhysicalFormat.mFormatID == pDesc->mFormatID &&
        checkPhysicalFormat.mFramesPerPacket == pDesc->mFramesPerPacket)
    {
      /* The right format is now active. */
      CLog::Log(LOGDEBUG, "CCoreAudioStream::SetPhysicalFormat: Physical format for stream 0x%04x. now active (%s)", m_StreamId, StreamDescriptionToString(checkPhysicalFormat, formatString));
      break;
    }
    Sleep(100);
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
  m_Initialized     (false        ),
  m_pSource         (NULL         ),
  m_renderProc      (NULL         ),
  m_audioUnit       (NULL         ),
  m_audioNode       (NULL         ),
  m_audioGraph      (NULL         ),
  m_busNumber       (INVALID_BUS  )
{
}

CCoreAudioUnit::~CCoreAudioUnit()
{
  Close();
}

bool CCoreAudioUnit::Open(AUGraph audioGraph, ComponentDescription desc)
{
  if (m_audioUnit)
    Close();

  OSStatus ret;

  m_Initialized = false;

  ret = AUGraphNewNode(audioGraph, &desc, 0, NULL, &m_audioNode);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error add m_outputNode. Error = %s", GetError(ret).c_str());
    return false;
  }

  ret = AUGraphGetNodeInfo(audioGraph, m_audioNode, 0, 0, 0, &m_audioUnit);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error getting m_outputNode. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_audioGraph  = audioGraph;
  m_Initialized = true;

  return true;
}

bool CCoreAudioUnit::Open(AUGraph audioGraph, OSType type, OSType subType, OSType manufacturer)
{
  ComponentDescription desc;
  desc.componentType = type;
  desc.componentSubType = subType;
  desc.componentManufacturer = manufacturer;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  return Open(audioGraph, desc);
}

void CCoreAudioUnit::Close()
{
  if (!m_Initialized && !m_audioUnit)
    return;

  if (m_renderProc)
    SetInputSource(NULL);

  Stop();

  if (m_busNumber != INVALID_BUS)
  {
    OSStatus ret = AUGraphDisconnectNodeInput(m_audioGraph, m_audioNode, m_busNumber);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioUnit::Close: Unable to disconnect AudioUnit. Error = %s", GetError(ret).c_str());
    }

    ret = AUGraphRemoveNode(m_audioGraph, m_audioNode);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioUnit::Close: Unable to disconnect AudioUnit. Error = %s", GetError(ret).c_str());
    }
  }

  AUGraphUpdate(m_audioGraph, NULL);

  m_Initialized = false;
  m_audioUnit = NULL;
  m_audioNode = NULL;
  m_pSource = NULL;
}

bool CCoreAudioUnit::GetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus)
{
  if (!m_audioUnit || !pDesc)
    return false;

  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_StreamFormat, scope, bus, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetFormat: Unable to get AudioUnit format. Bus : %d Scope : %d : Error = %s", scope, bus, GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioUnit::SetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus)
{
  if (!m_audioUnit || !pDesc)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_StreamFormat, scope, bus, pDesc, sizeof(AudioStreamBasicDescription));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetFormat: Unable to set AudioUnit format. Bus : %d Scope : %d : Error = %s", scope, bus, GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioUnit::SetMaxFramesPerSlice(UInt32 maxFrames)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetMaxFramesPerSlice: Unable to set AudioUnit max frames per slice. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioUnit::GetSupportedChannelLayouts(AudioChannelLayoutList* pLayouts)
{
  if (!m_audioUnit)
    return false;
  if (!pLayouts)
    return false;

  UInt32 propSize = 0;
  Boolean writable = false;
  OSStatus ret = AudioUnitGetPropertyInfo(m_audioUnit, kAudioUnitProperty_SupportedChannelLayoutTags, kAudioUnitScope_Input, 0, &propSize, &writable);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetSupportedChannelLayouts: "
              "Unable to retrieve supported channel layout property info. Error = %s", GetError(ret).c_str());
    return false;
  }
  UInt32 layoutCount = propSize / sizeof(AudioChannelLayoutTag);
  AudioChannelLayoutTag* pSuppLayouts = new AudioChannelLayoutTag[layoutCount];
  ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_SupportedChannelLayoutTags, kAudioUnitScope_Output, 0, pSuppLayouts, &propSize);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetSupportedChannelLayouts: "
              "Unable to retrieve supported channel layouts. Error = %s", GetError(ret).c_str());
    return false;
  }
  for (UInt32 layout = 0; layout < layoutCount; layout++)
    pLayouts->push_back(pSuppLayouts[layout]);
  delete[] pSuppLayouts;
  return true;
}

bool CCoreAudioUnit::SetInputSource(ICoreAudioSource* pSource)
{
  m_pSource = pSource;
  if (pSource)
    return SetRenderProc();
  else
    return RemoveRenderProc();
}

bool CCoreAudioUnit::SetRenderProc()
{
  if (!m_audioUnit || m_renderProc)
    return false;

  AURenderCallbackStruct callbackInfo;
  callbackInfo.inputProc = RenderCallback; // Function to be called each time the AudioUnit needs data
  callbackInfo.inputProcRefCon = this; // Pointer to be returned in the callback proc
  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_SetRenderCallback,
                                      kAudioUnitScope_Input, 0, &callbackInfo, sizeof(AURenderCallbackStruct));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetRenderProc: Unable to set AudioUnit render callback. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_renderProc = RenderCallback;

  CLog::Log(LOGDEBUG, "CCoreAudioUnit::SetRenderProc: Set RenderProc 0x%08x for unit 0x%08x.", m_renderProc, m_audioUnit);

  return true;
}

bool CCoreAudioUnit::RemoveRenderProc()
{
  if (!m_audioUnit || !m_renderProc)
    return false;


  AURenderCallbackStruct callbackInfo;
  callbackInfo.inputProc = nil;
  callbackInfo.inputProcRefCon = nil;
  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_SetRenderCallback,
                                      kAudioUnitScope_Input, 0, &callbackInfo, sizeof(AURenderCallbackStruct));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::RemoveRenderProc: Unable to remove AudioUnit render callback. Error = %s", GetError(ret).c_str());
    return false;
  }

  CLog::Log(LOGDEBUG, "CCoreAudioUnit::RemoveRenderProc: Remove RenderProc 0x%08x for unit 0x%08x.", m_renderProc, m_audioUnit);

  m_renderProc = NULL;
  Sleep(100);

  return true;
}

OSStatus CCoreAudioUnit::RenderCallback(void *inRefCon,
                                        AudioUnitRenderActionFlags *ioActionFlags,
                                        const AudioTimeStamp *inTimeStamp,
                                        UInt32 inBusNumber,
                                        UInt32 inNumberFrames,
                                        AudioBufferList *ioData)
{
  OSStatus ret = noErr;
  CCoreAudioUnit *audioUnit = (CCoreAudioUnit*)inRefCon;

  if (audioUnit->m_pSource)
  {
    ret = audioUnit->m_pSource->Render(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
  }
  else
  {
    ioData->mBuffers[0].mDataByteSize = 0;
    if (ioActionFlags)
      *ioActionFlags |=  kAudioUnitRenderAction_OutputIsSilence;
  }


  return ret;
}

void CCoreAudioUnit::GetFormatDesc(AEAudioFormat format,
                                   AudioStreamBasicDescription *streamDesc,
                                   AudioStreamBasicDescription *coreaudioDesc)
{
  unsigned int bps = CAEUtil::DataFormatToBits(format.m_dataFormat);

  // Set the input stream format for the AudioUnit
  // We use the default DefaultOuput AudioUnit, so we only can set the input stream format.
  // The autput format is automaticaly set to the input format.
  streamDesc->mFormatID = kAudioFormatLinearPCM;            //  Data encoding format
  streamDesc->mFormatFlags = kLinearPCMFormatFlagIsPacked;
  switch (format.m_dataFormat)
  {
    case AE_FMT_FLOAT:
    case AE_FMT_LPCM:
      streamDesc->mFormatFlags |= kAudioFormatFlagsNativeEndian;
      streamDesc->mFormatFlags |= kAudioFormatFlagIsFloat;
      break;
    case AE_FMT_AC3:
    case AE_FMT_DTS:
    case AE_FMT_DTSHD:
    case AE_FMT_TRUEHD:
    case AE_FMT_EAC3:
      streamDesc->mFormatFlags |= kAudioFormatFlagsNativeEndian;
      streamDesc->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
    case AE_FMT_S16LE:
      streamDesc->mFormatFlags |= kAudioFormatFlagsNativeEndian;
      streamDesc->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
    case AE_FMT_S16BE:
      streamDesc->mFormatFlags |= kAudioFormatFlagIsBigEndian;
      streamDesc->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
    default:
      streamDesc->mFormatFlags |= kAudioFormatFlagsNativeEndian;
      streamDesc->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
  }
  streamDesc->mChannelsPerFrame = format.m_channelLayout.Count();               // Number of interleaved audiochannels
  streamDesc->mSampleRate = (Float64)format.m_sampleRate;                       //  the sample rate of the audio stream
  streamDesc->mBitsPerChannel = bps;                                            // Number of bits per sample, per channel
  streamDesc->mBytesPerFrame = (bps>>3) * format.m_channelLayout.Count();       // Size of a frame == 1 sample per channel
  streamDesc->mFramesPerPacket = 1;                                             // The smallest amount of indivisible data. Always 1 for uncompressed audio
  streamDesc->mBytesPerPacket = streamDesc->mBytesPerFrame * streamDesc->mFramesPerPacket;
  streamDesc->mReserved = 0;

  // Audio units use noninterleaved 32-bit floating point linear PCM data for input and output,
  // ...except in the case of an audio unit that is a data format converter, which converts to or from this format.
  coreaudioDesc->mFormatID = kAudioFormatLinearPCM;
  coreaudioDesc->mFormatFlags = kAudioFormatFlagsNativeEndian |
                                kAudioFormatFlagIsPacked |
                                kAudioFormatFlagIsNonInterleaved;
  switch (format.m_dataFormat)
  {
    case AE_FMT_FLOAT:
      coreaudioDesc->mFormatFlags |= kAudioFormatFlagIsFloat;
    default:
      coreaudioDesc->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
  }
  coreaudioDesc->mBitsPerChannel = bps; //sizeof(Float32)<<3;
  coreaudioDesc->mSampleRate = (Float64)format.m_sampleRate;;
  coreaudioDesc->mFramesPerPacket = 1;
  coreaudioDesc->mChannelsPerFrame = streamDesc->mChannelsPerFrame;
  coreaudioDesc->mBytesPerFrame = (bps>>3); //sizeof(Float32);
  coreaudioDesc->mBytesPerPacket = (bps>>3); //sizeof(Float32);

}

float CCoreAudioUnit::GetLatency()
{
  if (!m_audioUnit)
    return 0.0f;

  Float64 latency;
  UInt32 size = sizeof(latency);

  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0, &latency, &size);

  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetLatency: Unable to set AudioUnit latency. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }

  return latency;
}

bool CCoreAudioUnit::Stop()
{
  if (!m_audioUnit)
    return false;

  AudioOutputUnitStop(m_audioUnit);

  return true;
}

bool CCoreAudioUnit::Start()
{
  if (!m_audioUnit)
    return false;

  AudioOutputUnitStart(m_audioUnit);

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAUOutputDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAUOutputDevice::CAUOutputDevice() :
  m_DeviceId    (NULL )
{
}

CAUOutputDevice::~CAUOutputDevice()
{
}

bool CAUOutputDevice::SetCurrentDevice(AudioDeviceID deviceId)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret;

  ret = AudioUnitSetProperty(m_audioUnit,
                             kAudioOutputUnitProperty_CurrentDevice,
                             kAudioUnitScope_Global,
                             kOutputBus,
                             &deviceId,
                             sizeof(AudioDeviceID));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentDevice: Unable to set current device. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_DeviceId = deviceId;

  CLog::Log(LOGDEBUG, "CCoreAudioUnit::SetCurrentDevice: Current device 0x%08x", m_DeviceId);

  return true;
}

bool CAUOutputDevice::GetChannelMap(CoreAudioChannelList* pChannelMap)
{
  if (!m_audioUnit)
    return false;

  UInt32 size = 0;
  Boolean writable = false;
  AudioUnitGetPropertyInfo(m_audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, &size, &writable);
  UInt32 channels = size/sizeof(SInt32);
  SInt32* pMap = new SInt32[channels];
  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, &size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputChannelMap: Unable to retrieve AudioUnit input channel map. Error = %s", GetError(ret).c_str());
  else
    for (UInt32 i = 0; i < channels; i++)
      pChannelMap->push_back(pMap[i]);
  delete[] pMap;
  return (!ret);
}

bool CAUOutputDevice::SetChannelMap(CoreAudioChannelList* pChannelMap)
{
  // The number of array elements must match the number of output channels provided by the device
  if (!m_audioUnit || !pChannelMap)
    return false;
  UInt32 channels = pChannelMap->size();
  UInt32 size = sizeof(SInt32) * channels;
  SInt32* pMap = new SInt32[channels];
  for (UInt32 i = 0; i < channels; i++)
    pMap[i] = (*pChannelMap)[i];
  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: Unable to get current device's buffer size. Error = %s", GetError(ret).c_str());
  delete[] pMap;
  return (!ret);
}

Float32 CAUOutputDevice::GetCurrentVolume()
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 volPct = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit,  kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &volPct);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetCurrentVolume: Unable to get AudioUnit volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return volPct;
}

bool CAUOutputDevice::SetCurrentVolume(Float32 vol)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit, kHALOutputParam_Volume,
                                       kAudioUnitScope_Global, 0, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentVolume: Unable to set AudioUnit volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

UInt32 CAUOutputDevice::GetBufferFrameSize()
{
  if (!m_audioUnit)
    return 0;

  UInt32 size = sizeof(UInt32);
  UInt32 bufferSize = 0;

  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, &bufferSize, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: Unable to get current device's buffer size. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return bufferSize;
}

bool CAUOutputDevice::EnableInputOuput()
{
  if (!m_audioUnit)
    return false;

  OSStatus ret;
  UInt32 enable;
  UInt32 hasio;
  UInt32 size=sizeof(UInt32);

  ret = AudioUnitGetProperty(m_audioUnit,kAudioOutputUnitProperty_HasIO,kAudioUnitScope_Input, 1, &hasio, &size);

  if (hasio)
  {
    enable = 1;
    ret =  AudioUnitSetProperty(m_audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &enable, sizeof(enable));
    if (ret)
    {
      CLog::Log(LOGERROR, "CAUOutputDevice::EnableInputOuput:: Unable to enable input on bus 1. Error = %s", GetError(ret).c_str());
      return false;
    }

    enable = 1;
    ret = AudioUnitSetProperty(m_audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &enable, sizeof(enable));
    if (ret)
    {
      CLog::Log(LOGERROR, "CAUOutputDevice::EnableInputOuput:: Unable to disable output on bus 0. Error = %s", GetError(ret).c_str());
      return false;
    }
  }

  return true;
}

bool CAUOutputDevice::GetPreferredChannelLayout(CCoreAudioChannelLayout& layout)
{
  if (!m_DeviceId)
    return false;

  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false,
                                            kAudioDevicePropertyPreferredChannelLayout, &propertySize, &writable);
  if (ret)
    return false;

  void* pBuf = malloc(propertySize);
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false,
                               kAudioDevicePropertyPreferredChannelLayout, &propertySize, pBuf);
  if (ret)
    CLog::Log(LOGERROR, "CAUOutputDevice::GetPreferredChannelLayout: Unable to retrieve preferred channel layout. Error = %s", GetError(ret).c_str());
  else
    layout.CopyLayout(*((AudioChannelLayout*)pBuf)); // Copy the result into the caller's instance
  free(pBuf);
  return (ret == noErr);
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

bool CAUMatrixMixer::InitMatrixMixerVolumes()
{
  // Fetch thechannel configuration
  UInt32 dims[2];
  UInt32 size = sizeof(dims);
  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_MatrixDimensions, kAudioUnitScope_Global, 0, dims, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::Initialize:: Get matrix dimesion. Error = %s", GetError(ret).c_str());
    return false;
  }

  // Initialize global, input, and output levels
  if (!SetGlobalVolume(1.0f))
    return false;
  for (UInt32 i = 0; i < dims[0]; i++)
    if (!SetInputVolume(i, 1.0f))
      return false;
  for (UInt32 i = 0; i < dims[1]; i++)
    if (!SetOutputVolume(i, 1.0f))
      return false;

  return true;
}

UInt32 CAUMatrixMixer::GetInputBusCount()
{
  if (!m_audioUnit)
    return 0;

  UInt32 busCount = 0;
  UInt32 size = sizeof(busCount);
  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &busCount, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetInputBusCount: Unable to get input bus count. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return busCount;
}

bool CAUMatrixMixer::SetInputBusFormat(UInt32 busCount, AudioStreamBasicDescription *pFormat)
{
  if (!m_audioUnit)
    return false;

  UInt32 enable = 1;
  for (UInt32 i = 0; i < busCount; i++)
  {
    AudioUnitSetParameter(m_audioUnit, kMatrixMixerParam_Enable, kAudioUnitScope_Input, i, enable, 0);
    if (!SetFormat(pFormat, kAudioUnitScope_Input, i))
      return false;
  }

  return true;
}

bool CAUMatrixMixer::SetInputBusCount(UInt32 busCount)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &busCount, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetInputBusCount: Unable to set input bus count. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

UInt32 CAUMatrixMixer::GetOutputBusCount()
{
  if (!m_audioUnit)
    return 0;

  UInt32 busCount = 0;
  UInt32 size = sizeof(busCount);
  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Output, 0, &busCount, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetOutputBusCount: Unable to get output bus count. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return busCount;
}

bool CAUMatrixMixer::SetOutputBusCount(UInt32 busCount)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_BusCount, kAudioUnitScope_Output, 0, &busCount, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetOutputBusCount: Unable to set output bus count. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

Float32 CAUMatrixMixer::GetGlobalVolume()
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 vol = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit, kMatrixMixerParam_Volume, kAudioUnitScope_Global, 0xFFFFFFFF, &vol);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetGlobalVolume: Unable to get global volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return vol;
}

bool CAUMatrixMixer::SetGlobalVolume(Float32 vol)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit, kMatrixMixerParam_Volume, kAudioUnitScope_Global, 0xFFFFFFFF, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetGlobalVolume: Unable to set global volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

Float32 CAUMatrixMixer::GetInputVolume(UInt32 element)
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 vol = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit, kMatrixMixerParam_Volume, kAudioUnitScope_Input, element, &vol);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetInputVolume: Unable to get input volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return vol;
}

bool CAUMatrixMixer::SetInputVolume(UInt32 element, Float32 vol)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit, kMatrixMixerParam_Volume, kAudioUnitScope_Input, element, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetInputVolume: Unable to set input volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

Float32 CAUMatrixMixer::GetOutputVolume(UInt32 element)
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 vol = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit, kMatrixMixerParam_Volume, kAudioUnitScope_Output, element, &vol);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetOutputVolume: Unable to get output volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return vol;
}

bool CAUMatrixMixer::SetOutputVolume(UInt32 element, Float32 vol)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit, kMatrixMixerParam_Volume, kAudioUnitScope_Output, element, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetOutputVolume: Unable to set output volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioMixMap
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioMixMap::CCoreAudioMixMap() :
  m_isValid(false)
{
  m_pMap = (Float32*)calloc(sizeof(AudioChannelLayout), 1);
}

CCoreAudioMixMap::CCoreAudioMixMap(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout) :
  m_isValid(false)
{
  Rebuild(inLayout, outLayout);
}

CCoreAudioMixMap::~CCoreAudioMixMap()
{
  free(m_pMap);
  m_pMap = NULL;
}

void CCoreAudioMixMap::Rebuild(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout)
{
  // map[in][out] = mix-level of input_channel[in] into output_channel[out]

  free(m_pMap);
  m_pMap = NULL;

  m_inChannels = CCoreAudioChannelLayout::GetChannelCountForLayout(inLayout);
  m_outChannels = CCoreAudioChannelLayout::GetChannelCountForLayout(outLayout);

  // Try to find a 'well-known' matrix
  const AudioChannelLayout* layouts[] = {&inLayout, &outLayout};
  UInt32 propSize = 0;
  OSStatus ret = AudioFormatGetPropertyInfo(kAudioFormatProperty_MatrixMixMap, sizeof(layouts), layouts, &propSize);
  m_pMap = (Float32*)calloc(1,propSize);

  // Try and get a predefined mixmap
  ret = AudioFormatGetProperty(kAudioFormatProperty_MatrixMixMap, sizeof(layouts), layouts, &propSize, m_pMap);
  if (!ret)
  {
    m_isValid = true;
    return; // Nothing else to do...a map already exists
  }

  // No predefined mixmap was available. Going to have to build it manually
  CLog::Log(LOGDEBUG, "CCoreAudioMixMap::CreateMap: Unable to locate pre-defined mixing matrix");

  m_isValid = false;
}

CCoreAudioMixMap *CCoreAudioMixMap::CreateMixMap(CAUOutputDevice  *audioUnit, AEAudioFormat &format, AudioChannelLayoutTag layoutTag)
{
  if (!audioUnit)
    return NULL;

  AudioStreamBasicDescription inputFormat;
  AudioStreamBasicDescription fmt;

  // get the stream input format
  audioUnit->GetFormatDesc(format, &inputFormat, &fmt);

  unsigned int channels = format.m_channelLayout.Count();
  CAEChannelInfo channelLayout = format.m_channelLayout;
  bool hasLFE = false;
  // Convert XBMC input channel layout format to CoreAudio layout format
  AudioChannelLayout* pInLayout = (AudioChannelLayout*)malloc(sizeof(AudioChannelLayout) + sizeof(AudioChannelDescription) * channels);
  pInLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
  pInLayout->mChannelBitmap = 0;
  pInLayout->mNumberChannelDescriptions = channels;
  for (unsigned int chan=0; chan < channels; chan++)
  {
    AudioChannelDescription* pDesc = &pInLayout->mChannelDescriptions[chan];
    pDesc->mChannelLabel = g_LabelMap[(unsigned int)channelLayout[chan]]; // Convert from XBMC channel tag to CoreAudio channel tag
    pDesc->mChannelFlags = kAudioChannelFlags_AllOff;
    pDesc->mCoordinates[0] = 0.0f;
    pDesc->mCoordinates[1] = 0.0f;
    pDesc->mCoordinates[2] = 0.0f;
    if (pDesc->mChannelLabel == kAudioChannelLabel_LFEScreen)
      hasLFE = true;
  }
  // HACK: Fix broken channel layouts coming from some aac sources that include rear channel but no side channels.
  // 5.1 streams should include front and side channels. Rear channels are added by 6.1 and 7.1, so any 5.1
  // source that claims to have rear channels is wrong.
  if (inputFormat.mChannelsPerFrame == 6 && hasLFE) // Check for 5.1 configuration (as best we can without getting too silly)
  {
    for (unsigned int chan=0; chan < inputFormat.mChannelsPerFrame; chan++)
    {
      AudioChannelDescription* pDesc = &pInLayout->mChannelDescriptions[chan];
      if (pDesc->mChannelLabel == kAudioChannelLabel_LeftSurround || pDesc->mChannelLabel == kAudioChannelLabel_LeftSurround)
        break; // Required condition cannot be true

      if (pDesc->mChannelLabel == kAudioChannelLabel_LeftSurroundDirect)
      {
        pDesc->mChannelLabel = kAudioChannelLabel_LeftSurround; // Change [Back Left] to [Side Left]
        CLog::Log(LOGINFO, "CCoreAudioGraph::CreateMixMap: Detected faulty input channel map...fixing(Back Left-->Side Left)");
      }
      if (pDesc->mChannelLabel == kAudioChannelLabel_RightSurroundDirect)
      {
        pDesc->mChannelLabel = kAudioChannelLabel_RightSurround; // Change [Back Left] to [Side Left]
        CLog::Log(LOGINFO, "CCoreAudioGraph::CreateMixMap: Detected faulty input channel map...fixing(Back Right-->Side Right)");
      }
    }
  }

  CCoreAudioChannelLayout sourceLayout(*pInLayout);
  free(pInLayout);
  pInLayout = NULL;

  std::string strInLayout;
  CLog::Log(LOGINFO, "CCoreAudioGraph::CreateMixMap: Source Stream Layout: %s", CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)sourceLayout, strInLayout));

  // Get User-Configured (XBMC) Speaker Configuration
  AudioChannelLayout guiLayout;
  guiLayout.mChannelLayoutTag = layoutTag;
  CCoreAudioChannelLayout userLayout(guiLayout);
  std::string strUserLayout;
  CLog::Log(LOGINFO, "CCoreAudioGraph::CreateMixMap: User-Configured Speaker Layout: %s", CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)userLayout, strUserLayout));

  // Get OS-Configured (Audio MIDI Setup) Speaker Configuration (Channel Layout)
  CCoreAudioChannelLayout deviceLayout;
  if (!audioUnit->GetPreferredChannelLayout(deviceLayout))
    return NULL;

  // When all channels on the output device are unknown take the gui layout
  //if(deviceLayout.AllChannelUnknown())
  //  deviceLayout.CopyLayout(guiLayout);

  std::string strOutLayout;
  CLog::Log(LOGINFO, "CCoreAudioGraph::CreateMixMap: Output Device Layout: %s", CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)deviceLayout, strOutLayout));

  // TODO:
  // Reconcile the OS and GUI layout configurations. Clamp to the minimum number of speakers
  // For each OS-defined output, see if it exists in the GUI configuration
  // If it does, add it to the 'union' layout (bitmap?)
  // User may have configured 5.1 in GUI, but only 2.0 in OS
  // Resulting layout would be {FL, FR}
  // User may have configured 2.0 in GUI, and 5.1 in OS
  // Resulting layout would be {FL, FR}

  // Correct any configuration incompatibilities
  //if (CCoreAudioChannelLayout::GetChannelCountForLayout(guiLayout) < CCoreAudioChannelLayout::GetChannelCountForLayout(deviceLayout))
  //  deviceLayout.CopyLayout(guiLayout);

  // TODO: Skip matrix mixer if input/output are compatible

  AudioChannelLayout* layoutCandidates[] = {(AudioChannelLayout*)deviceLayout, (AudioChannelLayout*)userLayout, NULL};

  // Try to construct a mapping matrix for the mixer. Work through the layout candidates and see if any will work
  CCoreAudioMixMap *mixMap = new CCoreAudioMixMap();
  for (AudioChannelLayout** pLayout = layoutCandidates; *pLayout != NULL; pLayout++)
  {
    mixMap->Rebuild(*sourceLayout, **pLayout);
    if (mixMap->IsValid())
      break;
  }
  return mixMap;
}

bool CCoreAudioMixMap::SetMixingMatrix(CAUMatrixMixer  *mixerUnit, CCoreAudioMixMap *mixMap, AudioStreamBasicDescription *inputFormat, AudioStreamBasicDescription *fmt, int channelOffset)
{
  if (!mixerUnit || !inputFormat || !fmt)
    return false;

  OSStatus ret;
  // Configure the mixing matrix
  Float32* val = (Float32*)*mixMap;
  CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: Loading matrix mixer configuration");
  for (UInt32 i = 0; i < inputFormat->mChannelsPerFrame; ++i)
  {
    for (UInt32 j = 0; j < fmt->mChannelsPerFrame; ++j)
    {
      ret = AudioUnitSetParameter(mixerUnit->GetUnit(),
                                  kMatrixMixerParam_Volume, kAudioUnitScope_Global,
                                  ( (i + channelOffset) << 16 ) | j, *val++, 0);
      if (!ret)
      {
        CLog::Log(LOGINFO, "CCoreAudioGraph::Open: \t[%d][%d][%0.1f]",
                  (int)i + channelOffset, (int)j, *(val-1));
      }
    }
  }

  CLog::Log(LOGINFO, "CCoreAudioGraph::Open: "
            "Mixer Output Format: %d channels, %0.1f kHz, %d bits, %d bytes per frame",
            (int)fmt->mChannelsPerFrame, fmt->mSampleRate / 1000.0f, (int)fmt->mBitsPerChannel, (int)fmt->mBytesPerFrame);

  if (!mixerUnit->InitMatrixMixerVolumes())
    return false;

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioAEMixMap
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioChannelLayout::CCoreAudioChannelLayout() :
  m_pLayout(NULL)
{

}

CCoreAudioChannelLayout::CCoreAudioChannelLayout(AudioChannelLayout& layout) :
m_pLayout(NULL)
{
  CopyLayout(layout);
}

CCoreAudioChannelLayout::~CCoreAudioChannelLayout()
{
  free(m_pLayout);
}

bool CCoreAudioChannelLayout::CopyLayout(AudioChannelLayout& layout)
{
  free(m_pLayout);
  m_pLayout = NULL;

  // This method always produces a layout with a ChannelDescriptions structure

  OSStatus ret = 0;
  UInt32 channels = GetChannelCountForLayout(layout);
  UInt32 size = sizeof(AudioChannelLayout) + (channels - kVariableLengthArray) * sizeof(AudioChannelDescription);

  if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions) // We can copy the whole layout
  {
    m_pLayout = (AudioChannelLayout*)malloc(size);
    memcpy(m_pLayout, &layout, size);
  }
  else if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap) // Deconstruct the bitmap to get the layout
  {
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize);
    m_pLayout = (AudioChannelLayout*)malloc(propSize);
    ret = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize, m_pLayout);
    m_pLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
  }
  else // Convert the known layout to a custom layout
  {
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForTag, sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize);
    m_pLayout = (AudioChannelLayout*)malloc(propSize);
    ret = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForTag, sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize, m_pLayout);
    m_pLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
  }

  return (ret == noErr);
}

UInt32 CCoreAudioChannelLayout::GetChannelCountForLayout(AudioChannelLayout& layout)
{
  UInt32 channels = 0;
  if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap) // Channels are in fixed-order('USB Order'), any combination
  {
    UInt32 bitmap = layout.mChannelBitmap;
    for (UInt32 c = 0; c < (sizeof(layout.mChannelBitmap) << 3); c++)
    {
      if (bitmap & 0x1)
        channels++;
      bitmap >>= 1;
    }
  }
  else if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions) // Channels are in any order, any combination
    channels = layout.mNumberChannelDescriptions;
  else // Channels are in a predefined order and combination
    channels = AudioChannelLayoutTag_GetNumberOfChannels(layout.mChannelLayoutTag);

  return channels;
}

const char* CCoreAudioChannelLayout::ChannelLabelToString(UInt32 label)
{
  if (label > MAX_CHANNEL_LABEL)
    return "Unknown";
  return g_ChannelLabels[label];
}

const char* CCoreAudioChannelLayout::ChannelLayoutToString(AudioChannelLayout& layout, std::string& str)
{
  AudioChannelLayout* pLayout = NULL;

  if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
  {
    pLayout = &layout;
  }
  else if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap) // Deconstruct the bitmap to get the layout
  {
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize, pLayout);
  }
  else // Predefinied layout 'tag'
  {
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForTag, sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForTag, sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize, pLayout);
  }

  for (UInt32 c = 0; c < pLayout->mNumberChannelDescriptions; c++)
  {
    str += "[";
    str += ChannelLabelToString(pLayout->mChannelDescriptions[c].mChannelLabel);
    str += "] ";
  }

  if (layout.mChannelLayoutTag != kAudioChannelLayoutTag_UseChannelDescriptions)
    free(pLayout);

  return str.c_str();
}

bool CCoreAudioChannelLayout::AllChannelUnknown()
{
  AudioChannelLayout* pLayout = NULL;

  if (!m_pLayout)
    return false;

  if (m_pLayout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
  {
    pLayout = m_pLayout;
  }
  else if (m_pLayout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap) // Deconstruct the bitmap to get the layout
  {
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(m_pLayout->mChannelBitmap), &m_pLayout->mChannelBitmap, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(m_pLayout->mChannelBitmap), &m_pLayout->mChannelBitmap, &propSize, pLayout);
  }
  else // Predefinied layout 'tag'
  {
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForTag, sizeof(m_pLayout->mChannelLayoutTag), &m_pLayout->mChannelLayoutTag, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForTag, sizeof(m_pLayout->mChannelLayoutTag), &m_pLayout->mChannelLayoutTag, &propSize, pLayout);
  }

  for (UInt32 c = 0; c < pLayout->mNumberChannelDescriptions; c++)
  {
    if (pLayout->mChannelDescriptions[c].mChannelLabel != kAudioChannelLabel_Unknown)
    {
      return false;
    }
  }

  if (m_pLayout->mChannelLayoutTag != kAudioChannelLayoutTag_UseChannelDescriptions)
    free(pLayout);

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioGraph
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioGraph::CCoreAudioGraph() :
  m_audioGraph    (NULL ),
  m_audioUnit     (NULL ),
  m_mixerUnit     (NULL ),
  m_inputUnit     (NULL ),
  m_initialized   (false),
  m_deviceId      (NULL ),
  m_allowMixing   (false),
  m_mixMap        (NULL ),
  m_ATV1          (false)
{
  for (int i = 0; i < MAX_CONNECTION_LIMIT; i++)
  {
    m_reservedBusNumber[i] = false;
  }

  m_ATV1 = g_sysinfo.IsAppleTV();
}

CCoreAudioGraph::~CCoreAudioGraph()
{
  Close();

  delete m_mixMap;
}

bool CCoreAudioGraph::Open(ICoreAudioSource *pSource, AEAudioFormat &format, AudioDeviceID deviceId, bool allowMixing, AudioChannelLayoutTag layoutTag)
{
  OSStatus ret;

  AudioStreamBasicDescription inputFormat;
  AudioStreamBasicDescription outputFormat;
  AudioStreamBasicDescription fmt;

  m_allowMixing   = allowMixing;
  m_deviceId      = deviceId;

  ret = NewAUGraph(&m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error create audio grpah. Error = %s", GetError(ret).c_str());
    return false;
  }
  ret = AUGraphOpen(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error open audio grpah. Error = %s", GetError(ret).c_str());
    return false;
  }

  // get output unit
  if (m_audioUnit)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error audio unit already open. double call ?");
    return false;
  }

  m_audioUnit = new CAUOutputDevice();
  if (!m_audioUnit->Open(m_audioGraph, kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple))
    return false;

  m_audioUnit->GetFormatDesc(format, &inputFormat, &fmt);

  if (!m_audioUnit->EnableInputOuput())
    return false;

  if (!m_audioUnit->SetCurrentDevice(deviceId))
    return false;

  if (allowMixing)
  {
    delete m_mixMap;
    m_mixMap = CCoreAudioMixMap::CreateMixMap(m_audioUnit, format, layoutTag);

    if (m_mixMap || m_mixMap->IsValid())
    {
      // maximum input channel ber input bus
      //fmt.mChannelsPerFrame = MAXIMUM_MIXER_CHANNELS;

      // get output unit
      if (m_inputUnit)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error mixer unit already open. double call ?");
        return false;
      }

      m_inputUnit = new CAUOutputDevice();

      if (!m_inputUnit->Open(m_audioGraph, kAudioUnitType_FormatConverter, kAudioUnitSubType_AUConverter, kAudioUnitManufacturer_Apple))
        return false;

      if (!m_inputUnit->SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
        return false;

      if (!m_inputUnit->SetFormat(&fmt, kAudioUnitScope_Output, kOutputBus))
        return false;

      // get mixer unit
      if (m_mixerUnit)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error mixer unit already open. double call ?");
        return false;
      }

      m_mixerUnit = new CAUMatrixMixer();

      if (!m_mixerUnit->Open(m_audioGraph, kAudioUnitType_Mixer, kAudioUnitSubType_MatrixMixer, kAudioUnitManufacturer_Apple))
        return false;

      // set number of input buses
      if (!m_mixerUnit->SetInputBusCount(MAX_CONNECTION_LIMIT))
        return false;

      // set number of output buses
      if (!m_mixerUnit->SetOutputBusCount(1))
        return false;

      if (!m_mixerUnit->SetInputBusFormat(MAX_CONNECTION_LIMIT, &fmt))
        return false;

      if (!m_mixerUnit->SetFormat(&fmt, kAudioUnitScope_Output, kOutputBus))
        return false;

      ret =  AUGraphConnectNodeInput(m_audioGraph, m_mixerUnit->GetNode(), 0, m_audioUnit->GetNode(), 0);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error connecting m_m_mixerNode. Error = %s", GetError(ret).c_str());
        return false;
      }

      m_mixerUnit->SetBus(0);

      // configure output unit
      int busNumber = GetFreeBus();

      ret = AUGraphConnectNodeInput(m_audioGraph, m_inputUnit->GetNode(), 0, m_mixerUnit->GetNode(), busNumber);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error connecting m_converterNode. Error = %s", GetError(ret).c_str());
        return false;
      }

      m_inputUnit->SetBus(busNumber);

      ret = AUGraphUpdate(m_audioGraph, NULL);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error update graph. Error = %s", GetError(ret).c_str());
        return false;
      }
      ret = AUGraphInitialize(m_audioGraph);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error initialize graph. Error = %s", GetError(ret).c_str());
        return false;
      }

      // Update format structure to reflect the desired format from the mixer
      fmt.mChannelsPerFrame = m_mixMap->GetOutputChannels(); // The output format of the mixer is identical to the input format, except for the channel count

      UInt32 inputNumber = m_inputUnit->GetBus();
      int channelOffset = GetMixerChannelOffset(inputNumber);
      if (!CCoreAudioMixMap::SetMixingMatrix(m_mixerUnit, m_mixMap, &inputFormat, &fmt, channelOffset))
        return false;

      // Regenerate audio format and copy format for the Output AU
      outputFormat = fmt;
    }
    else
    {
      outputFormat = inputFormat;
    }

  }
  else
  {
    outputFormat = inputFormat;
  }

  if (!m_audioUnit->SetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error setting input format on audio device. Channel count %d, set it to %d",
              outputFormat.mChannelsPerFrame, format.m_channelLayout.Count());
    outputFormat.mChannelsPerFrame = format.m_channelLayout.Count();
    if (!m_audioUnit->SetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
      return false;
  }

  std::string formatString;
  // asume we are in dd-wave mode
  if (!m_ATV1 && !m_inputUnit)
  {
    if (!m_audioUnit->SetFormat(&inputFormat, kAudioUnitScope_Output, kInputBus))
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error setting Device Output Stream Format %s", StreamDescriptionToString(inputFormat, formatString));
    }
  }

#ifdef TAGRGET_IOS
  if (!m_audioUnit->SetFormat(&inputFormat, kAudioUnitScope_Output, kInputBus))
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error setting Device Output Stream Format %s", StreamDescriptionToString(inputFormat, formatString));
  }
#endif

  ret = AUGraphUpdate(m_audioGraph, NULL);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error update graph. Error = %s", GetError(ret).c_str());
    return false;
  }

  AudioStreamBasicDescription inputDesc_end, outputDesc_end;
  m_audioUnit->GetFormat(&inputDesc_end, kAudioUnitScope_Input, kOutputBus);
  m_audioUnit->GetFormat(&outputDesc_end, kAudioUnitScope_Output, kInputBus);
  CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Input Stream Format  %s", StreamDescriptionToString(inputDesc_end, formatString));
  CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Output Stream Format %s", StreamDescriptionToString(outputDesc_end, formatString));

  if (m_mixerUnit)
  {
    m_mixerUnit->GetFormat(&inputDesc_end, kAudioUnitScope_Input, kOutputBus);
    m_mixerUnit->GetFormat(&outputDesc_end, kAudioUnitScope_Output, kOutputBus);
    CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Input Stream Format  %s", StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Output Stream Format %s", StreamDescriptionToString(outputDesc_end, formatString));
  }

  if (m_inputUnit)
  {
    m_inputUnit->GetFormat(&inputDesc_end, kAudioUnitScope_Input, kOutputBus);
    m_inputUnit->GetFormat(&outputDesc_end, kAudioUnitScope_Output, kOutputBus);
    CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Input Stream Format  %s", StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Output Stream Format %s", StreamDescriptionToString(outputDesc_end, formatString));
  }

  ret = AUGraphInitialize(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error initialize graph. Error = %s", GetError(ret).c_str());
    return false;
  }

  UInt32 bufferFrames = m_audioUnit->GetBufferFrameSize();
  if (!m_audioUnit->SetMaxFramesPerSlice(bufferFrames))
    return false;

  SetInputSource(pSource);

  ShowGraph();

  return Start();
}

bool CCoreAudioGraph::Close()
{
  if (!m_audioGraph)
    return false;

  OSStatus ret;

  Stop();

  SetInputSource(NULL);

  while (!m_auUnitList.empty())
  {
    CAUOutputDevice *d = m_auUnitList.front();
    m_auUnitList.pop_front();
    ReleaseBus(d->GetBus());
    d->Close();
    delete d;
  }

  if (m_inputUnit)
  {
    ReleaseBus(m_inputUnit->GetBus());
    m_inputUnit->Close();
    delete m_inputUnit;
    m_inputUnit = NULL;
  }

  if (m_mixerUnit)
  {
    m_mixerUnit->Close();
    delete m_mixerUnit;
    m_mixerUnit = NULL;
  }

  if (m_audioUnit)
  {
    m_audioUnit->Close();
    delete m_audioUnit;
    m_audioUnit = NULL;
  }

  ret = AUGraphUninitialize(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Close: Error unitialize. Error = %s", GetError(ret).c_str());
  }

  ret = AUGraphClose(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Close: Error close. Error = %s", GetError(ret).c_str());
  }

  ret = DisposeAUGraph(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Close: Error dispose. Error = %s", GetError(ret).c_str());
  }

  return true;
}

bool CCoreAudioGraph::Start()
{
  if (!m_audioGraph)
    return false;

  OSStatus ret;
  Boolean isRunning = false;

  ret = AUGraphIsRunning(m_audioGraph, &isRunning);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Start: Audio graph not running. Error = %s", GetError(ret).c_str());
    return false;
  }
  if (!isRunning)
  {

    if (m_audioUnit)
      m_audioUnit->Start();
    if (m_mixerUnit)
      m_mixerUnit->Start();
    if (m_inputUnit)
      m_inputUnit->Start();

    ret = AUGraphStart(m_audioGraph);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Start: Error starting audio graph. Error = %s", GetError(ret).c_str());
    }
    ShowGraph();
  }

  return true;
}

bool CCoreAudioGraph::Stop()
{
  if (!m_audioGraph)
    return false;

  OSStatus ret;
  Boolean isRunning = false;

  ret = AUGraphIsRunning(m_audioGraph, &isRunning);
  if (ret)
  {

    if (m_inputUnit)
      m_inputUnit->Stop();
    if (m_mixerUnit)
      m_mixerUnit->Stop();
    if (m_audioUnit)
      m_audioUnit->Stop();

    CLog::Log(LOGERROR, "CCoreAudioGraph::Stop: Audio graph not running. Error = %s", GetError(ret).c_str());
    return false;
  }
  if (isRunning)
  {
    ret = AUGraphStop(m_audioGraph);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Stop: Error stopping audio graph. Error = %s", GetError(ret).c_str());
    }
  }

  return true;
}

AudioChannelLayoutTag CCoreAudioGraph::GetChannelLayoutTag(int layout)
{
  return g_LayoutMap[layout];
}

bool CCoreAudioGraph::SetInputSource(ICoreAudioSource* pSource)
{
  if (m_inputUnit)
    return m_inputUnit->SetInputSource(pSource);
  else if (m_audioUnit)
    return m_audioUnit->SetInputSource(pSource);

  return false;
}

bool CCoreAudioGraph::SetCurrentVolume(Float32 vol)
{
  if (!m_audioUnit)
    return false;

  return m_audioUnit->SetCurrentVolume(vol);
}

CAUOutputDevice *CCoreAudioGraph::DestroyUnit(CAUOutputDevice *outputUnit)
{
  if (!outputUnit)
    return NULL;

  Stop();

  for (AUUnitList::iterator itt = m_auUnitList.begin(); itt != m_auUnitList.end(); ++itt)
    if (*itt == outputUnit)
    {
      m_auUnitList.erase(itt);
      break;
    }

  ReleaseBus(outputUnit->GetBus());
  outputUnit->SetInputSource(NULL);
  outputUnit->Close();
  delete outputUnit;
  outputUnit = NULL;

  AUGraphUpdate(m_audioGraph, NULL);

  printf("Remove unit\n\n");
  ShowGraph();
  printf("\n");

  Start();

  return NULL;
}

CAUOutputDevice *CCoreAudioGraph::CreateUnit(AEAudioFormat &format)
{
  if (!m_audioUnit || !m_mixerUnit)
    return NULL;

  AudioStreamBasicDescription inputFormat;
  AudioStreamBasicDescription outputFormat;
  AudioStreamBasicDescription fmt;

  OSStatus ret;

  int busNumber = GetFreeBus();
  if (busNumber == INVALID_BUS)
    return  NULL;

  // create output unit
  CAUOutputDevice *outputUnit = new CAUOutputDevice();
  if (!outputUnit->Open(m_audioGraph, kAudioUnitType_FormatConverter, kAudioUnitSubType_AUConverter, kAudioUnitManufacturer_Apple))
    goto error;

  m_audioUnit->GetFormatDesc(format, &inputFormat, &fmt);

  // get the format frm the mixer
  if (!m_mixerUnit->GetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
    goto error;

  if (!outputUnit->SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
    goto error;

  if (!outputUnit->SetFormat(&outputFormat, kAudioUnitScope_Output, kOutputBus))
    goto error;

  ret = AUGraphConnectNodeInput(m_audioGraph, outputUnit->GetNode(), 0, m_mixerUnit->GetNode(), busNumber);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::CreateUnit: Error connecting outputUnit. Error = %s", GetError(ret).c_str());
    goto error;
  }

  // TODO: setup mixmap, get free bus number for connection

  outputUnit->SetBus(busNumber);

  if (m_mixMap || m_mixMap->IsValid())
  {
    UInt32 inputNumber = outputUnit->GetBus();
    int channelOffset = GetMixerChannelOffset(inputNumber);
    CCoreAudioMixMap::SetMixingMatrix(m_mixerUnit, m_mixMap, &inputFormat, &fmt, channelOffset);
  }


  AUGraphUpdate(m_audioGraph, NULL);

  printf("Add unit\n\n");
  ShowGraph();
  printf("\n");

  m_auUnitList.push_back(outputUnit);

  return outputUnit;

error:
  delete outputUnit;
  return NULL;
}

int CCoreAudioGraph::GetFreeBus()
{
  for (int i = 0; i < MAX_CONNECTION_LIMIT; i++)
  {
    if (!m_reservedBusNumber[i])
    {
      m_reservedBusNumber[i] = true;
      return i;
    }
  }
  return INVALID_BUS;
}

void CCoreAudioGraph::ReleaseBus(int busNumber)
{
  if (busNumber > MAX_CONNECTION_LIMIT || busNumber < 0)
    return;

  m_reservedBusNumber[busNumber] = false;
}

bool CCoreAudioGraph::IsBusFree(int busNumber)
{
  if (busNumber > MAX_CONNECTION_LIMIT || busNumber < 0)
    return false;
  return m_reservedBusNumber[busNumber];
}

int CCoreAudioGraph::GetMixerChannelOffset(int busNumber)
{
  if (!m_mixerUnit)
    return 0;

  int offset = 0;
  AudioStreamBasicDescription fmt;

  for (int i = 0; i < busNumber; i++)
  {
    memset(&fmt, 0x0, sizeof(fmt));
    m_mixerUnit->GetFormat(&fmt, kAudioUnitScope_Input, busNumber);
    offset += fmt.mChannelsPerFrame;
  }
  return offset;
}

void CCoreAudioGraph::ShowGraph()
{
  CAShow(m_audioGraph);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioAEHALOSX
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCoreAudioAEHALOSX::CCoreAudioAEHALOSX() :
  m_Initialized       (false  ),
  m_Passthrough       (false  ),
  m_NumLatencyFrames  (0      ),
  m_OutputBufferIndex (0      ),
  m_allowMixing       (false  ),
  m_encoded           (false  ),
  m_audioGraph        (NULL   )
{
  m_AudioDevice   = new CCoreAudioDevice();
  m_OutputStream  = new CCoreAudioStream();

#if defined(__APPLE__) && !defined(__arm__)
  SInt32 major,  minor;
  Gestalt(gestaltSystemVersionMajor, &major);
  Gestalt(gestaltSystemVersionMinor, &minor);

  // By default, kAudioHardwarePropertyRunLoop points at the process's main thread on SnowLeopard,
  // If your process lacks such a run loop, you can set kAudioHardwarePropertyRunLoop to NULL which
  // tells the HAL to run it's own thread for notifications (which was the default prior to SnowLeopard).
  // So tell the HAL to use its own thread for similar behavior under all supported versions of OSX.
  if (major == 10 && minor >=6)
  {
    CFRunLoopRef theRunLoop = NULL;
    AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    OSStatus theError = AudioObjectSetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
    if (theError != noErr)
    {
      CLog::Log(LOGERROR, "CCoreAudioAE::constructor: kAudioHardwarePropertyRunLoop error.");
    }
  }
#endif
}

CCoreAudioAEHALOSX::~CCoreAudioAEHALOSX()
{
  Deinitialize();

  delete m_audioGraph;
  delete m_AudioDevice;
  delete m_OutputStream;
}

bool CCoreAudioAEHALOSX::InitializePCM(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing, AudioDeviceID outputDevice)
{

  if (m_audioGraph)
  {
    m_audioGraph->Close();
    delete m_audioGraph;
  }
  m_audioGraph = new CCoreAudioGraph();

  if (!m_audioGraph)
    return false;

  if (!m_audioGraph->Open(pSource, format, outputDevice, allowMixing, g_LayoutMap[ g_guiSettings.GetInt("audiooutput.channellayout") ] ))
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::Initialize: Unable to initialize audio due a missconfiguration. Try 2.0 speaker configuration.");
    return false;
  }

  m_NumLatencyFrames = m_AudioDevice->GetNumLatencyFrames();

  m_allowMixing = allowMixing;

  return true;
}

bool CCoreAudioAEHALOSX::InitializePCMEncoded(ICoreAudioSource *pSource, AEAudioFormat &format, AudioDeviceID outputDevice)
{
  m_AudioDevice->SetHogStatus(true); // Prevent any other application from using this device.
  m_AudioDevice->SetMixingSupport(false); // Try to disable mixing support. Effectiveness depends on the device.

  // Set the Sample Rate as defined by the spec.
  m_AudioDevice->SetNominalSampleRate((float)format.m_sampleRate);

  if (!InitializePCM(pSource, format, false, outputDevice))
    return false;

  return true;
}

bool CCoreAudioAEHALOSX::InitializeEncoded(AudioDeviceID outputDevice, AEAudioFormat &format)
{
  std::string formatString;
  AudioStreamBasicDescription outputFormat = {0};
  AudioStreamID outputStream = 0;

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

      if (m_rawDataFormat == AE_FMT_LPCM || m_rawDataFormat == AE_FMT_DTSHD ||
         m_rawDataFormat == AE_FMT_TRUEHD || m_rawDataFormat == AE_FMT_EAC3)
      {
        unsigned int bps = CAEUtil::DataFormatToBits(AE_FMT_S16NE);
        if (desc.mFormat.mChannelsPerFrame == m_initformat.m_channelLayout.Count() && desc.mFormat.mBitsPerChannel == bps &&
            desc.mFormat.mSampleRate == m_initformat.m_sampleRate )
        {
          outputFormat = desc.mFormat; // Select this format
          m_OutputBufferIndex = streamIndex;
          outputStream = stream.GetId();
          break;
        }
      }
      else
      {
        if (desc.mFormat.mFormatID == kAudioFormat60958AC3 || desc.mFormat.mFormatID == 'IAC3')
        {
          outputFormat = desc.mFormat; // Select this format
          m_OutputBufferIndex = streamIndex;
          outputStream = stream.GetId();
          break;
        }
      }
      physicalFormats.pop_front();
    }

    // TODO: How do we determine if this is the right stream (not just the right format) to use?
    if (outputFormat.mFormatID)
      break; // We found a suitable format. No need to continue.
    streamIndex++;
  }

  if (!outputFormat.mFormatID) // No match found
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Unable to identify suitable output format.");
    return false;
  }

  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Selected stream[%lu] - id: 0x%04lX, Physical Format: %s", m_OutputBufferIndex, outputStream, StreamDescriptionToString(outputFormat, formatString));

  // TODO: Auto hogging sets this for us. Figure out how/when to turn it off or use it
  // It appears that leaving this set will aslo restore the previous stream format when the
  // Application exits. If auto hogging is set and we try to set hog mode, we will deadlock
  // From the SDK docs: "If the AudioDevice is in a non-mixable mode, the HAL will automatically take hog mode on behalf of the first process to start an IOProc."

  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)

  CCoreAudioHardware::SetAutoHogMode(false); // Auto-Hog does not always un-hog the device when changing back to a mixable mode. Handle this on our own until it is fixed.
  bool autoHog = CCoreAudioHardware::GetAutoHogMode();
  CLog::Log(LOGDEBUG, " CoreAudioRenderer::InitializeEncoded: Auto 'hog' mode is set to '%s'.", autoHog ? "On" : "Off");
  if (!autoHog) // Try to handle this ourselves
  {
    m_AudioDevice->SetHogStatus(true); // Hog the device if it is not set to be done automatically
    m_AudioDevice->SetMixingSupport(false); // Try to disable mixing. If we cannot, it may not be a problem
  }

  m_NumLatencyFrames = m_AudioDevice->GetNumLatencyFrames();

  // Configure the output stream object
  m_OutputStream->Open(outputStream); // This is the one we will keep

  AudioStreamBasicDescription virtualFormat;
  m_OutputStream->GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Previous Virtual Format: %s", StreamDescriptionToString(virtualFormat, formatString));

  AudioStreamBasicDescription previousPhysicalFormat;
  m_OutputStream->GetPhysicalFormat(&previousPhysicalFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: Previous Physical Format: %s", StreamDescriptionToString(previousPhysicalFormat, formatString));

  m_OutputStream->SetPhysicalFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
  m_NumLatencyFrames += m_OutputStream->GetNumLatencyFrames();

  m_OutputStream->GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: New Virtual Format: %s", StreamDescriptionToString(virtualFormat, formatString));
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: New Physical Format: %s", StreamDescriptionToString(outputFormat, formatString));

  m_allowMixing = false;

  return true;
}

bool CCoreAudioAEHALOSX::Initialize(ICoreAudioSource *ae, bool passThrough, AEAudioFormat &format, AEDataFormat rawDataFormat, std::string &device)
{
  // Reset all the devices to a default 'non-hog' and mixable format.
  // If we don't do this we may be unable to find the Default Output device.
  // (e.g. if we crashed last time leaving it stuck in AC-3 mode)

  CCoreAudioHardware::ResetAudioDevices();

  m_ae = (CCoreAudioAE *)ae;

  if (!m_ae)
    return false;

  m_initformat          = format;
  m_rawDataFormat       = rawDataFormat;
  m_Passthrough         = passThrough;
  m_encoded             = false;
  m_OutputBufferIndex   = 0;

  if (format.m_channelLayout.Count() == 0)
  {
    CLog::Log(LOGERROR, "CCoreAudioAEHALOSX::Initialize - Unable to open the requested channel layout");
    return false;
  }

  if (device.find("CoreAudio:") != std::string::npos)
    device.erase(0, strlen("CoreAudio:"));

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
  if (m_Passthrough)
  {
    m_encoded = InitializeEncoded(outputDevice, format);
  }

  // If this is a PCM stream, or we failed to handle a passthrough stream natively,
  // prepare the standard interleaved PCM interface
  if (!m_encoded)
  {
    // If we are here and this is a passthrough stream, native handling failed.
    // Try to handle it as IEC61937 data over straight PCM (DD-Wav)
    bool configured = false;
    if (m_Passthrough)
    {
      CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::Initialize: No suitable AC3 output format found. Attempting DD-Wav.");
      configured = InitializePCMEncoded(ae, format, outputDevice);
    }
    else
    {
      // Standard PCM data
      configured = InitializePCM(ae, format, true, outputDevice);
    }

    if (!configured) // No suitable output format was able to be configured
      return false;
  }

  if (m_audioGraph)
    m_audioGraph->ShowGraph();

  m_Initialized = true;

  return true;
}

CAUOutputDevice *CCoreAudioAEHALOSX::DestroyUnit(CAUOutputDevice *outputUnit)
{
  if (m_audioGraph && outputUnit)
    return m_audioGraph->DestroyUnit(outputUnit);

  return NULL;
}

CAUOutputDevice *CCoreAudioAEHALOSX::CreateUnit(ICoreAudioSource *pSource, AEAudioFormat &format)
{
  CAUOutputDevice *outputUnit = NULL;

  // when HAL is using a mixer, the input is routed through converter units.
  // therefore we create a converter unit attach the source and give it back.
  if (m_allowMixing && m_audioGraph)
  {
    outputUnit = m_audioGraph->CreateUnit(format);

    if (pSource && outputUnit)
      outputUnit->SetInputSource(pSource);
  }

  return outputUnit;
}

void CCoreAudioAEHALOSX::Deinitialize()
{
  if (!m_Initialized)
    return;

  Stop();

  //if (m_encoded)

  if (m_encoded)
    m_AudioDevice->SetInputSource(NULL, 0, 0);

  if (m_audioGraph)
    m_audioGraph->SetInputSource(NULL);

  m_OutputStream->Close();
  m_AudioDevice->Close();

  if (m_audioGraph)
  {
    //m_audioGraph->Close();
    delete m_audioGraph;
  }
  m_audioGraph = NULL;

  m_NumLatencyFrames = 0;
  m_OutputBufferIndex = 0;

  m_Initialized = false;
  m_Passthrough = false;

  CLog::Log(LOGINFO, "CCoreAudioAEHALOSX::Deinitialize: Audio device has been closed.");
}

void CCoreAudioAEHALOSX::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  CoreAudioDeviceList deviceList;
  CCoreAudioHardware::GetOutputDevices(&deviceList);

  std::string defaultDeviceName;
  CCoreAudioHardware::GetOutputDeviceName(defaultDeviceName);

  std::string deviceName;
  for (int i = 0; !deviceList.empty(); i++)
  {
    CCoreAudioDevice device(deviceList.front());
    deviceName = device.GetName();

    std::string deviceName_Internal = std::string("CoreAudio:");
    deviceName_Internal.append(deviceName);
    devices.push_back(AEDevice(deviceName, deviceName_Internal));

    deviceList.pop_front();
  }
}

void CCoreAudioAEHALOSX::Stop()
{
  if (!m_Initialized)
    return;

  if (m_encoded)
    m_AudioDevice->Stop();
  else
    m_audioGraph->Stop();
}

bool CCoreAudioAEHALOSX::Start()
{
  if (!m_Initialized)
    return false;

  if (m_encoded)
    m_AudioDevice->Start();
  else
    m_audioGraph->Start();

  return true;
}

void CCoreAudioAEHALOSX::SetDirectInput(ICoreAudioSource *pSource, AEAudioFormat &format)
{
  if (!m_Initialized)
    return;

  // when HAL is initialized encoded we use directIO
  // when HAL is not in encoded mode and there is no mixer attach source the audio unit
  // when mixing is allowed in HAL, HAL is working with converter units where we attach the source.

  if (m_encoded)
  {
    // register directcallback for the audio HAL
    // direct render callback need to know the framesize and buffer index
    if (pSource)
    {
      m_AudioDevice->SetInputSource(pSource, format.m_frameSize, m_OutputBufferIndex);
    }
    else
    {
      m_AudioDevice->SetInputSource(pSource, 0, 0);
    }
  }
  else if (!m_encoded && !m_allowMixing)
  {
    // register render callback for the audio unit
    m_audioGraph->SetInputSource(pSource);
  }

  if (m_audioGraph)
    m_audioGraph->ShowGraph();

}

double CCoreAudioAEHALOSX::GetDelay()
{
  double delay;

  delay = (double)(m_NumLatencyFrames) / (m_initformat.m_sampleRate);

  return delay;
}

void  CCoreAudioAEHALOSX::SetVolume(float volume)
{
  if (m_encoded || m_Passthrough)
    return;

  m_audioGraph->SetCurrentVolume(volume);
}

unsigned int CCoreAudioAEHALOSX::GetBufferIndex()
{
  return m_OutputBufferIndex;
}

#endif
