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

#include "CoreAudioUnit.h"

#include "CoreAudioAEHAL.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

#include <AudioToolbox/AUGraph.h>

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
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error add m_outputNode. Error = %s", GetError(ret).c_str());
    return false;
  }

  ret = AUGraphNodeInfo(audioGraph, m_audioNode, 0, &m_audioUnit);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error getting m_outputNode. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_audioGraph  = audioGraph;
  m_Initialized = true;

  return true;
}

bool CCoreAudioUnit::Open(AUGraph audioGraph, OSType type, OSType subType, OSType manufacturer)
{
  AudioComponentDescription desc = {0};
  desc.componentType = type;
  desc.componentSubType = subType;
  desc.componentManufacturer = manufacturer;

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
    if (ret && ret != kAUGraphErr_NodeNotFound)
    {
      CLog::Log(LOGERROR, "CCoreAudioUnit::Close: "
        "Unable to disconnect AudioUnit. Error = %s", GetError(ret).c_str());
    }

    ret = AUGraphRemoveNode(m_audioGraph, m_audioNode);
    if (ret != noErr)
    {
      CLog::Log(LOGERROR, "CCoreAudioUnit::Close: "
        "Unable to remove AudioUnit. Error = %s", GetError(ret).c_str());
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
  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioUnitProperty_StreamFormat, scope, bus, pDesc, &size);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetFormat: "
      "Unable to get AudioUnit format. Bus : %d Scope : %d : Error = %s",
        (int)bus, (int)scope, GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioUnit::SetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus)
{
  if (!m_audioUnit || !pDesc)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit,
    kAudioUnitProperty_StreamFormat, scope, bus, pDesc, sizeof(AudioStreamBasicDescription));
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetFormat: "
      "Unable to set AudioUnit format. Bus : %d Scope : %d : Error = %s",
       (int)bus, (int)scope, GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioUnit::SetMaxFramesPerSlice(UInt32 maxFrames)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit,
    kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, sizeof(UInt32));
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetMaxFramesPerSlice: "
      "Unable to set AudioUnit max frames per slice. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioUnit::GetSupportedChannelLayouts(AudioChannelLayoutList* pLayouts)
{
  if (!m_audioUnit || !pLayouts)
    return false;

  UInt32 propSize = 0;
  Boolean writable = false;
  OSStatus ret = AudioUnitGetPropertyInfo(m_audioUnit,
    kAudioUnitProperty_SupportedChannelLayoutTags, kAudioUnitScope_Input, 0, &propSize, &writable);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetSupportedChannelLayouts: "
      "Unable to retrieve supported channel layout property info. Error = %s", GetError(ret).c_str());
    return false;
  }
  UInt32 layoutCount = propSize / sizeof(AudioChannelLayoutTag);
  AudioChannelLayoutTag* pSuppLayouts = new AudioChannelLayoutTag[layoutCount];
  ret = AudioUnitGetProperty(m_audioUnit,
    kAudioUnitProperty_SupportedChannelLayoutTags, kAudioUnitScope_Output, 0, pSuppLayouts, &propSize);
  if (ret != noErr)
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
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetRenderProc: "
      "Unable to set AudioUnit render callback. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_renderProc = RenderCallback;

  return true;
}

bool CCoreAudioUnit::RemoveRenderProc()
{
  if (!m_audioUnit || !m_renderProc)
    return false;

  AudioUnitInitialize(m_audioUnit);

  AURenderCallbackStruct callbackInfo;
  callbackInfo.inputProc = nil;
  callbackInfo.inputProcRefCon = nil;
  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_SetRenderCallback,
    kAudioUnitScope_Input, 0, &callbackInfo, sizeof(AURenderCallbackStruct));
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::RemoveRenderProc: "
      "Unable to remove AudioUnit render callback. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_renderProc = NULL;
  Sleep(100);

  return true;
}

