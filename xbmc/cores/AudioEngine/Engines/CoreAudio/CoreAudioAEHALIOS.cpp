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

#if defined(TARGET_DARWIN_IOS)
#include "system.h"

#include "CoreAudioAEHALIOS.h"

#include "xbmc/cores/AudioEngine/Utils/AEUtil.h"
#include "AEFactory.h"
#include "CoreAudioAE.h"
#include "utils/log.h"

#include <math.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define BUFFERED_FRAMES 1024

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CIOSCoreAudioHardware
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioComponentInstance CIOSCoreAudioHardware::FindAudioDevice(std::string searchName)
{
  if (!searchName.length())
    return 0;

  AudioComponentInstance defaultDevice = GetDefaultOutputDevice();

  return defaultDevice;
}

AudioComponentInstance CIOSCoreAudioHardware::GetDefaultOutputDevice()
{
  AudioComponentInstance ret = (AudioComponentInstance)1;

  return ret;
}

UInt32 CIOSCoreAudioHardware::GetOutputDevices(IOSCoreAudioDeviceList* pList)
{
  if (!pList)
    return 0;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioUnit
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioUnit::CCoreAudioUnit() :
m_pSource         (NULL         ),
m_audioUnit       (NULL         ),
m_audioNode       (NULL         ),
m_audioGraph      (NULL         ),
m_Initialized     (false        ),
m_renderProc      (NULL         ),
m_busNumber       (INVALID_BUS  )
{
}

CCoreAudioUnit::~CCoreAudioUnit()
{
  Close();
}

bool CCoreAudioUnit::Open(AUGraph audioGraph, AudioComponentDescription desc)
{
  if (m_audioUnit)
    Close();

  OSStatus ret;

  m_Initialized = false;

  ret = AUGraphAddNode(audioGraph, &desc, &m_audioNode);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error add m_outputNode. Error = %s", GetError(ret).c_str());
    return false;
  }

  ret = AUGraphNodeInfo(audioGraph, m_audioNode, NULL, &m_audioUnit);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error getting m_outputNode. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_audioGraph  = audioGraph;
  m_Initialized = true;

  Start();

  return true;
}

bool CCoreAudioUnit::Open(AUGraph audioGraph, OSType type, OSType subType, OSType manufacturer)
{
  AudioComponentDescription desc;
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetFormat: Unable to get AudioUnit format. Bus : %d Scope : %d : Error = %s", (int)scope, (int)bus, GetError(ret).c_str());
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
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetFormat: Unable to set AudioUnit format. Bus : %d Scope : %d : Error = %s", (int)scope, (int)bus, GetError(ret).c_str());
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
                                   AudioStreamBasicDescription *streamDesc)
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
      streamDesc->mFormatFlags |= kAudioFormatFlagsNativeEndian;
      streamDesc->mFormatFlags |= kAudioFormatFlagIsFloat;
      break;
    case AE_FMT_S16NE:
    case AE_FMT_AC3:
    case AE_FMT_DTS:
    case AE_FMT_DTSHD:
    case AE_FMT_TRUEHD:
    case AE_FMT_EAC3:
      streamDesc->mFormatFlags |= kAudioFormatFlagsNativeEndian;
      streamDesc->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
    case AE_FMT_S16LE:
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
}

float CCoreAudioUnit::GetLatency()
{
  if (!m_audioUnit)
    return 0.0f;

  //kAudioSessionProperty_CurrentHardwareIOBufferDuration
  //kAudioSessionProperty_CurrentHardwareOutputLatency

  Float32 preferredBufferSize = 0.0f;
  UInt32 size = sizeof(preferredBufferSize);
  AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareOutputLatency, &size, &preferredBufferSize);
  return preferredBufferSize;
}

bool CCoreAudioUnit::SetSampleRate(Float64 sampleRate, AudioUnitScope scope, AudioUnitElement bus)
{
  if (!m_audioUnit)
    return false;

  UInt32 size = sizeof(Float64);

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_SampleRate, scope, bus, &sampleRate, size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetSampleRate: Unable to set AudioUnit format. Bus : %d Scope : %d : Error = %s", (int)scope, (int)bus, GetError(ret).c_str());
    return false;
  }
  return true;
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
CAUOutputDevice::CAUOutputDevice()
{
}

CAUOutputDevice::~CAUOutputDevice()
{
}

/*
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
*/

