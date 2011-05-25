/*
 *      Copyright (C) 2010 Team XBMC
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

#if defined(__APPLE__) && defined(__arm__)
#include <math.h>

#include "IOSCoreAudio.h"
#include "PlatformDefs.h"
#include "utils/log.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

char* IOSUInt32ToFourCC(UInt32* pVal) // NOT NULL TERMINATED! Modifies input value.
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

const char* IOSStreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str)
{
  UInt32 formatId = desc.mFormatID;
  char* fourCC = IOSUInt32ToFourCC(&formatId);
  
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
// CIOSCoreAudioHardware
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioComponentInstance CIOSCoreAudioHardware::FindAudioDevice(CStdString searchName)
{
  if (!searchName.length())
    return 0;
  
  AudioComponentInstance defaultDevice = GetDefaultOutputDevice();
  CLog::Log(LOGDEBUG, "CIOSCoreAudioHardware::FindAudioDevice: Returning default device [0x%04x].", (uint32_t)defaultDevice);

  return defaultDevice;  
}

AudioComponentInstance CIOSCoreAudioHardware::GetDefaultOutputDevice()
{
  AudioComponentInstance ret = (AudioComponentInstance)1;
  
  return ret;
  
  /*
  OSStatus ret;
  AudioComponentInstance audioUnit;
  
  // Describe audio component
  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_RemoteIO;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  
  // Get component
  AudioComponent inputComponent = AudioComponentFindNext(NULL, &desc);
  
  // Get audio units
  ret = AudioComponentInstanceNew(inputComponent, &audioUnit);
  
  if (ret) {
    CLog::Log(LOGERROR, "CIOSCoreAudioHardware::GetDefaultOutputDevice: Unable to identify default output device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return 0;
  }
  
  return audioUnit;
  */
}

UInt32 CIOSCoreAudioHardware::GetOutputDevices(IOSCoreAudioDeviceList* pList)
{
  if (!pList)
    return 0;
  
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CIOSCoreAudioDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CIOSCoreAudioDevice::CIOSCoreAudioDevice()  : 
  m_AudioUnit(0),
  m_MixerUnit(0)
{
  
}

CIOSCoreAudioDevice::CIOSCoreAudioDevice(AudioComponentInstance deviceId)
{
}

CIOSCoreAudioDevice::~CIOSCoreAudioDevice()
{
  Stop();
}

void CIOSCoreAudioDevice::SetupInfo()
{
  AudioStreamBasicDescription pDesc;
  CStdString formatString;

  if(m_AudioUnit) 
  {
    if(!GetFormat(m_AudioUnit, kAudioUnitScope_Output, kInputBus, &pDesc))
      return;

     CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Remote/IO Output Stream Bus %d Format %s", 
              kInputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));

    if(!GetFormat(m_AudioUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;

     CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Remote/IO Input Stream Bus %d Format %s", 
              kInputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));
  }

  if(m_MixerUnit) 
  {
    if(!GetFormat(m_AudioUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;

     CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Remote/IO Output Stream Bus %d Format %s", 
              kInputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));
  
    if(!GetFormat(m_MixerUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;
  
    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Mixer Input Stream Bus %d Format %s", 
              kOutputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));
  
    if(!GetFormat(m_MixerUnit, kAudioUnitScope_Input, kInputBus, &pDesc))
      return;

    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Mixer Input Stream Bus %d Format %s", 
              kInputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));
  }
  
}

