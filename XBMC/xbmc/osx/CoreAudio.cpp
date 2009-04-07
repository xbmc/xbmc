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

#include "stdafx.h"
#include "CoreAudio.h"

void UInt32ToFourCC(char* fourCC, Uint32 val) // fourCC must be at least 5 BYTES wide
{
  char* pFormatId = (char*)&val;
  fourCC[3] = pFormatId[0];
  fourCC[2] = pFormatId[1];
  fourCC[1] = pFormatId[2];
  fourCC[0] = pFormatId[3];
  fourCC[4] = '\0'; // NULL terminate
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioHardware
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceID CCoreAudioHardware::FindAudioDevice(CStdString deviceName)
{
  if (!deviceName.length())
    return 0;
  
  UInt32 size = 0;
  AudioDeviceID deviceId = 0;
  OSStatus ret;
  
  CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice: Searching for device - %s.", deviceName.c_str());
  
  // Obtain a list of all available audio devices
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  UInt32 deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  ret = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, pDevices);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioHardware::FindAudioDevice: Unable to retrieve the list of available devices. ErrCode = 0x%08x", ret);
    delete[] pDevices;
    return 0; 
  }
  
  // Attempt to locate the requested device
  // TODO: Improve new/delete performance
  for (UInt32 dev = 0; dev < deviceCount; dev++)
  {
    AudioDeviceGetPropertyInfo(pDevices[dev],0, false, kAudioDevicePropertyDeviceName, &size, NULL); // TODO: Change to kAudioObjectPropertyName
    char* pName = new char[size];
    AudioDeviceGetProperty(pDevices[dev],0, false, kAudioDevicePropertyDeviceName, &size, pName);
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice:   Found device - %s.", pName);
    if (deviceName.Equals(pName))
      deviceId = pDevices[dev];
    delete[] pName;
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
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetDefaultOutputDevice: Unable to identify default output device.");
    return 0;
  }
  return deviceId;
}

bool CCoreAudioHardware::GetAutoHogMode()
{
  UInt32 val = 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyHogModeIsAllowed, &size, &val);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetAutoHogMode: Unable to get auto 'hog' mode.");
    return false;
  }
  return (val == 1);
}

void CCoreAudioHardware::SetAutoHogMode(bool enable)
{
  UInt32 val = enable ? 1 : 0;
  OSStatus ret = AudioHardwareSetProperty(kAudioHardwarePropertyHogModeIsAllowed, sizeof(val), &val);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioHardware::SetAutoHogMode: Unable to set auto 'hog' mode.");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioDevice::CCoreAudioDevice()  : 
  m_DeviceId(0),
  m_Hog(-1),
  m_MixerRestore(-1),
  m_IoProc(NULL)
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
  
  Stop();
  SetHogStatus(false);
  if (m_MixerRestore > -1) // We changed the mixer status
    SetMixingSupport((m_MixerRestore ? true : false));
  m_MixerRestore = -1;

  CLog::Log(LOGDEBUG, "CCoreAudioDevice::Close: Closed device 0x%04x", m_DeviceId);
  m_DeviceId = 0;
  m_IoProc = NULL;
  
}

void CCoreAudioDevice::Start(AudioDeviceIOProc ioProc)
{
  if (!m_DeviceId || m_IoProc) // Only one IOProc at a time
    return;
  
  OSStatus ret = AudioDeviceStart(m_DeviceId, ioProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Start: Unable to start device. Error = 0x%08x (%4.4s)", ret, (char*)&ret);
  m_IoProc = ioProc;
}

void CCoreAudioDevice::Stop()
{
  if (!m_DeviceId)
    return;
  
  OSStatus ret = AudioDeviceStop(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Stop: Unable to stop device. Error = 0x%08x (%4.4s)", ret, (char*)&ret);
  m_IoProc = NULL;
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

bool CCoreAudioDevice::SetHogStatus(bool hog)
{
  if (!m_DeviceId)
    return false;
  
  if (hog)
  {
    if (m_Hog == -1) // Not already set
    {
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Setting 'hog' status on device 0x%04x", m_DeviceId);
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(pid_t), &m_Hog);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to set 'hog' status. Error = 0x%08x (%4.4s)", ret, (char*)&ret);
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
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(pid_t), &hogPid);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to release 'hog' status. Error = 0x%08x (%4.4s)", ret, (char*)&ret);
        return false;
      }
      m_Hog = hogPid; // Reset internal state
    }
  }
  return true;
}

