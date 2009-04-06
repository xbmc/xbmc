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

#define PROFILE_ATOMIC
#define MAX_SPIN 1000

#include "stdafx.h"
#include "CoreAudioRenderer.h"
#include <Atomics.h>
#include <Settings.h>

#define LOG_STREAM_DESC(msg, sd) CLog::Log(LOGDEBUG, "%s - SampleRate: %u, BitDepth: %u, Channels: %u",msg, (UInt32) sd.mSampleRate, sd.mBitsPerChannel, sd.mChannelsPerFrame)

//***********************************************************************************************
// Contruction/Destruction
//***********************************************************************************************
CCoreAudioRenderer::CCoreAudioRenderer() :
  m_Pause(false),
  m_ChunkLen(0),
  m_MaxCacheLen(0),
  m_TotalBytesIn(0),
  m_TotalBytesOut(0),
  m_AvgBytesPerSec(0),
  m_CurrentVolume(0),
  m_Initialized(false)
{
  
}

CCoreAudioRenderer::~CCoreAudioRenderer()
{
    Deinitialize();
}

//***********************************************************************************************
// Initialization
//***********************************************************************************************
bool CCoreAudioRenderer::Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{  
  if (m_Initialized)
    Deinitialize();
  
  // Attempt to find the configured output device
  CCoreAudioHardware audioHardware;
  AudioDeviceID outputDevice = audioHardware.FindAudioDevice(g_guiSettings.GetString("audiooutput.audiodevice"));
  if (!outputDevice) // Fall back to the default device if no match is found
  {
    CLog::Log(LOGWARNING, "CoreAudioRenderer::Initialize: Unable to locate configured device, falling-back to the system default.");
    outputDevice = audioHardware.GetDefaultOutputDevice();
    if (!outputDevice) // Not a lot to be done with no device. TODO: Should we just grab the first existing device?
      return false;
  }
  m_AudioDevice.Open(outputDevice);

  // Create the Output AudioUnit
  ComponentDescription outputCompDesc;
	outputCompDesc.componentType = kAudioUnitType_Output;
	outputCompDesc.componentSubType = kAudioUnitSubType_HALOutput;
	outputCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
	outputCompDesc.componentFlags = 0;
	outputCompDesc.componentFlagsMask = 0;
  
  if (!m_AudioUnit.Open(outputCompDesc))
    return false;

  // Now we split based on the output type (PCM/SPDIF)
  if (bPassthrough)
    InitializeEncoded(outputDevice);
  else
  {
    // Hook the Ouput AudioUnit to the selected device
    // TODO: Move back out when PassthroughAudioUnit is done...
    if (!m_AudioUnit.SetCurrentDevice(outputDevice))
      return false;    
    InitializePCM(iChannels, uiSamplesPerSec, uiBitsPerSample);
  } 
  
  // Initialize the Output AudioUnit
 if (!m_AudioUnit.Initialize())
   return false;

  // Finish configuring the renderer
  m_MaxCacheLen = m_AvgBytesPerSec;     // Set the max cache size to 1 second of data. TODO: Make this more intelligent
  m_Pause = true;                       // We will resume playback once we have some data.
  
  // Generate some log entries
  AudioStreamBasicDescription inputDesc_end, outputDesc_end;
  m_AudioUnit.GetInputFormat(&inputDesc_end);
  m_AudioUnit.GetOutputFormat(&outputDesc_end);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Renderer Configuration - Chunk Len: %u, Max Cache: %u.", m_ChunkLen, m_MaxCacheLen);
  LOG_STREAM_DESC("CoreAudioRenderer::Initialize: Input Stream Format", inputDesc_end);
  LOG_STREAM_DESC("CoreAudioRenderer::Initialize: Output Stream Format", outputDesc_end);  
  CLog::Log(LOGINFO, "CoreAudioRenderer::Initialize: Successfully configured audio output.");
  
  m_Initialized = true;
  return true;
}

HRESULT CCoreAudioRenderer::Deinitialize()
{
  if (!m_Initialized)
    return S_OK; // Not really a failure...
  
  Stop();

  m_ChunkLen = 0;
  m_MaxCacheLen = 0;
  m_AvgBytesPerSec = 0;
  
  m_AudioUnit.Close();
  m_AudioDevice.Close();
  m_OutputStream.Close();
  
  CLog::Log(LOGINFO, "CoreAudioRenderer::Deinitialize: Renderer has been shut down.");

  m_Initialized = false;
  return S_OK;
}

//***********************************************************************************************
// Transport control methods
//***********************************************************************************************
HRESULT CCoreAudioRenderer::Pause()
{
  if (!m_Pause)
  {
    m_AudioUnit.Stop();
    m_Pause = true;
  }
  
  return S_OK;
}

HRESULT CCoreAudioRenderer::Resume()
{
  if (m_Pause)
  {
    CLog::Log(LOGINFO, "CoreAudioRenderer::Resume: Resuming Playback.");
    m_AudioUnit.Start();
    m_Pause = false;
  }

  return S_OK; // Not really much to be done by returning an error
}