bool CIOSCoreAudioDevice::Init(bool bPassthrough, AudioStreamBasicDescription* pDesc, AURenderCallback renderCallback, void *pClientData)
{
  OSStatus ret;
  AudioComponent audioComponent;
  AudioComponentDescription desc;
  
  m_AudioUnit = 0;
  m_MixerUnit = 0;
  m_Passthrough = bPassthrough;
  
  /*
  ret = AudioSessionInitialize(NULL, NULL, NULL, NULL);
  if (ret) {
    CLog::Log(LOGERROR, "CIOSCoreAudioHardware::Init: Unable to initialize session. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  
  ret = AudioSessionSetActive(true);
  if (ret) {
    CLog::Log(LOGERROR, "CIOSCoreAudioHardware::Init: Unable to set session active. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  */
  
  // Describe audio component
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_RemoteIO;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  
  // Get component
  audioComponent = AudioComponentFindNext(NULL, &desc);
  
  // Get audio unit
  ret = AudioComponentInstanceNew(audioComponent, &m_AudioUnit);
  
  if (ret) {
    CLog::Log(LOGERROR, "CIOSCoreAudioHardware::Init: Unable to open Remote/IO device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  
  if(!EnableOutput(m_AudioUnit, kOutputBus))
    return false;

  if(!SetFormat(m_AudioUnit, kAudioUnitScope_Input, kOutputBus, pDesc))
    return false;
  
  if(!SetFormat(m_AudioUnit, kAudioUnitScope_Output, kInputBus, pDesc))
    return false;
  
  if(!m_Passthrough) { 
    // Describe audio component
    desc.componentType = kAudioUnitType_Mixer;
    desc.componentSubType = kAudioUnitSubType_AU3DMixerEmbedded;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  
    // Get component
    audioComponent = AudioComponentFindNext(NULL, &desc);
    
    // Get mixer unit
    ret = AudioComponentInstanceNew(audioComponent, &m_MixerUnit);
    if (ret) {
      CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Init: Unable to open Mixer device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
      return false;
    }
  
    if(!SetFormat(m_MixerUnit, kAudioUnitScope_Input, kOutputBus, pDesc))
      return false;

    if(!SetFormat(m_MixerUnit, kAudioUnitScope_Input, kInputBus, pDesc))
      return false;
  
    if(!SetRenderProc(m_MixerUnit, kOutputBus, renderCallback, pClientData))
      return false;
  
    // Connect mixer to output
    AudioUnitConnection connection;
    connection.sourceAudioUnit = m_MixerUnit;
    connection.sourceOutputNumber = kOutputBus;
    connection.destInputNumber = kOutputBus;
  
    ret = AudioUnitSetProperty(m_AudioUnit, kAudioUnitProperty_MakeConnection, 
                               kAudioUnitScope_Input, kOutputBus, &connection, sizeof(connection));
    if (ret)
    { 
      CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Init: Unable to make IO connections. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
      return false; 
    }

  } else {
    if(!SetRenderProc(m_AudioUnit, kOutputBus, renderCallback, pClientData))
      return false;
  }

  SetupInfo();
  
  return true;
}

bool CIOSCoreAudioDevice::Open()
{
  if(!m_AudioUnit)
    return false;

  OSStatus ret;
    
  if(!m_Passthrough) {
    ret = AudioUnitInitialize(m_MixerUnit);
    if (ret)
    { 
      CLog::Log(LOGERROR, "CIOSCoreAudioUnit::Open: Unable to Open Mixer device. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
      return false; 
    }
  }

  ret = AudioUnitInitialize(m_AudioUnit);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::Open: Unable to Open AudioUnit. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false; 
  } 

  return true;
}

void CIOSCoreAudioDevice::Close()
{
  if (!m_AudioUnit)
    return;
 
  Stop();
 
  if(m_Passthrough)
    SetRenderProc(m_AudioUnit, kOutputBus, nil, nil);
  else
    SetRenderProc(m_MixerUnit, kOutputBus, nil, nil);
  
  OSStatus ret = AudioUnitUninitialize(m_AudioUnit);
  
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Close: Unable to close Audio device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    
  ret = AudioComponentInstanceDispose(m_AudioUnit);
  
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Close: Unable to dispose Audio device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    
  m_AudioUnit = 0;
  
  if(!m_Passthrough) { 
    ret = AudioUnitUninitialize(m_MixerUnit);
    if (ret)
      CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Close: Unable to close Mixer device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
  
    ret = AudioComponentInstanceDispose(m_MixerUnit);
  
    if (ret)
      CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Close: Unable to dispose Mixer device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    
    m_MixerUnit = 0;
    
  }
}

void CIOSCoreAudioDevice::Start()
{
  if (!m_AudioUnit) 
    return;
  
  OSStatus ret ;
  
  ret = AudioOutputUnitStart(m_AudioUnit);
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Start: Unable to start device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
  
}

void CIOSCoreAudioDevice::Stop()
{
  if (!m_AudioUnit)
    return;
  
  OSStatus ret = AudioOutputUnitStop(m_AudioUnit);
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Stop: Unable to stop device. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));
  
}

const char* CIOSCoreAudioDevice::GetName(CStdString& name)
{
  if (!m_AudioUnit)
    return NULL;

  return name.c_str();
}