bool CCoreAudioDevice::GetHogStatus()
{
  // TODO: Should we confirm with CoreAudio?
  return (m_Hog > -1);
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
  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertySupportsMixing, sizeof(UInt32), &mixEnable);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetMixingSupport: Unable to set MixingSupport to %s. Error = 0x%08x (%4.4s)", mix ? "'On'" : "'Off'", ret, (char*)&ret);
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
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertySupportsMixing, &size, &val);
  if (ret)
    return false;
  return (val > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioStream
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioStream::CCoreAudioStream()
{
  m_OriginalVirtualFormat.mFormatID = 0;
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

UInt32 CCoreAudioStream::GetLatency()
{
  if (!m_StreamId)
    return 0;  
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyLatency, &size, &val);
  if (ret)
    return 0;
  return val;  
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
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to set virtual format for stream 0x%04x. Error = 0x%08x (%4.4s)", m_StreamId, ret, (char*)&ret);
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
  OSStatus ret = AudioStreamSetProperty(m_StreamId, NULL, 0, kAudioStreamPropertyPhysicalFormat, sizeof(AudioStreamBasicDescription), pDesc);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to set physical format for stream 0x%04x. Error = 0x%08x (%4.4s)", m_StreamId, ret, (char*)&ret);
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
CCoreAudioUnit::CCoreAudioUnit()
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::Open: Unable to open AudioUnit Component. ErrCode: 0x%08x", ret);
    return false; 
  }
  return true;
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::Initialize: Unable to Initialize AudioUnit. ErrCode = 0x%08x", ret);
    return false; 
  } 
  m_Initialized = true;
  return true;
}

bool CCoreAudioUnit::SetCurrentDevice(AudioDeviceID deviceId)
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &deviceId, sizeof(AudioDeviceID));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentDevice: Unable to set current device. Error = 0x%08x (%4.4s)", ret,(char*)&ret);
    return false; 
  }  
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputFormat: Unable to get AudioUnit input format. ErrCode = 0x%08x", ret);
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputFormat: Unable to get AudioUnit output format. ErrCode = 0x%08x", ret);
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetInputFormat: Unable to set AudioUnit input format. ErrCode = 0x%08x", ret);
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetInputFormat: Unable to set AudioUnit output format. ErrCode = 0x%08x", ret);
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetRenderProc: Unable to set AudioUnit render callback. ErrCode = 0x%08x", ret);
    return false;
  }
  return true;
}

UInt32 CCoreAudioUnit::GetBufferFrameSize()
{
  if (!m_Component)
    return 0;
  
  UInt32 size = sizeof(UInt32);
  UInt32 bufferSize = 0;
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, &bufferSize, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: Unable to get current device's buffer size. ErrCode = 0x%08x", ret);
    return 0;
  }
  return bufferSize;
}

bool CCoreAudioUnit::SetMaxFramesPerSlice(UInt32 maxFrames)
{
  if (!m_Component)
    return false;
  
	OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetMaxFramesPerSlice: Unable to set AudioUnit max frames per slice. ErrCode = 0x%08x", ret);
    return false;
  }
  return true;  
}

// TODO: These are not true AudioUnit methods. Move to OutputDeviceAU class
void CCoreAudioUnit::Start()
{
  // TODO: Check component status
  if (m_Component)
    AudioOutputUnitStart(m_Component);  
}

void CCoreAudioUnit::Stop()
{
  // TODO: Check component status
  if (m_Component)
    AudioOutputUnitStop(m_Component);    
}

Float32 CCoreAudioUnit::GetCurrentVolume()
{
  if (!m_Component)
    return 0.0f;
  
  Float32 volPct = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_Component,  kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &volPct);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetCurrentVolume: Unable to get AudioUnit volume. ErrCode = 0x%08x", ret);
    return 0.0f;
  }
  return volPct;
}

bool CCoreAudioUnit::SetCurrentVolume(Float32 vol)
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitSetParameter(m_Component, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentVolume: Unable to set AudioUnit volume. ErrCode = 0x%08x", ret);
    return false;
  }
  return true;
}


#endif