UInt32 CAUOutputDevice::GetBufferFrameSize()
{
  if (!m_audioUnit)
    return 0;

  return BUFFERED_FRAMES;
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAUMultiChannelMixer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAUMultiChannelMixer::CAUMultiChannelMixer()
{
}

CAUMultiChannelMixer::~CAUMultiChannelMixer()
{

}

UInt32 CAUMultiChannelMixer::GetInputBusCount()
{
  if (!m_audioUnit)
    return 0;

  UInt32 busCount = 0;
  UInt32 size = sizeof(busCount);
  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &busCount, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMultiChannelMixer::GetInputBusCount: Unable to get input bus count. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return busCount;
}

bool CAUMultiChannelMixer::SetInputBusFormat(UInt32 busCount, AudioStreamBasicDescription *pFormat)
{
  if (!m_audioUnit)
    return false;

  for (UInt32 i = 0; i < busCount; i++)
  {
    if (!SetFormat(pFormat, kAudioUnitScope_Input, i))
      return false;
  }

  return true;
}

bool CAUMultiChannelMixer::SetInputBusCount(UInt32 busCount)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &busCount, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMultiChannelMixer::SetInputBusCount: Unable to set input bus count. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

UInt32 CAUMultiChannelMixer::GetOutputBusCount()
{
  if (!m_audioUnit)
    return 0;

  UInt32 busCount = 0;
  UInt32 size = sizeof(busCount);
  OSStatus ret = AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Output, 0, &busCount, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMultiChannelMixer::GetOutputBusCount: Unable to get output bus count. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return busCount;
}

bool CAUMultiChannelMixer::SetOutputBusCount(UInt32 busCount)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Output, 0, &busCount, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMultiChannelMixer::SetOutputBusCount: Unable to set output bus count. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

Float32 CAUMultiChannelMixer::GetCurrentVolume()
{

  if (!m_audioUnit)
    return false;

  Float32 volPct = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Input, kInputBus, &volPct);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMultiChannelMixer::GetCurrentVolume: Unable to get Mixer volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return volPct;

}

bool CAUMultiChannelMixer::SetCurrentVolume(Float32 vol)
{

  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Output, kOutputBus, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMultiChannelMixer::SetCurrentVolume: Unable to set Mixer volume. Error = %s", GetError(ret).c_str());
    return false;
  }

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
m_allowMixing   (false)
{
  for (int i = 0; i < MAX_CONNECTION_LIMIT; i++)
  {
    m_reservedBusNumber[i] = false;
  }
}

CCoreAudioGraph::~CCoreAudioGraph()
{
  Close();
}