OSStatus CCoreAudioUnit::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
  const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
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
  AudioStreamBasicDescription *streamDesc, AudioStreamBasicDescription *coreaudioDesc)
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

  // Audio units use noninterleaved 32-bit floating point
  //  linear PCM data for input and output, ...except in the 
  // case of an audio unit that is a data format converter,
  // which converts to or from this format.
  coreaudioDesc->mFormatID = kAudioFormatLinearPCM;
  coreaudioDesc->mFormatFlags = kAudioFormatFlagsNativeEndian |
    kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
  switch (format.m_dataFormat)
  {
    case AE_FMT_FLOAT:
      coreaudioDesc->mFormatFlags |= kAudioFormatFlagIsFloat;
    default:
      coreaudioDesc->mFormatFlags |= kAudioFormatFlagIsSignedInteger;
      break;
  }
  coreaudioDesc->mBitsPerChannel   = bps; //sizeof(Float32)<<3;
  coreaudioDesc->mSampleRate       = (Float64)format.m_sampleRate;;
  coreaudioDesc->mFramesPerPacket  = 1;
  coreaudioDesc->mChannelsPerFrame = streamDesc->mChannelsPerFrame;
  coreaudioDesc->mBytesPerFrame    = (bps>>3); //sizeof(Float32);
  coreaudioDesc->mBytesPerPacket   = (bps>>3); //sizeof(Float32);
}

float CCoreAudioUnit::GetLatency()
{
  if (!m_audioUnit)
    return 0.0f;

  Float64 latency;
  UInt32 size = sizeof(latency);

  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0, &latency, &size);

  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetLatency: "
      "Unable to set AudioUnit latency. Error = %s", GetError(ret).c_str());
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

  OSStatus ret = AudioUnitSetProperty(m_audioUnit, kAudioOutputUnitProperty_CurrentDevice,
    kAudioUnitScope_Global, kOutputBus, &deviceId, sizeof(AudioDeviceID));
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentDevice: "
      "Unable to set current device. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_DeviceId = deviceId;

  return true;
}

bool CAUOutputDevice::GetChannelMap(CoreAudioChannelList* pChannelMap)
{
  if (!m_audioUnit)
    return false;

  UInt32 size = 0;
  Boolean writable = false;
  AudioUnitGetPropertyInfo(m_audioUnit,
    kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, &size, &writable);

  UInt32 channels = size/sizeof(SInt32);
  SInt32* pMap = new SInt32[channels];
  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, &size);
  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputChannelMap: "
      "Unable to retrieve AudioUnit input channel map. Error = %s", GetError(ret).c_str());
  else
    for (UInt32 i = 0; i < channels; i++)
      pChannelMap->push_back(pMap[i]);
  delete[] pMap;
  return (!ret);
}

bool CAUOutputDevice::SetChannelMap(CoreAudioChannelList* pChannelMap)
{
  // The number of array elements must match the
  // number of output channels provided by the device
  if (!m_audioUnit || !pChannelMap)
    return false;

  UInt32 channels = pChannelMap->size();
  UInt32 size = sizeof(SInt32) * channels;
  SInt32* pMap = new SInt32[channels];
  for (UInt32 i = 0; i < channels; i++)
    pMap[i] = (*pChannelMap)[i];

  OSStatus ret = AudioUnitSetProperty(m_audioUnit,
    kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, size);
  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: "
      "Unable to get current device's buffer size. Error = %s", GetError(ret).c_str());
  delete[] pMap;
  return (!ret);
}

Float32 CAUOutputDevice::GetCurrentVolume()
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 volPct = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit,
    kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &volPct);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetCurrentVolume: "
      "Unable to get AudioUnit volume. Error = %s", GetError(ret).c_str());
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
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentVolume: "
      "Unable to set AudioUnit volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

UInt32 CAUOutputDevice::GetBufferFrameSize()
{
  if (!m_audioUnit)
    return 0;

  UInt32 bufferSize = 0;
  UInt32 size = sizeof(UInt32);

  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, &bufferSize, &size);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: "
      "Unable to get current device's buffer size. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return bufferSize;
}

bool CAUOutputDevice::EnableInputOuput()
{
  if (!m_audioUnit)
    return false;

  UInt32 hasio;
  UInt32 size=sizeof(UInt32);
  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioOutputUnitProperty_HasIO,kAudioUnitScope_Input, 1, &hasio, &size);

  if (hasio)
  {
    UInt32 enable;
    enable = 0;
    ret =  AudioUnitSetProperty(m_audioUnit,
      kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &enable, sizeof(enable));
    if (ret != noErr)
    {
      CLog::Log(LOGERROR, "CAUOutputDevice::EnableInputOuput:: "
        "Unable to enable input on bus 1. Error = %s", GetError(ret).c_str());
      return false;
    }

    enable = 1;
    ret = AudioUnitSetProperty(m_audioUnit,
      kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &enable, sizeof(enable));
    if (ret != noErr)
    {
      CLog::Log(LOGERROR, "CAUOutputDevice::EnableInputOuput:: "
        "Unable to disable output on bus 0. Error = %s", GetError(ret).c_str());
      return false;
    }
  }

  return true;
}