bool CIOSCoreAudioDevice::EnableInput(AudioComponentInstance componentInstance, AudioUnitElement bus)
{
  if (!componentInstance)
    return false;
  
  UInt32 flag = 0;
  OSStatus ret = AudioUnitSetProperty(componentInstance, kAudioOutputUnitProperty_EnableIO, 
                                kAudioUnitScope_Input, bus, &flag, sizeof(flag));
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::EnableInput: Failed to enable input. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CIOSCoreAudioDevice::EnableOutput(AudioComponentInstance componentInstance, AudioUnitElement bus)
{
  if (!componentInstance)
    return false;
  
  UInt32 flag = 1;
  OSStatus ret = AudioUnitSetProperty(componentInstance, kAudioOutputUnitProperty_EnableIO, 
                                      kAudioUnitScope_Output, bus, &flag, sizeof(flag));
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::EnableInput: Failed to enable output. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

Float32 CIOSCoreAudioDevice::GetCurrentVolume() 
{

  if (!m_MixerUnit)
    return false;

  Float32 volPct = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_MixerUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Input, kInputBus, &volPct);
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::GetCurrentVolume: Unable to get Mixer volume. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return 0.0f;
  }
  return volPct;
 
}

bool CIOSCoreAudioDevice::SetCurrentVolume(Float32 vol) 
{

  if (!m_MixerUnit && m_Passthrough)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_MixerUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Input, kInputBus, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::SetCurrentVolume: Unable to set Mixer volume. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }

  ret = AudioUnitSetParameter(m_MixerUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Input, kInputBus, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::SetCurrentVolume: Unable to set Mixer volume. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CIOSCoreAudioDevice::GetFormat(AudioComponentInstance componentInstance, AudioUnitScope scope,
                                    AudioUnitElement bus, AudioStreamBasicDescription* pDesc)
{
  if (!componentInstance || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(componentInstance, kAudioUnitProperty_StreamFormat, 
                                      scope, bus, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::GetFormat: Unable to get Audio Unit format bus %d. Error = 0x%08x (%4.4s)", 
             (int)bus, (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CIOSCoreAudioDevice::SetFormat(AudioComponentInstance componentInstance, AudioUnitScope scope, 
                                    AudioUnitElement bus, AudioStreamBasicDescription* pDesc)
{
  if (!componentInstance || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitSetProperty(componentInstance, kAudioUnitProperty_StreamFormat, 
                                      scope, bus, pDesc, size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::SetFormat: Unable to set Audio Unit format bus %d. Error = 0x%08x (%4.4s)", 
              (int)bus, (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

int CIOSCoreAudioDevice::FramesPerSlice(int nSlices)
{
  if (!m_AudioUnit)
    return false;
  
  UInt32 maximumFramesPerSlice = nSlices;
  OSStatus ret = AudioUnitSetProperty(m_AudioUnit, kAudioUnitProperty_MaximumFramesPerSlice, 
                                      kAudioUnitScope_Global, kOutputBus, &maximumFramesPerSlice, sizeof (maximumFramesPerSlice));
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::FramesPerSlice: Unable to setFramesPerSlice. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return maximumFramesPerSlice;  


}
                      
void CIOSCoreAudioDevice::AudioChannelLayout(int layoutTag)
{
  if (!m_AudioUnit)
    return;
  
  struct AudioChannelLayout layout;
  layout.mChannelBitmap = 0;
  layout.mNumberChannelDescriptions = 0;
  layout.mChannelLayoutTag = layoutTag;

  OSStatus ret = AudioUnitSetProperty(m_AudioUnit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, kOutputBus, &layout, sizeof (layout));
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::AudioUnitSetProperty: Unable to set property kAudioUnitProperty_AudioChannelLayout. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
}
                      
bool CIOSCoreAudioDevice::SetRenderProc(AudioComponentInstance componentInstance, AudioUnitElement bus,
                                        AURenderCallback callback, void* pClientData)
{
  if (!componentInstance)
    return false;
  
  AURenderCallbackStruct callbackInfo;
	callbackInfo.inputProc = callback; // Function to be called each time the AudioUnit needs data
	callbackInfo.inputProcRefCon = pClientData; // Pointer to be returned in the callback proc
	OSStatus ret = AudioUnitSetProperty(componentInstance, kAudioUnitProperty_SetRenderCallback, 
                                      kAudioUnitScope_Global, bus, &callbackInfo, 
                                      sizeof(AURenderCallbackStruct));
  
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::SetRenderProc: Unable to set AudioUnit render callback. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  
  return true;
}

bool CIOSCoreAudioDevice::SetSessionListener(AudioSessionPropertyID inID,
                                             AudioSessionPropertyListener inProc, void* pClientData)
{
	OSStatus ret = AudioSessionAddPropertyListener(inID, inProc, pClientData);
  
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::SetSessionListener: Unable to set Session Listener Callback. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  
  return true;
}

#endif