bool CCoreAudioGraph::Open(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing, float initVolume)
{
  OSStatus ret;

  AudioStreamBasicDescription inputFormat;
  AudioStreamBasicDescription outputFormat;

  m_allowMixing   = allowMixing;

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
  if (!m_audioUnit->Open(m_audioGraph, kAudioUnitType_Output, kAudioUnitSubType_RemoteIO, kAudioUnitManufacturer_Apple))
    return false;

  if (!m_audioUnit->EnableInputOuput())
    return false;

  m_audioUnit->GetFormatDesc(format, &inputFormat);

  //if(!allowMixing)
  //{
    if (!m_audioUnit->SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
      return false;

    if (!m_audioUnit->SetFormat(&inputFormat, kAudioUnitScope_Output, kInputBus))
      return false;
  //}

  if (allowMixing)
  {
    // get mixer unit
    if (m_mixerUnit)
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error mixer unit already open. double call ?");
      return false;
    }

    m_mixerUnit = new CAUMultiChannelMixer();

    if (!m_mixerUnit->Open(m_audioGraph, kAudioUnitType_Mixer, kAudioUnitSubType_MultiChannelMixer, kAudioUnitManufacturer_Apple))
      return false;

    // set number of input buses
    if (!m_mixerUnit->SetInputBusCount(MAX_CONNECTION_LIMIT))
      return false;

    //if(!m_mixerUnit->SetFormat(&fmt, kAudioUnitScope_Output, kOutputBus))
    //  return false;

    m_mixerUnit->SetBus(0);

    if (!m_audioUnit->GetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
      return false;

    /*
    if(!m_mixerUnit->SetInputBusFormat(MAX_CONNECTION_LIMIT, &outputFormat))
      return false;
    */

    ret =  AUGraphConnectNodeInput(m_audioGraph, m_mixerUnit->GetNode(), 0, m_audioUnit->GetNode(), 0);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error connecting m_m_mixerNode. Error = %s", GetError(ret).c_str());
      return false;
    }

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

    /*
    if(!m_inputUnit->SetFormat(&outputFormat, kAudioUnitScope_Output, kOutputBus))
      return false;
    */

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

    // Regenerate audio format and copy format for the Output AU
  }

  ret = AUGraphUpdate(m_audioGraph, NULL);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error update graph. Error = %s", GetError(ret).c_str());
    return false;
  }

  std::string formatString;
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

  SetCurrentVolume(initVolume);

  UInt32 bufferFrames = m_audioUnit->GetBufferFrameSize();

  m_audioUnit->SetMaxFramesPerSlice(bufferFrames);
  if (m_inputUnit)
    m_inputUnit->SetMaxFramesPerSlice(bufferFrames);

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
    CLog::Log(LOGERROR, "CCoreAudioGraph::Stop: Audio graph not running. Error = %s", GetError(ret).c_str());
    return false;
  }
  if (isRunning)
  {

    if (m_inputUnit)
      m_inputUnit->Stop();
    if (m_mixerUnit)
      m_mixerUnit->Stop();
    if (m_audioUnit)
      m_audioUnit->Stop();

    ret = AUGraphStop(m_audioGraph);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Stop: Error stopping audio graph. Error = %s", GetError(ret).c_str());
    }
  }

  return true;
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
  if (!m_mixerUnit)
    return false;

  return m_mixerUnit->SetCurrentVolume(vol);
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

  std::string formatString;
  AudioStreamBasicDescription inputFormat;
  AudioStreamBasicDescription outputFormat;

  OSStatus ret;

  int busNumber = GetFreeBus();
  if (busNumber == INVALID_BUS)
    return  NULL;

  // create output unit
  CAUOutputDevice *outputUnit = new CAUOutputDevice();
  if (!outputUnit->Open(m_audioGraph, kAudioUnitType_FormatConverter, kAudioUnitSubType_AUConverter, kAudioUnitManufacturer_Apple))
    goto error;

  m_audioUnit->GetFormatDesc(format, &inputFormat);

  // get the format frm the mixer
  if (!m_mixerUnit->GetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
    goto error;

  if (!outputUnit->SetFormat(&outputFormat, kAudioUnitScope_Output, kOutputBus))
    goto error;

  if (!outputUnit->SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
    goto error;

  ret = AUGraphConnectNodeInput(m_audioGraph, outputUnit->GetNode(), 0, m_mixerUnit->GetNode(), busNumber);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::CreateUnit: Error connecting outputUnit. Error = %s", GetError(ret).c_str());
    goto error;
  }

  // TODO: setup mixmap, get free bus number for connection

  outputUnit->SetBus(busNumber);

  AUGraphUpdate(m_audioGraph, NULL);

  printf("Add unit\n\n");
  ShowGraph();
  printf("\n");

  CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Input Stream Format  %s", StreamDescriptionToString(inputFormat, formatString));
  CLog::Log(LOGINFO, "CCoreAudioGraph::Open: Output Stream Format %s", StreamDescriptionToString(outputFormat, formatString));

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

float CCoreAudioGraph::GetLatency()
{
  float delay = 0.0f;

  if (m_audioUnit)
    delay += m_audioUnit->GetLatency();

  return delay;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioAEHALIOS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioAEHALIOS::CCoreAudioAEHALIOS() :
m_audioGraph        (NULL   ),
m_Initialized       (false  ),
m_Passthrough       (false  ),
m_allowMixing       (false  ),
m_encoded           (false  ),
m_initVolume        (1.0f   ),
m_NumLatencyFrames  (0      ),
m_OutputBufferIndex (0      )
{
}

CCoreAudioAEHALIOS::~CCoreAudioAEHALIOS()
{
  Deinitialize();

  delete m_audioGraph;
}

bool CCoreAudioAEHALIOS::InitializePCM(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing)
{

  if (m_audioGraph)
  {
    m_audioGraph->Close();
    delete m_audioGraph;
  }
  m_audioGraph = new CCoreAudioGraph();

  if (!m_audioGraph)
    return false;

  if (!m_audioGraph->Open(pSource, format, allowMixing, m_initVolume))
  {
    CLog::Log(LOGERROR, "CCoreAudioAEHALIOS::Initialize: Unable to initialize audio due a missconfiguration. Try 2.0 speaker configuration.");
    return false;
  }

  m_NumLatencyFrames = 0;

  m_allowMixing = allowMixing;

  return true;
}

bool CCoreAudioAEHALIOS::InitializePCMEncoded(ICoreAudioSource *pSource, AEAudioFormat &format)
{
  if (!InitializePCM(pSource, format, false))
    return false;

  return true;
}

bool CCoreAudioAEHALIOS::Initialize(ICoreAudioSource *ae, bool passThrough, AEAudioFormat &format, AEDataFormat rawDataFormat, std::string &device, float initVolume)
{
  m_ae = (CCoreAudioAE *)ae;

  if (!m_ae)
    return false;

  m_initformat          = format;
  m_Passthrough         = passThrough;
  m_encoded             = false;
  m_OutputBufferIndex   = 0;
  m_rawDataFormat       = rawDataFormat;
  m_initVolume          = initVolume;

  if (format.m_channelLayout.Count() == 0)
  {
    CLog::Log(LOGERROR, "CCoreAudioAEHALIOS::Initialize - Unable to open the requested channel layout");
    return false;
  }

  if (device.find("CoreAudio:"))
    device.erase(0, strlen("CoreAudio:"));

  // If this is a passthrough (AC3/DTS) stream, attempt to handle it natively
  if (m_Passthrough)
  {
    m_encoded = false;
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
      CLog::Log(LOGERROR, "CCoreAudioAEHALIOS::Initialize: No suitable AC3 output format found. Attempting DD-Wav.");
      configured = InitializePCMEncoded(ae, format);
    }
    else
    {
      // Standard PCM data
      configured = InitializePCM(ae, format, true);
    }

    if (!configured) // No suitable output format was able to be configured
      return false;
  }

  if (m_audioGraph)
    m_audioGraph->ShowGraph();

  m_Initialized = true;

  return true;
}

CAUOutputDevice *CCoreAudioAEHALIOS::DestroyUnit(CAUOutputDevice *outputUnit)
{
  if (m_audioGraph && outputUnit)
    return m_audioGraph->DestroyUnit(outputUnit);

  return NULL;
}

CAUOutputDevice *CCoreAudioAEHALIOS::CreateUnit(ICoreAudioSource *pSource, AEAudioFormat &format)
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

void CCoreAudioAEHALIOS::Deinitialize()
{
  if (!m_Initialized)
    return;

  Stop();

  //if (m_encoded)

  if (m_audioGraph)
    m_audioGraph->SetInputSource(NULL);

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
}

void CCoreAudioAEHALIOS::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  IOSCoreAudioDeviceList deviceList;
  CIOSCoreAudioHardware::GetOutputDevices(&deviceList);

  // Add default output device if GetOutputDevices return nothing
  devices.push_back(AEDevice("Default", "IOSCoreAudio:default"));

  std::string deviceName;
  for (int i = 0; !deviceList.empty(); i++)
  {
    std::string deviceName_Internal = std::string("IOSCoreAudio:") + deviceName;
    devices.push_back(AEDevice(deviceName, deviceName_Internal));

    deviceList.pop_front();

  }
}