bool CAUOutputDevice::GetPreferredChannelLayout(CCoreAudioChannelLayout& layout)
{
  if (!m_DeviceId)
    return false;

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput; 
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout; 
  if (!AudioObjectHasProperty(m_DeviceId, &propertyAddress)) 
    return false;

  UInt32 propertySize = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_DeviceId, &propertyAddress, 0, NULL, &propertySize); 
  if (ret != noErr)
    CLog::Log(LOGERROR, "CAUOutputDevice::GetPreferredChannelLayout: "
      "Unable to retrieve preferred channel layout size. Error = %s", GetError(ret).c_str());

  void *pBuf = malloc(propertySize);
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, pBuf); 
  if (ret != noErr)
    CLog::Log(LOGERROR, "CAUOutputDevice::GetPreferredChannelLayout: "
      "Unable to retrieve preferred channel layout. Error = %s", GetError(ret).c_str());
  else
  {
    // Copy the result into the caller's instance
    layout.CopyLayout(*((AudioChannelLayout*)pBuf));
  }
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
  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioUnitProperty_MatrixDimensions, kAudioUnitScope_Global, 0, dims, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::Initialize:: "
      "Get matrix dimesion. Error = %s", GetError(ret).c_str());
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
  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &busCount, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetInputBusCount: "
      "Unable to get input bus count. Error = %s", GetError(ret).c_str());
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

  OSStatus ret = AudioUnitSetProperty(m_audioUnit,
    kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &busCount, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetInputBusCount: "
      "Unable to set input bus count. Error = %s", GetError(ret).c_str());
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
  OSStatus ret = AudioUnitGetProperty(m_audioUnit,
    kAudioUnitProperty_ElementCount, kAudioUnitScope_Output, 0, &busCount, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetOutputBusCount: "
      "Unable to get output bus count. Error = %s", GetError(ret).c_str());
    return 0;
  }
  return busCount;
}

bool CAUMatrixMixer::SetOutputBusCount(UInt32 busCount)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetProperty(m_audioUnit,
    kAudioUnitProperty_BusCount, kAudioUnitScope_Output, 0, &busCount, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetOutputBusCount: "
      "Unable to set output bus count. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

Float32 CAUMatrixMixer::GetGlobalVolume()
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 vol = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit,
    kMatrixMixerParam_Volume, kAudioUnitScope_Global, 0xFFFFFFFF, &vol);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetGlobalVolume: "
      "Unable to get global volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return vol;
}

bool CAUMatrixMixer::SetGlobalVolume(Float32 vol)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit,
    kMatrixMixerParam_Volume, kAudioUnitScope_Global, 0xFFFFFFFF, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetGlobalVolume: "
      "Unable to set global volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

Float32 CAUMatrixMixer::GetInputVolume(UInt32 element)
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 vol = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit,
    kMatrixMixerParam_Volume, kAudioUnitScope_Input, element, &vol);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetInputVolume: "
      "Unable to get input volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return vol;
}

bool CAUMatrixMixer::SetInputVolume(UInt32 element, Float32 vol)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit,
    kMatrixMixerParam_Volume, kAudioUnitScope_Input, element, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetInputVolume: "
      "Unable to set input volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

Float32 CAUMatrixMixer::GetOutputVolume(UInt32 element)
{
  if (!m_audioUnit)
    return 0.0f;

  Float32 vol = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_audioUnit,
    kMatrixMixerParam_Volume, kAudioUnitScope_Output, element, &vol);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::GetOutputVolume: "
      "Unable to get output volume. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return vol;
}

bool CAUMatrixMixer::SetOutputVolume(UInt32 element, Float32 vol)
{
  if (!m_audioUnit)
    return false;

  OSStatus ret = AudioUnitSetParameter(m_audioUnit,
    kMatrixMixerParam_Volume, kAudioUnitScope_Output, element, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CAUMatrixMixer::SetOutputVolume: "
      "Unable to set output volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