HRESULT CCoreAudioRenderer::Stop()
{
  m_AudioUnit.Stop();

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
  return m_CurrentVolume;
}

void CCoreAudioRenderer::Mute(bool bMute)
{
  SetCurrentVolume(0);
}

HRESULT CCoreAudioRenderer::SetCurrentVolume(LONG nVolume)
{  
  // Scale the provided value to a range of 0 -> 1
  Float32 volPct = (Float32)(nVolume + 6000.0f)/6000.0f;
  
  // Try to set the volume. If it fails there is not a lot to be done (may be an encoded stream).
  if (!m_AudioUnit.SetCurrentVolume(volPct))
    return E_FAIL;
  
  m_CurrentVolume = nVolume;
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

  DWORD cacheSpace = GetSpace();  
  if (len > cacheSpace)
    return 0; // Wait until we can accept all of it
  
  size_t bytesUsed = len;
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
  // TODO: Replace statics with performance counter class
  static UInt32 lastUpdateTime = 0;
  static UInt64 lastTotalBytes = 0;
  
  UInt32 profileTime[3] = {0};
  UInt32 time = timeGetTime();
  profileTime[0] = time;

  if (lastUpdateTime == 0)
    lastUpdateTime = time;
  
  if (lastTotalBytes == 0)
    lastTotalBytes = m_TotalBytesOut;
  UInt32 bytesRequested = m_BytesPerFrame * inNumberFrames;
  if (bytesRequested > ioData->mBuffers[0].mDataByteSize)
  {
    CLog::Log(LOGERROR, "Supplied buffer is too small(%u) to accept requested sample data(%u). Truncating data", ioData->mBuffers[0].mDataByteSize, bytesRequested);
    bytesRequested = ioData->mBuffers[0].mDataByteSize;
    // TODO: Pull extra bytes from cache to keep time
  }
  
  if (bytesRequested / m_BytesPerFrame != inNumberFrames)
    CLog::Log(LOGDEBUG, "Buffer is larger than frames requested");
    
  if (m_Cache.GetTotalBytes() < bytesRequested) // Not enough data to satisfy the request
  {
    profileTime[1] = timeGetTime();
    Pause(); // Stop further requests until we have more data.  The AddPackets method will resume playback
    ioData->mBuffers[0].mDataByteSize = 0; // Is this the proper way to signal an underrun to the output?
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Buffer underrun.");
  }
  else
  {
    profileTime[1] = timeGetTime();
    // Fetch the requested amount of data from our cache
    m_Cache.GetData(ioData->mBuffers[0].mData, bytesRequested);
    // Calculate stats and perform a sanity check
    m_TotalBytesOut += bytesRequested;    
    size_t cacheLen = m_Cache.GetTotalBytes();
    UInt32 cacheCalc = m_TotalBytesIn - m_TotalBytesOut;
    if (cacheCalc != cacheLen)
      CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Cache length mismatch. We seem to have lost some data. Calc = %u, Act = %u.",cacheCalc, cacheLen);
    profileTime[2] = timeGetTime();
    UInt32 deltaTime = time - lastUpdateTime; 
    if (deltaTime > 1000) // Check our sanity once a second
    {
      UInt32 outgoingBitrate = (m_TotalBytesOut - lastTotalBytes) / ((float)deltaTime/1000.0f);
      if (outgoingBitrate < 0.99 * m_AvgBytesPerSec && m_TotalBytesOut > m_AvgBytesPerSec * 2) // Check if CoreAudio is behind (ignore the first 2 seconds)
        CLog::Log(LOGWARNING, "CCoreAudioRenderer::OnRender: Outgoing bitrate is lagging. Target: %u, Actual: %u. deltaTime was %u", m_AvgBytesPerSec, outgoingBitrate, deltaTime);
      //CLog::Log(LOGDEBUG, "CCoreAudioRenderer: Summary - Sent %u bytes in %u ms. %0.2f frames / sec. Cache Len = %u.",(UInt32)(m_TotalBytesOut - lastTotalBytes), deltaTime, 1000*((float)(m_TotalBytesOut - lastTotalBytes) / (float)m_BytesPerFrame)/(float)deltaTime, cacheLen);
      lastUpdateTime = time;
      lastTotalBytes = m_TotalBytesOut;
    }
  }
  unsigned int t[2];
  t[0] = profileTime[1] - profileTime[0];
  t[1] = profileTime[2] - profileTime[2];
  if (t[0] > 1 || t[1] > 1)
    CLog::Log(LOGDEBUG, "CCoreAudioRenderer::OnRender: Profile times %u %u.", t[0], t[1]);  
  
  return noErr;
}

// Static Callback from Output AudioUnit 
OSStatus CCoreAudioRenderer::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  return ((CCoreAudioRenderer*)inRefCon)->OnRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

// Static Callback from AudioDevice
OSStatus CCoreAudioRenderer::PassthroughRenderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData)
{
  return ((CCoreAudioRenderer*)inClientData)->OnRender(NULL, inInputTime, 0, 1536, outOutputData);
}


