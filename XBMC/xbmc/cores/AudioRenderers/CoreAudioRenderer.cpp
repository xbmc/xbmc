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
#include "CoreAudioRenderer.h"

#define LOG_STREAM_DESC(msg, sd) CLog::Log(LOGDEBUG, "%s - SampleRate: %u, BitDepth: %u, Channels: %u",msg, (Uint32) sd.mSampleRate, sd.mBitsPerChannel, sd.mChannelsPerFrame)

//***********************************************************************************************
// Contruction/Destruction
//***********************************************************************************************
CCoreAudioRenderer::CCoreAudioRenderer() :
  m_Pause(false),
  m_ChunkLen(0),
  m_MaxCacheLen(0),
  m_OutputUnit(NULL),
  m_TotalBytesIn(0),
  m_TotalBytesOut(0),
  m_AvgBytesPerSec(0)
{
  
}

CCoreAudioRenderer::~CCoreAudioRenderer()
{
  if (m_OutputUnit)
    Deinitialize();
}

//***********************************************************************************************
// Initialization
//***********************************************************************************************
bool CCoreAudioRenderer::Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  // Re-used vars
  UInt32 size = 0;
  OSStatus ret;
  
  // Obtain a list of all available audio devices
    AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  UInt32 deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  ret = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, pDevices);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to retrieve the list of available devices. ErrCode = 0x%08x", ret);
    return false; 
  }
  
  // Attempt to find the configured output device
  AudioDeviceID outputDevice = 0;
  for (UInt32 dev = 0; dev < deviceCount; dev++)
  {
    size = 0;
    AudioDeviceGetPropertyInfo(pDevices[dev],0, false, kAudioDevicePropertyDeviceName, &size, NULL);
    char* pName = new char[size];
    ret = AudioDeviceGetProperty(pDevices[dev],0, false, kAudioDevicePropertyDeviceName, &size, pName);
    CLog::Log(LOGINFO, "CoreAudioRenderer::Initialize: Found device - %s.", pName);
    // TODO: Compare to configured output device and select it if there is a match
    delete[] pName;
  }
  delete[] pDevices;
  
  if (!outputDevice) // Fall back to the default device if no match is found
  {
    size = sizeof(AudioDeviceID);
    AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &outputDevice);
  }

  // Find the required Output AudioUnit
  ComponentDescription outputCompDesc;
	outputCompDesc.componentType = kAudioUnitType_Output;
	outputCompDesc.componentSubType = kAudioUnitSubType_HALOutput;
	outputCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
	outputCompDesc.componentFlags = 0;
	outputCompDesc.componentFlagsMask = 0;
	Component outputComp = FindNextComponent(NULL, &outputCompDesc);
	if (outputComp == NULL)  // Unable to find the AudioUnit we requested
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to locate Output AudioUnit.");
    return false;
  }
	
  // Create an instance of the AudioUnit Component
  ret = OpenAComponent(outputComp, &m_OutputUnit);
	if (ret) // Unable to open AudioUnit
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to open Output AudioUnit. ErrCode: 0x%08x", ret);
    return false; 
  }  
  
