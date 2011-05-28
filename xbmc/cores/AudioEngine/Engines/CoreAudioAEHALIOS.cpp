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

#if defined(__APPLE__) && defined(__arm__)
#include <math.h>

#include "CoreAudioAEHALIOS.h"

#include "PlatformDefs.h"
#include "utils/log.h"
#include "system.h"
#include "CoreAudioAE.h"
#include "AEUtil.h"
#include "AEFactory.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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
              kInputBus, (char*)StreamDescriptionToString(pDesc, formatString));
    
    if(!GetFormat(m_AudioUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;
    
    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Remote/IO Input Stream Bus %d Format %s", 
              kInputBus, (char*)StreamDescriptionToString(pDesc, formatString));
  }
  
  if(m_MixerUnit) 
  {
    if(!GetFormat(m_AudioUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;
    
    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Remote/IO Output Stream Bus %d Format %s", 
              kInputBus, (char*)StreamDescriptionToString(pDesc, formatString));
    
    if(!GetFormat(m_MixerUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;
    
    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Mixer Input Stream Bus %d Format %s", 
              kOutputBus, (char*)StreamDescriptionToString(pDesc, formatString));
    
    if(!GetFormat(m_MixerUnit, kAudioUnitScope_Input, kInputBus, &pDesc))
      return;
    
    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Mixer Input Stream Bus %d Format %s", 
              kInputBus, (char*)StreamDescriptionToString(pDesc, formatString));
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioAEHALIOS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCoreAudioAEHALIOS::CCoreAudioAEHALIOS() :
	m_Initialized(false),
	m_Passthrough(false),
	m_BytesPerFrame(0),
	m_BytesPerSec(0),
	m_NumLatencyFrames(0),
	m_OutputBufferIndex(0)
{
	m_AudioDevice		= new CIOSCoreAudioDevice;
}

CCoreAudioAEHALIOS::~CCoreAudioAEHALIOS()
{
	delete m_AudioDevice;
}

bool CCoreAudioAEHALIOS::Initialize(IAE *ae, bool passThrough, AEAudioFormat &format, CStdString &device)
{ 
	m_ae = (CCoreAudioAE *)ae;

	if(!m_ae)
		return false;
	
	m_Passthrough = passThrough;
	
  unsigned int bps = CAEUtil::DataFormatToBits(format.m_dataFormat);;
  
  if (format.m_channelCount == 0)
  {
    CLog::Log(LOGERROR, "CCoreAudioAEHALIOS::Initialize - Unable to open the requested channel layout");
    return false;
  }
  
  // Set the input stream format for the AudioUnit
  // We use the default DefaultOuput AudioUnit, so we only can set the input stream format.
  // The autput format is automaticaly set to the input format.
  AudioStreamBasicDescription audioFormat;
  audioFormat.mFormatID = kAudioFormatLinearPCM;						//  Data encoding format
  audioFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsPacked;
	switch(format.m_dataFormat) {
    case AE_FMT_FLOAT:
      audioFormat.mFormatFlags |= kAudioFormatFlagIsFloat;
      break;
    default:
      audioFormat.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
	}
  audioFormat.mChannelsPerFrame = format.m_channelCount;		// Number of interleaved audiochannels
  audioFormat.mSampleRate = (Float64)format.m_sampleRate;		//  the sample rate of the audio stream
  audioFormat.mBitsPerChannel = bps;						// Number of bits per sample, per channel
  audioFormat.mBytesPerFrame = (bps>>3) * format.m_channelCount; // Size of a frame == 1 sample per channel   
  audioFormat.mFramesPerPacket = 1;													// The smallest amount of indivisible data. Always 1 for uncompressed audio   
  audioFormat.mBytesPerPacket = audioFormat.mBytesPerFrame * audioFormat.mFramesPerPacket;
  audioFormat.mReserved = 0;
	
  // Attach our output object to the device
  if(!m_AudioDevice->Init(/*m_Passthrough*/ true, &audioFormat, m_ae->RenderCallback, m_ae))
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALIOS::Init failed");
    return false;
  }
	
	UInt32 m_PacketSize = 64;
	m_AudioDevice->FramesPerSlice(m_PacketSize);
  
  // set the format parameters
  m_BytesPerFrame = audioFormat.mBytesPerFrame;
	
	if (!m_AudioDevice->Open())
		return false;

  // set the format parameters
  format.m_frameSize    = m_BytesPerFrame;
  
  m_Initialized = true;
  
  return true;
}

void CCoreAudioAEHALIOS::Deinitialize()
{
  if(!m_Initialized)
    return;
	
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
  
  CLog::Log(LOGINFO, "CCoreAudioAEHALIOS::Deinitialize: Audio device has been closed.");
}

void CCoreAudioAEHALIOS::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
	IOSCoreAudioDeviceList deviceList;
	CIOSCoreAudioHardware::GetOutputDevices(&deviceList);
	
	// Add default output device if GetOutputDevices return nothing
	devices.push_back(AEDevice("Default", "IOSCoreAudio:default"));
	
	CStdString deviceName;
	for (int i = 0; !deviceList.empty(); i++)
	{
		CIOSCoreAudioDevice device(deviceList.front());
		device.GetName(deviceName);
		
		CStdString deviceName_Internal = CStdString("IOSCoreAudio:") + deviceName;
		devices.push_back(AEDevice(deviceName, deviceName_Internal));
		
		deviceList.pop_front();
		
	}
}

void CCoreAudioAEHALIOS::Stop()
{
  if(!m_Initialized)
    return;
	
  m_AudioDevice->Stop();
}

bool CCoreAudioAEHALIOS::Start()
{
  if(!m_Initialized)
    return false;
  
  m_AudioDevice->Start();

	return true;
}

float CCoreAudioAEHALIOS::GetDelay()
{   
  return 0.0f;
}


#endif