bool CCoreAudioRenderer::InitializePCM(UInt32 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample)
{  
    // Setup the callback function that the head AudioUnit will use to request data	
  if (!m_AudioUnit.SetRenderProc(CCoreAudioRenderer::RenderCallback, this))
    return false;
    
  // Set the input stream format for the AudioUnit (this is what is being sent to us)
  // TODO: Get rid of the m_InputDesc member
  AudioStreamBasicDescription inputFormat;
  inputFormat.mFormatID = kAudioFormatLinearPCM;			      //	Data encoding format
  inputFormat.mFormatFlags = kAudioFormatFlagsNativeEndian
  | kLinearPCMFormatFlagIsPacked  // Samples occupy all bits (not left or right aligned)
  | kAudioFormatFlagIsSignedInteger;
  inputFormat.mChannelsPerFrame = channels;                // Number of interleaved audiochannels
  inputFormat.mSampleRate = (Float64)samplesPerSecond;       //	the sample rate of the audio stream
  inputFormat.mBitsPerChannel = bitsPerSample;            // Number of bits per sample, per channel
  inputFormat.mBytesPerFrame = (bitsPerSample>>3) * channels; // Size of a frame == 1 sample per channel		
  inputFormat.mFramesPerPacket = 1;                         // The smallest amount of indivisible data. Always 1 for uncompressed audio 	
  inputFormat.mBytesPerPacket = inputFormat.mBytesPerFrame * inputFormat.mFramesPerPacket; 
  if (!m_AudioUnit.SetInputFormat(&inputFormat))
    return false;
  
  // Configure the maximum number of frames that the AudioUnit will ask to process at one time. The reliability of this is questionable.
  UInt32 bufferFrames = m_AudioUnit.GetBufferFrameSize(); // This will return the configured buffer size of the associated output device
  if (!m_AudioUnit.SetMaxFramesPerSlice(bufferFrames))
    return false;
  
  m_ChunkLen = bufferFrames * inputFormat.mBytesPerFrame;                       // This is the minimum amount of data that we will accept from a client
  m_AvgBytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame;      // 1 sample per channel per frame
  m_BytesPerFrame = inputFormat.mBytesPerFrame;
  
  return true;
}

bool CCoreAudioRenderer::InitializeEncoded(AudioDeviceID outputDevice)
{
  CStdString formatString;
  UInt32 framesPerSlice = 0;
    
  // Fetch a list of the streams defined by the output device
  AudioStreamIdList streams;
  m_AudioDevice.GetStreams(&streams);
  while (!streams.empty())
  {
    // Get the next stream
    CCoreAudioStream stream;
    stream.Open(streams.front());
    streams.pop_front(); // We copied it, now we are done with it
    
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: Found %s stream - id: 0x%04X, Terminal Type: 0x%04X", 
              stream.GetDirection() ? "Input" : "Output", 
              stream.GetId(), 
              stream.GetTerminalType());
    
    // TODO: How do we determine if this is the right stream to use?
    
    AudioStreamBasicDescription outputFormat = {0};
    
    // Available Virtual (IOProc) formats
    // TODO: Deal with sample rate ranges
    StreamFormatList virtualFormats;
    stream.GetAvailableVirtualFormats(&virtualFormats);
    while (!virtualFormats.empty())
    {
      AudioStreamRangedDescription& desc = virtualFormats.front();
      
      if (desc.mFormat.mFormatID == kAudioFormat60958AC3 && desc.mFormat.mSampleRate == 48000)
        outputFormat = desc.mFormat; // Select this format

      StreamDescriptionToString(desc.mFormat, formatString);
      CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded:     Virtual Format - %s", formatString.c_str());
      
      virtualFormats.pop_front(); // We're done with it (copied if necessary)
    }
    
    if (outputFormat.mFormatID) // Found one
    {
      m_OutputStream.Open(stream.GetId()); // This is the one we will keep
      m_OutputStream.SetVirtualFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
      
      m_ChunkLen = outputFormat.mBytesPerPacket; // 1 Chunk == 1 Packet
      framesPerSlice = outputFormat.mFramesPerPacket; // 1 Slice == 1 Packet == 1 Chunk
      m_AvgBytesPerSec = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3) * outputFormat.mSampleRate; // TODO: Is this correct?
      m_BytesPerFrame = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3); // Should always be 4
      break; // We found a suitable stream/format combination
    }
  }

  if (!m_OutputStream.GetId()) // No suitable stream was found
    return false;
  
  // Try to disable mixing. If we cannot, it may not be a problem
  m_AudioDevice.SetMixingSupport(false);

  //AudioDeviceAddIOProc(outputDevice, PassthroughRenderCallback, this);
  
  // If all else was successful, hog the output device
  // TODO: Is failure here critical?
  if (!m_AudioDevice.SetHogStatus(true))
    return false;
  
  return true;
}
                                
/***************************Core Audio Helper Methods*************************************/

#endif