// ** Anytime after this we need to close the component if we fail
  
  // Hook the Ouput AudioUnit to the selected device
  ret = AudioUnitSetProperty(m_OutputUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &outputDevice, sizeof(AudioDeviceID));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to set current AudioUnit device. ErrCode = 0x%08x", ret);
    // TODO: Cleanup
    return false; 
  }
  
  // TODO: Register for notification of device disconnects
  // TODO: Register for notification of property changes
  
  // Setup the callback function that the head AudioUnit will use to request data	
  AURenderCallbackStruct callbackInfo;
	callbackInfo.inputProc = CCoreAudioRenderer::RenderCallback; // Function to be called each time the AudioUnit needs data
	callbackInfo.inputProcRefCon = this; // Pointer to be returned in the callback proc
	ret = AudioUnitSetProperty (m_OutputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callbackInfo, sizeof(AURenderCallbackStruct));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to set callback property. ErrCode = 0x%08x",ret);
    // TODO: Cleanup
    return false; 
  }  
  
  // Set the quality of the output converter (Higher quality conversion requires more CPU
  UInt32 convQuality = kRenderQuality_Max; // We want the best
  ret = AudioUnitSetProperty(m_OutputUnit, kAudioUnitProperty_RenderQuality, kAudioUnitScope_Global , 0, &convQuality, sizeof(Uint32));
  
  // Set the input stream format for the AudioUnit (this is what is being sent to us)
  m_InputDesc.mFormatID = kAudioFormatLinearPCM;			      //	Data encoding format
  m_InputDesc.mFormatFlags = kAudioFormatFlagsNativeEndian
                            | kLinearPCMFormatFlagIsPacked  // Samples occupy all bits (not left or right aligned)
                            | kAudioFormatFlagIsSignedInteger;
  m_InputDesc.mChannelsPerFrame = iChannels;                // Number of interleaved audiochannels
  m_InputDesc.mSampleRate = (Float64)uiSamplesPerSec;       //	the sample rate of the audio stream
  m_InputDesc.mBitsPerChannel = uiBitsPerSample;            // Number of bits per sample, per channel
  m_InputDesc.mBytesPerFrame = (uiBitsPerSample>>3) * iChannels; // Size of a frame == 1 sample per channel		
  m_InputDesc.mFramesPerPacket = 1;                         // The smallest amount of indivisible data. Always 1 for uncompressed audio 	
  m_InputDesc.mBytesPerPacket = m_InputDesc.mBytesPerFrame * m_InputDesc.mFramesPerPacket; 
  ret = AudioUnitSetProperty (m_OutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &m_InputDesc, sizeof(AudioStreamBasicDescription));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to set Format Conversion AudioUnit input format. ErrCode = 0x%08x", ret);
    // TODO: Cleanup
    return false; 
  }
  
  // Configure the maximum number of frames that the AudioUnit will ask to process at one time. The reliability of this is questionable.
  UInt32 bufferFrames = GetAUPropUInt32(m_OutputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input) * 2; // This will return the configured buffer size of the associated output device
  ret = AudioUnitSetProperty (m_OutputUnit, kAudioUnitProperty_MaximumFramesPerSlice , kAudioUnitScope_Global, 0, &bufferFrames, sizeof(UInt32));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to set Format Conversion AudioUnit slice size. ErrCode = 0x%08x", ret);
    // TODO: Cleanup
    return false; 
  }
  
  // Initialize the Output AudioUnit
  ret = AudioUnitInitialize(m_OutputUnit);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CoreAudioRenderer::Initialize: Unable to Initialize Output AudioUnit. ErrCode = 0x%08x", ret);
    // TODO: Cleanup
    return false; 
  }  

  // Finish configuring the renderer
  m_AvgBytesPerSec = m_InputDesc.mSampleRate * m_InputDesc.mBytesPerFrame;  // 1 sample per channel per frame
  m_ChunkLen = bufferFrames * m_InputDesc.mBytesPerFrame;                   // This is the minimum amount of data that we will accept from a client
  m_MaxCacheLen = m_AvgBytesPerSec * 4;                                         // Set the max cache size to 1 second of data. TODO: Make this more intelligent

  m_Pause = true; // We will resume playback once we have some data.
  
  AudioStreamBasicDescription inputDesc_end, outputDesc_end;
  size = sizeof(AudioStreamBasicDescription);
  AudioUnitGetProperty(m_OutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &inputDesc_end, &size);
  AudioUnitGetProperty(m_OutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outputDesc_end, &size);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Renderer Configuration - Chunk Len: %u, Max Cache: %u.", m_ChunkLen, m_MaxCacheLen);
  LOG_STREAM_DESC("CoreAudioRenderer::Initialize: Input Stream Format", inputDesc_end);
  LOG_STREAM_DESC("CoreAudioRenderer::Initialize: Output Stream Format", outputDesc_end);  
  CLog::Log(LOGINFO, "CoreAudioRenderer::Initialize: Successfully configured audio output.");
  
  return true;
}