void CCoreAudioAEHALIOS::Stop()
{
  if (!m_Initialized)
    return;

  m_audioGraph->Stop();
}

bool CCoreAudioAEHALIOS::Start()
{
  if (!m_Initialized)
    return false;

  m_audioGraph->Start();

  return true;
}

void CCoreAudioAEHALIOS::SetDirectInput(ICoreAudioSource *pSource, AEAudioFormat &format)
{
  if (!m_Initialized)
    return;

  // when HAL is initialized encoded we use directIO
  // when HAL is not in encoded mode and there is no mixer attach source the audio unit
  // when mixing is allowed in HAL, HAL is working with converter units where we attach the source.

  if (!m_encoded && !m_allowMixing)
  {
    // register render callback for the audio unit
    m_audioGraph->SetInputSource(pSource);
  }

  if (m_audioGraph)
    m_audioGraph->ShowGraph();

}

double CCoreAudioAEHALIOS::GetDelay()
{
  /*
  float delay;

  delay = (float)(m_NumLatencyFrames) / (m_initformat.m_sampleRate);

  return delay;
  */

  return (double)(BUFFERED_FRAMES) / (double)(m_initformat.m_sampleRate);
}

void  CCoreAudioAEHALIOS::SetVolume(float volume)
{
  if (!m_encoded)
    m_audioGraph->SetCurrentVolume(volume);
}

unsigned int CCoreAudioAEHALIOS::GetBufferIndex()
{
  return m_OutputBufferIndex;
}

#endif
