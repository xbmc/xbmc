/*
 *      Copyright (C) 2012 Team XBMC
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
#include "settings/Settings.h"

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

AudioUnit CIOSCoreAudioHardware::FindAudioDevice(CStdString searchName)
{
  if (!searchName.length())
    return 0;
  
  AudioUnit defaultDevice = GetDefaultOutputDevice();
  CLog::Log(LOGDEBUG, "CIOSCoreAudioHardware::FindAudioDevice: Returning default device [0x%04x].", (uint32_t)defaultDevice);

  return defaultDevice;  
}

AudioUnit CIOSCoreAudioHardware::GetDefaultOutputDevice()
{
  AudioUnit ret = (AudioUnit)1;
  
  return ret;
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
  m_AudioGraph(0),
  m_OutputNode(0),
  m_MixerNode(0),
  m_OutputUnit(0),
  m_MixerUnit(0)
{
  
}

CIOSCoreAudioDevice::CIOSCoreAudioDevice(AudioUnit deviceId)
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

  if(m_OutputUnit) 
  {
    if(!GetFormat(m_OutputUnit, kAudioUnitScope_Output, kInputBus, &pDesc))
      return;

    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Remote/IO Output Stream Bus %d Format %s", 
              kInputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));

    if(!GetFormat(m_OutputUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;

    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Remote/IO Input Stream Bus %d Format %s", 
              kOutputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));
  }

  if(m_MixerUnit) 
  {
    if(!GetFormat(m_MixerUnit, kAudioUnitScope_Input, kOutputBus, &pDesc))
      return;

    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Mixer Input Stream Bus %d Format %s", 
              kOutputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));
    if(!GetFormat(m_MixerUnit, kAudioUnitScope_Output, kOutputBus, &pDesc))
      return;

    CLog::Log(LOGDEBUG, "CIOSCoreAudioDevice::SetupInfo: Mixer Output Stream Bus %d Format %s", 
              kOutputBus, (char*)IOSStreamDescriptionToString(pDesc, formatString));
  }
}

bool CIOSCoreAudioDevice::Init(bool bPassthrough, AudioStreamBasicDescription* pDesc, AURenderCallback renderCallback, void *pClientData)
{
  OSStatus ret;
    
  if(!pDesc)
    return false;

  m_OutputUnit = 0;
  m_MixerUnit = 0;
  m_OutputNode = 0;
  m_MixerNode = 0;
  m_AudioGraph = 0;
  m_Passthrough = bPassthrough;

  //1. - create a audio graph
  ret = NewAUGraph(&m_AudioGraph);
  if(ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Open: Error create audio grpah. Error = %4.4s", CONVERT_OSSTATUS(ret));
    return false;
  }

  //2. - open the audio graph
  ret = AUGraphOpen(m_AudioGraph);
  if(ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Open: Error open audio grpah. Error = %4.4s", CONVERT_OSSTATUS(ret));
    return false;
  }

  //3. - create the remoteio unit and node (filled into m_OutputUnit and m_OutputNode)
  if(!OpenUnit(kAudioUnitType_Output, kAudioUnitSubType_RemoteIO, kAudioUnitManufacturer_Apple, m_OutputUnit, m_OutputNode))
    return false;

  //4. enable output of output unit    
  if(!EnableOutput(m_OutputUnit, kOutputBus, true))
    return false;
  
  //5. set input and output format to the passed pDesc (format of our raw data)
  if(!SetFormat(m_OutputUnit, kAudioUnitScope_Input, kOutputBus, pDesc))
    return false;
    
  if(!SetFormat(m_OutputUnit, kAudioUnitScope_Output, kInputBus, pDesc))
    return false;

  //if not pass through - create a mixer unit and node and add it to the graph
  //mixer unit is used for volume control in that case
  if (!m_Passthrough)
  {
    UInt32 busCount = 1;

    //6. create mixer unit and node (filled into m_MixerUnit and m_MixerNode    
    if(!OpenUnit(kAudioUnitType_Mixer, kAudioUnitSubType_MultiChannelMixer, kAudioUnitManufacturer_Apple, m_MixerUnit, m_MixerNode))
      return false;
    
    //7. set number of input buses for mixer unit to 1
    ret = AudioUnitSetProperty(m_MixerUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &busCount, sizeof(UInt32));
    if (ret)
    {
      CLog::Log(LOGERROR, "CIOSCoreAudioDevice::SetInputBusCount: Unable to set input bus count. Error = %4.4s", CONVERT_OSSTATUS(ret));
      return false;
    }
    
    //8. set input and output format of mixerunit the passed pDesc (format of our raw data)
    if(!SetFormat(m_MixerUnit,kAudioUnitScope_Input, kOutputBus,  pDesc))
      return false;

    if(!SetFormat(m_MixerUnit,kAudioUnitScope_Output, kOutputBus,  pDesc))
      return false;
    
    //9. connect output of mixer to input of output unit
    ret =  AUGraphConnectNodeInput(m_AudioGraph, m_MixerNode, 0, m_OutputNode, 0);
    if(ret)
    {
      CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Open: Error connecting m_m_mixerNode. Error = %4.4s", CONVERT_OSSTATUS(ret));
      return false;
    }
    
    //10. enable the input of our output unit (because it gets feed the sound from the mixer)
    if(!EnableInput(m_OutputUnit, kInputBus, true))
      return false;

    //11. update the graph for beeing sure the changes are ok
    ret = AUGraphUpdate(m_AudioGraph, NULL);
    if(ret)
    {
      CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Open: Error update graph. Error = %4.4s", CONVERT_OSSTATUS(ret));
      return false;
    }
    
    //12. hook our datacallback into the mixerunits input. (its the first unit in the chain)
    if(!SetRenderProc(m_MixerUnit, kOutputBus, renderCallback, pClientData))
      return false;
  }
  else//passthrough mode
  {
    //6.. hook our datacallback into the output units input. (its the only unit in the chain)
    if(!SetRenderProc(m_OutputUnit, kOutputBus, renderCallback, pClientData))
      return false;  
  }
  
  //prints the format info of the units in the graph
  SetupInfo();   
  //prints the graph into stdout
  CAShow(m_AudioGraph);  
  return true;
}

bool CIOSCoreAudioDevice::OpenUnit(OSType type, OSType subType, OSType manufacturer, AudioUnit &unit, AUNode &node)
{ 
  OSStatus ret;
  AudioComponentDescription desc;
  desc.componentType = type;
  desc.componentSubType = subType;
  desc.componentManufacturer = manufacturer;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  
  if(!m_AudioGraph)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::OpenUnit: Error no audio graph.");
    return false;
  }
  
  ret = AUGraphAddNode(m_AudioGraph, &desc, &node);
  if(ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::OpenUnit: Error add m_outputNode. Error = %4.4s", CONVERT_OSSTATUS(ret));
    return false;
  }
  
  ret = AUGraphNodeInfo(m_AudioGraph, node, NULL, &unit);
  if(ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::OpenUnit: Error getting m_outputNode. Error = %4.4s", CONVERT_OSSTATUS(ret));
    return false;
  }
    
  return true;
}

bool CIOSCoreAudioDevice::Open()
{
  OSStatus ret;
  if(!m_AudioGraph)
    return false;

  ret = AUGraphInitialize(m_AudioGraph);

  if(ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Open: Error initialize graph. Error = %4.4s", CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

void CIOSCoreAudioDevice::Close()
{
  OSStatus ret;

  if (!m_AudioGraph)
    return;

  Stop();

  ret = AUGraphUninitialize(m_AudioGraph);
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Close: Unable to close audiograph. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));

  ret = DisposeAUGraph(m_AudioGraph);

  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Close: Unable to dispose audiograph. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));

  m_AudioGraph = 0;
}

void CIOSCoreAudioDevice::Start()
{
  OSStatus ret;

  if(!m_AudioGraph)
    return;
  ret = AUGraphStart(m_AudioGraph);
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Start: Unable to start audiograph. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));    
}

void CIOSCoreAudioDevice::Stop()
{
  OSStatus ret;

  if (!m_AudioGraph)
    return;
  
  ret = AUGraphStop(m_AudioGraph);
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::Stop: Unable to audiograph. Error = 0x%08x (%4.4s).", (uint32_t)ret, CONVERT_OSSTATUS(ret));  
}

const char* CIOSCoreAudioDevice::GetName(CStdString& name)
{
  if (!m_AudioGraph)
    return NULL;

  return name.c_str();
}

bool CIOSCoreAudioDevice::EnableInput(AudioUnit audioUnit, AudioUnitElement bus, bool bEnable)
{
  UInt32 hasio;
  UInt32 size=sizeof(UInt32);

  if (!audioUnit)
    return false; 

  OSStatus  ret = AudioUnitGetProperty(audioUnit,kAudioOutputUnitProperty_HasIO,kAudioUnitScope_Input, 1, &hasio, &size);
  
  if(hasio)
  {  
    UInt32 flag = bEnable ? 1 : 0;
    ret = AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, 
                                        kAudioUnitScope_Input, bus, &flag, sizeof(flag));
    if (ret)
    {
      CLog::Log(LOGERROR, "CIOSCoreAudioUnit::EnableInput: Failed to enable input. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
      return false;
    }
  }
  return true;
}

bool CIOSCoreAudioDevice::EnableOutput(AudioUnit audioUnit, AudioUnitElement bus, bool bEnable)
{
  UInt32 hasio;
  UInt32 size=sizeof(UInt32);
 
  if (!audioUnit)
    return false;

  OSStatus ret = AudioUnitGetProperty(audioUnit,kAudioOutputUnitProperty_HasIO,kAudioUnitScope_Output, 1, &hasio, &size);
  
  if(hasio)
  {  
    UInt32 flag = bEnable ? 1 : 0;
    ret = AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, 
                                        kAudioUnitScope_Output, bus, &flag, sizeof(flag));
    if (ret)
    {
      CLog::Log(LOGERROR, "CIOSCoreAudioUnit::EnableInput: Failed to enable output. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
      return false;
    }
  }
  return true;
}

Float32 CIOSCoreAudioDevice::GetCurrentVolume() const
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

//vol is passed in as value between 0.0 and 1.0
bool CIOSCoreAudioDevice::SetCurrentVolume(Float32 vol) 
{
  if (!m_MixerUnit || m_Passthrough)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_MixerUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Output, kOutputBus, vol, 0); 
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::SetCurrentVolume: Unable to set Mixer volume. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }

  return true;
}

bool CIOSCoreAudioDevice::GetFormat(AudioUnit audioUnit, AudioUnitScope scope,
                                    AudioUnitElement bus, AudioStreamBasicDescription* pDesc)
{
  if (!audioUnit || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat, 
                                      scope, bus, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CIOSCoreAudioDevice::GetFormat: Unable to get Audio Unit format bus %d. Error = 0x%08x (%4.4s)", 
             (int)bus, (uint32_t)ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CIOSCoreAudioDevice::SetFormat(AudioUnit audioUnit, AudioUnitScope scope, 
                                    AudioUnitElement bus, AudioStreamBasicDescription* pDesc)
{
  if (!audioUnit || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, 
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
  if (!m_OutputUnit)
    return false;
  
  UInt32 maximumFramesPerSlice = nSlices;
  OSStatus ret = AudioUnitSetProperty(m_OutputUnit, kAudioUnitProperty_MaximumFramesPerSlice, 
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
  if (!m_OutputUnit)
    return;
  
  struct AudioChannelLayout layout;
  layout.mChannelBitmap = 0;
  layout.mNumberChannelDescriptions = 0;
  layout.mChannelLayoutTag = layoutTag;

  OSStatus ret = AudioUnitSetProperty(m_OutputUnit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, kOutputBus, &layout, sizeof (layout));
  if (ret)
    CLog::Log(LOGERROR, "CIOSCoreAudioUnit::AudioUnitSetProperty: Unable to set property kAudioUnitProperty_AudioChannelLayout. Error = 0x%08x (%4.4s)", (uint32_t)ret, CONVERT_OSSTATUS(ret));
}
                      
bool CIOSCoreAudioDevice::SetRenderProc(AudioUnit componentInstance, AudioUnitElement bus,
                                        AURenderCallback callback, void* pClientData)
{
  if (!componentInstance)
    return false;
  
  AURenderCallbackStruct callbackInfo;
	callbackInfo.inputProc = callback; // Function to be called each time the AudioUnit needs data
	callbackInfo.inputProcRefCon = pClientData; // Pointer to be returned in the callback proc
	OSStatus ret = AudioUnitSetProperty(componentInstance, kAudioUnitProperty_SetRenderCallback, 
                                      kAudioUnitScope_Input, bus, &callbackInfo, 
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