HRESULT CCoreAudioRenderer::Deinitialize()
{
  Stop();

  m_ChunkLen = 0;
  m_MaxCacheLen = 0;
  m_AvgBytesPerSec = 0;
  
  if (m_OutputUnit)
  {
    AudioUnitUninitialize (m_OutputUnit);
    CloseComponent(m_OutputUnit);
    m_OutputUnit = NULL;
  }
  CLog::Log(LOGINFO, "CoreAudioRenderer::Deinitialize: Renderer has been shut down.");

  return S_OK;
}

//***********************************************************************************************
// Transport control methods
//***********************************************************************************************
HRESULT CCoreAudioRenderer::Pause()
{
  //TODO: Query the state of the AU?
  if (!m_Pause && m_OutputUnit)
  {
    AudioOutputUnitStop(m_OutputUnit);
    m_Pause = true;
  }
  
  return S_OK;
}

HRESULT CCoreAudioRenderer::Resume()
{
  //TODO: Query the state of the AU?
  if (m_Pause && m_OutputUnit)
  {
    CLog::Log(LOGINFO, "CoreAudioRenderer::Resume: Resuming Playback.");
    AudioOutputUnitStart(m_OutputUnit);
    m_Pause = false;
  }

  return S_OK; // Not really much to be done by returning an error
}

HRESULT CCoreAudioRenderer::Stop()
{
  //TODO: Query the state of the AU?
  if (m_OutputUnit)
    AudioOutputUnitStop(m_OutputUnit);

  m_Pause = true;
  m_TotalBytesIn = 0;
  m_TotalBytesOut = 0;
  m_Cache.Clear();

  return S_OK;
}

//***********************************************************************************************
// Volume control methods
//***********************************************************************************************
LONG CCoreAudioRenderer::GetMinimumVolume() const
{
  return 0;
}

LONG CCoreAudioRenderer::GetMaximumVolume() const
{
  return -6000;
}

LONG CCoreAudioRenderer::GetCurrentVolume() const
{
  Float32 volPct = 0.0f;
  if (noErr == AudioUnitGetParameter(m_OutputUnit,  kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &volPct))
    return (LONG)(volPct * 6000) - 6000L;
  
  return 0; // Not sure what else to do here
}

void CCoreAudioRenderer::Mute(bool bMute)
{
  SetCurrentVolume(0);
}

HRESULT CCoreAudioRenderer::SetCurrentVolume(LONG nVolume)
{
  if (!m_OutputUnit)
    return E_FAIL;
  
  // Scale the provided value to a range of 0 -> 1
  Float32 volPct = (Float32)(nVolume + 6000.0f)/6000.0f;
  
  // Try to set the volume. If it fails there is not a lot to be done (may be an encoded stream).
  AudioUnitSetParameter(m_OutputUnit, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, volPct, 0);
  
  return S_OK;
}

//***********************************************************************************************
// Data management methods
//***********************************************************************************************
DWORD CCoreAudioRenderer::GetSpace()
{
  return m_MaxCacheLen - m_Cache.GetTotalBytes(); // This is just a guess, as the output is probably pulling data.
}

DWORD CCoreAudioRenderer::AddPackets(unsigned char *data, DWORD len)
{
  // Require at least one 'chunk'. This allows us at least some measure of control over efficiency
  if (len < m_ChunkLen ||  m_Cache.GetTotalBytes() >= m_MaxCacheLen) // No room at the inn
    return 0;

  size_t bytesUsed = len;
  DWORD cacheSpace = GetSpace();
  
  if (bytesUsed > cacheSpace)
    return 0; // Wait until we can accept all of it
  
  m_Cache.AddData(data, bytesUsed);
        
  // Update tracking varss
  m_TotalBytesIn += bytesUsed;
  
  // We have some data. Attmept to resume playback
  Resume();
  
  return bytesUsed; // Number of bytes added to cache;
}

FLOAT CCoreAudioRenderer::GetDelay()
{
  // TODO: Obtain hardware/os latency from the AU?
  float delay =  (float)m_Cache.GetTotalBytes()/(float)m_AvgBytesPerSec;
  return delay;
}

DWORD CCoreAudioRenderer::GetChunkLen()
{
  return m_ChunkLen;
}

void CCoreAudioRenderer::WaitCompletion()
{
  // TODO: Implement
  Stop();
}

//***********************************************************************************************
// Private Methods
//***********************************************************************************************
OSStatus CCoreAudioRenderer::OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  // TODO; !!THIS IS NOT THREAD SAFE!! FIX ME!!
  // TODO: Replace statics with performance counter class
  static UInt32 lastUpdateTime = 0;
  static Uint64 lastTotalBytes = 0;
  
  if (lastUpdateTime == 0)
    lastUpdateTime = timeGetTime();
  
  if (lastTotalBytes == 0)
    lastTotalBytes = m_TotalBytesOut;
  
  Uint32 bytesRequested = m_InputDesc.mBytesPerFrame * inNumberFrames;
  if (bytesRequested > ioData->mBuffers[0].mDataByteSize)
  {
    CLog::Log(LOGERROR, "Supplied buffer is too small(%u) to accept requested sample data(%u). Truncating data", ioData->mBuffers[0].mDataByteSize, bytesRequested);
    bytesRequested = ioData->mBuffers[0].mDataByteSize;
    // TODO: Pull extra bytes from cache to keep time
  }
  
  if (bytesRequested / m_InputDesc.mBytesPerFrame != inNumberFrames)
    CLog::Log(LOGDEBUG, "Bufeer is larger than frames requested");
    
  if (m_Cache.GetTotalBytes() < bytesRequested) // Not enough data to satisfy the request
  {
    Pause(); // Stop further requests until we have more data.  The AddPackets method will resume playback
    ioData->mBuffers[0].mDataByteSize = 0; // Is this the proper way to signal an underrun to the output?
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Buffer underrun.");
  }
  else
  {
    // Fetch the requested amount of data from our cache
    m_Cache.GetData(ioData->mBuffers[0].mData, bytesRequested);

    // Calculate stats and perform a sanity check
    m_TotalBytesOut += bytesRequested;    
    size_t cacheLen = m_Cache.GetTotalBytes();
    Uint32 cacheCalc = m_TotalBytesIn - m_TotalBytesOut;
    if (cacheCalc != cacheLen)
      CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Cache length mismatch. We seem to have lost some data. Calc = %u, Act = %u.",cacheCalc, cacheLen);

    Uint32 time = timeGetTime();
    Uint32 deltaTime = time - lastUpdateTime;
    if (deltaTime > 1000)// Spit out some summary information every 10 seconds
    {
      CLog::Log(LOGDEBUG, "CCoreAudioRenderer: Summary - Sent %u bytes in %u ms. %0.2f frames / sec. Cache Len = %u.",(Uint32)(m_TotalBytesOut - lastTotalBytes), deltaTime, 1000*((float)(m_TotalBytesOut - lastTotalBytes) / (float)m_InputDesc.mBytesPerFrame)/(float)deltaTime, cacheLen);
      lastUpdateTime = time;
      lastTotalBytes = m_TotalBytesOut;
    }
  }
  
  return noErr;
}

// Static Callback from Output AudioUnit 
OSStatus CCoreAudioRenderer::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  return ((CCoreAudioRenderer*)inRefCon)->OnRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}
#endif


