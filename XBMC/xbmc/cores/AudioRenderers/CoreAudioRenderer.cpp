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
#include <Atomics.h>
#include <Settings.h>

// TODO: Dynamically allocate this
short g_SampleBuffer[10000] = {0};
static const float short_to_float_mult = 1.0f / 32768.f;
void ShortToFloat(short* pIn, float* pOut, size_t count)
{
  while (count--)
  {
    float samp = *pIn * short_to_float_mult;
    *pOut = samp;
    pIn++;
    pOut++;
  }
}

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
  m_Initialized(false),
  m_Passthrough(false),
  m_PassthroughSpoof(false),
  m_OutputBufferIndex(0)
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
  if (m_Initialized) // Have to clean house before we start again. TODO: Should we return failure instead?
    Deinitialize();
  
  // TODO: If debugging, output information about all devices/streams
  
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
  
  // TODO: Determine if the device is in-use/locked by another process.
  
  // Attach our output object to the device
  m_AudioDevice.Open(outputDevice);

  // If this is a passthrough (AC3/DTS) stream, attempt to handle it natively
  if (bPassthrough)
    m_Passthrough = InitializeEncoded(outputDevice);

  // If this is a PCM stream, or we failed to handle a passthrough stream natively, 
  // prepare for standard interleaved PCM data
  if (!m_Passthrough)
  {
    // Create the Output AudioUnit Component
    ComponentDescription outputCompDesc;
    outputCompDesc.componentType = kAudioUnitType_Output;
    outputCompDesc.componentSubType = kAudioUnitSubType_HALOutput;
    outputCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    outputCompDesc.componentFlags = 0;
    outputCompDesc.componentFlagsMask = 0;
    
    if (!m_AudioUnit.Open(outputCompDesc))
      return false;

    // Hook the Ouput AudioUnit to the selected device
    if (!m_AudioUnit.SetCurrentDevice(outputDevice))
      return false;
    
    // If we are here and this is a passthrough stream, native handling failed.
    // Try to handle it as IEC61937 PCM data (DD-Wav)
    bool configured = false;
    if (bPassthrough)
    {
      CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: No suitable AC3 output format found. Attempting DD-Wav.");
      configured = SpoofPCM();
    }
    else // Standard PCM data
      configured = InitializePCM(iChannels, uiSamplesPerSec, uiBitsPerSample);
    
    if (!configured) // No suitable output format was able to be configured
      return false;
    
    // Configure the maximum number of frames that the AudioUnit will ask to process at one time.
    // If this is not called, there is no guarantee that the callback will ever be called.
    UInt32 bufferFrames = m_AudioUnit.GetBufferFrameSize(); // Size of the output buffer, in Frames
    if (!m_AudioUnit.SetMaxFramesPerSlice(bufferFrames))
      return false;    
    
    m_ChunkLen = bufferFrames * m_BytesPerFrame;                       // This is the minimum amount of data that we will accept from a client

    // Setup the callback function that the AudioUnit will use to request data	
    if (!m_AudioUnit.SetRenderProc(CCoreAudioRenderer::RenderCallback, this))
      return false;
    
    // Initialize the Output AudioUnit
    if (!m_AudioUnit.Initialize())
      return false;

    // Log some information about the stream
    AudioStreamBasicDescription inputDesc_end, outputDesc_end;
    CStdString formatString;
    m_AudioUnit.GetInputFormat(&inputDesc_end);
    m_AudioUnit.GetOutputFormat(&outputDesc_end);
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Renderer Configuration - Chunk Len: %u, Max Cache: %u (%0.0fms).", m_ChunkLen, m_MaxCacheLen, 1000.0 *(float)m_MaxCacheLen/(float)m_AvgBytesPerSec);
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Input Stream Format %s", StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Output Stream Format % s", StreamDescriptionToString(outputDesc_end, formatString));    
  }
  
  m_MaxCacheLen = m_AvgBytesPerSec;     // Set the max cache size to 1 second of data. TODO: Make this more intelligent
  m_Pause = true;                       // Suspend rendering. We will start once we have some data.
  m_Initialized = true;
  
  CLog::Log(LOGINFO, "CoreAudioRenderer::Initialize: Successfully configured audio output.");  
  return true;
}

HRESULT CCoreAudioRenderer::Deinitialize()
{
  if (!m_Initialized)
    return S_OK; // Not really a failure...

  // Stop rendering
  Stop();
  // Reset our state
  m_ChunkLen = 0;
  m_MaxCacheLen = 0;
  m_AvgBytesPerSec = 0;
  if (m_Passthrough)
    m_AudioDevice.RemoveIOProc();
  m_AudioUnit.Close();
  m_AudioDevice.Close();
  m_OutputStream.Close();
  m_Initialized = false;
  
  CLog::Log(LOGINFO, "CoreAudioRenderer::Deinitialize: Renderer has been shut down.");
  return S_OK;
}

//***********************************************************************************************
// Transport control methods
//***********************************************************************************************
HRESULT CCoreAudioRenderer::Pause()
{
  if (!m_Pause)
  {
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Pause: Pausing Playback.");
    if (m_Passthrough)
      m_AudioDevice.Stop();
    else
      m_AudioUnit.Stop();
    m_Pause = true;
  }
  
  return S_OK;
}

HRESULT CCoreAudioRenderer::Resume()
{
  if (m_Pause)
  {
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Resume: Resuming Playback.");
    if (m_Passthrough)
      m_AudioDevice.Start();
    else
     m_AudioUnit.Start();
    m_Pause = false;
  }
  return S_OK;
}

HRESULT CCoreAudioRenderer::Stop()
{
  if (m_Passthrough)
    m_AudioDevice.Stop();
  else
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
  if (bMute)
    SetCurrentVolume(0);
  else
    SetCurrentVolume(m_CurrentVolume);
}

HRESULT CCoreAudioRenderer::SetCurrentVolume(LONG nVolume)
{  
  if (m_Passthrough || m_PassthroughSpoof)
    return S_OK; // Don't change, but don't complain...
  
  // Scale the provided value to a range of 0.0 -> 1.0
  Float32 volPct = (Float32)(nVolume + 6000.0f)/6000.0f;
  
  // Try to set the volume. If it fails there is not a lot to be done.
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
  return m_MaxCacheLen - m_Cache.GetTotalBytes(); // This is just an estimate, since the driver is asynchonously pulling data.
}

DWORD CCoreAudioRenderer::AddPackets(unsigned char *data, DWORD len)
{  
  // Require at least one 'chunk'. This allows us at least some measure of control over efficiency
  if (len < m_ChunkLen ||  m_Cache.GetTotalBytes() >= m_MaxCacheLen)
    return 0;

  DWORD cacheSpace = GetSpace();  
  if (len > cacheSpace)
    return 0; // Wait until we can accept all of it
  
  size_t bytesUsed = m_Cache.AddData(data, len);
  
  // Update tracking variable
  // TODO: Add to performance class/struct
  m_TotalBytesIn += bytesUsed;
  
  Resume();  // We have some data. Attmept to resume playback
  
  return bytesUsed; // Number of bytes added to cache;
}

FLOAT CCoreAudioRenderer::GetDelay()
{
  // Calculate the duration of the data in the cache
  float delay = (float)m_Cache.GetTotalBytes()/(float)m_AvgBytesPerSec;
  // TODO: Obtain hardware/os latency for better accuracy
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
  // TODO: May need to remove all logging from this method since it is called from a realtime thread
  if (!m_Initialized)
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Callback to de/unitialized renderer.");
              
  // Make a local copy of the buffer index
  UInt32 buf = m_OutputBufferIndex; // This determines which device stream we send our data to.
  
  // TODO: Replace statics with performance counter class
  static UInt32 lastUpdateTime = 0;
  static UInt64 lastTotalBytes = 0;
  
  // TODO: Remove timing instrumentation
  UInt32 profileTime[3] = {0};
  UInt32 time = timeGetTime();
  profileTime[0] = time;

  if (lastUpdateTime == 0)
    lastUpdateTime = time;
  if (lastTotalBytes == 0)
    lastTotalBytes = m_TotalBytesOut;
  
  if (m_PassthroughSpoof) // Adjust frame count to line up with data format
    inNumberFrames *= 2;
  
  UInt32 bytesRequested = m_BytesPerFrame * inNumberFrames;
  if (bytesRequested > ioData->mBuffers[buf].mDataByteSize)
  {
    bytesRequested = ioData->mBuffers[buf].mDataByteSize;
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Supplied buffer is too small(%u) to accept requested sample data(%u). Truncating data", ioData->mBuffers[0].mDataByteSize, bytesRequested);
  }
    
  if (m_Cache.GetTotalBytes() < bytesRequested) // Not enough data to satisfy the request
  {
    profileTime[1] = timeGetTime();
    Pause(); // Stop further requests until we have more data.  The AddPackets method will resume playback
    memset(ioData->mBuffers[buf].mData, 0, ioData->mBuffers[buf].mDataByteSize);  // Return only silence
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Buffer underrun.");
  }
  else // Fetch some data for the caller
  {
    profileTime[1] = timeGetTime();
     if (m_PassthroughSpoof) // Need to convert the data
    {
      bytesRequested /= 2; // 16 to 32 bit (this will double when we output)
      m_Cache.GetData(g_SampleBuffer, bytesRequested); // Retrieve data from queue (16-bit Signed Integer)
      ShortToFloat(g_SampleBuffer, (float*)ioData->mBuffers[buf].mData, inNumberFrames); // Convert to 32-bit float (double the effective data size)
    }
    else // Copy data as-is
    {
      m_Cache.GetData(ioData->mBuffers[buf].mData, bytesRequested);
    }
    
    // Calculate stats and perform a sanity check
    // TODO: Either remove all of this or optimize it
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

// Static Callback from AudioUnit 
OSStatus CCoreAudioRenderer::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  return ((CCoreAudioRenderer*)inRefCon)->OnRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

// Static Callback from AudioDevice
OSStatus CCoreAudioRenderer::PassthroughRenderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData)
{
  CCoreAudioRenderer* pThis = (CCoreAudioRenderer*)inClientData;
  return pThis->OnRender(NULL, inInputTime, 0, outOutputData->mBuffers[0].mDataByteSize / pThis->m_BytesPerFrame, outOutputData);
}

bool CCoreAudioRenderer::InitializePCM(UInt32 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample)
{  
  // Set the input stream format for the AudioUnit (this is what is being sent to us)
  AudioStreamBasicDescription inputFormat;
  inputFormat.mFormatID = kAudioFormatLinearPCM;			      //	Data encoding format
  inputFormat.mFormatFlags = kAudioFormatFlagsNativeEndian
                           | kLinearPCMFormatFlagIsPacked   // Samples occupy all bits (not left or right aligned)
                           | kAudioFormatFlagIsSignedInteger;
  inputFormat.mChannelsPerFrame = channels;                 // Number of interleaved audiochannels
  inputFormat.mSampleRate = (Float64)samplesPerSecond;      //	the sample rate of the audio stream
  inputFormat.mBitsPerChannel = bitsPerSample;              // Number of bits per sample, per channel
  inputFormat.mBytesPerFrame = (bitsPerSample>>3) * channels; // Size of a frame == 1 sample per channel		
  inputFormat.mFramesPerPacket = 1;                         // The smallest amount of indivisible data. Always 1 for uncompressed audio 	
  inputFormat.mBytesPerPacket = inputFormat.mBytesPerFrame * inputFormat.mFramesPerPacket; 
  if (!m_AudioUnit.SetInputFormat(&inputFormat))
    return false;

  // TODO: Handle channel mapping
  
  m_BytesPerFrame = inputFormat.mBytesPerFrame;
  m_AvgBytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame / 2;      // 1 sample per channel per frame
  
  return true;
}

bool CCoreAudioRenderer::SpoofPCM()
{
  m_AudioDevice.SetNominalSampleRate(48000.0f);
  
  m_AudioDevice.SetHogStatus(true); // Prevent any other application from using this device.
  m_AudioDevice.SetMixingSupport(false); // Try to disable mixing support. Effectiveness depends on the device.
  
  // Set the input stream format for the AudioUnit. We will convert the data to this format before passing it to the AudioUnit.
  AudioStreamBasicDescription inputFormat;
  inputFormat.mFormatID = kAudioFormatLinearPCM;			      //	Data encoding format
  inputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked; // 32-bit float
  inputFormat.mChannelsPerFrame = 2;       // Number of interleaved audiochannels
  inputFormat.mSampleRate = 48000.0;       //	the sample rate of the audio stream
  inputFormat.mBitsPerChannel = 32;        // Number of bits per sample, per channel
  inputFormat.mBytesPerFrame = 8;          // Size of a frame == 1 sample per channel		
  inputFormat.mFramesPerPacket = 1;        // The smallest amount of indivisible data. Always 1 for uncompressed audio 	
  inputFormat.mBytesPerPacket = 8;
  if (!m_AudioUnit.SetInputFormat(&inputFormat))
    return false;
  
  // These define our output rate from the buffer, and are based on the 16-bit data coming from the client
  m_BytesPerFrame = inputFormat.mBytesPerFrame / 2;
  m_AvgBytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame / 2;      // 1 sample per channel per frame
  m_PassthroughSpoof = true;
  
  return true;  
}

bool CCoreAudioRenderer::InitializeEncoded(AudioDeviceID outputDevice)
{
  CStdString formatString;
  AudioStreamBasicDescription outputFormat = {0};
  AudioStreamID outputStream = 0;
        
  // Fetch a list of the streams defined by the output device
  AudioStreamIdList streams;
  UInt32  streamIndex = 0;
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

   // Probe physical formats
    StreamFormatList physicalFormats;
    stream.GetAvailablePhysicalFormats(&physicalFormats);
    while (!physicalFormats.empty())
    {
      AudioStreamRangedDescription& desc = physicalFormats.front();
      CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded:    Considering Physical Format: %s", StreamDescriptionToString(desc.mFormat, formatString));
      if (desc.mFormat.mFormatID == kAudioFormat60958AC3 && desc.mFormat.mSampleRate == 48000)
      {
        outputFormat = desc.mFormat; // Select this format
        m_OutputBufferIndex = streamIndex; // TODO: Is this technically correct? Will each stream have it's own IOProc buffer?
        outputStream = stream.GetId();
        break;
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
    CLog::Log(LOGERROR, "CoreAudioRenderer::InitializeEncoded: Unable to identify suitable output format.");
    return false;
  }    

  m_ChunkLen = outputFormat.mBytesPerPacket; // 1 Chunk == 1 Packet
  m_AvgBytesPerSec = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3) * outputFormat.mSampleRate; // For some reason, mBytesPerFrame is 0 for a cac3 stream
  m_BytesPerFrame = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: Selected stream[%u] - id: 0x%04X, Physical Format: %s (%u Bytes/sec.)", streamIndex, outputStream, StreamDescriptionToString(outputFormat, formatString), m_AvgBytesPerSec);  
  
  // TODO: Auto hogging sets this for us. Figure out how/when to turn it off or use it
  // It appears that leaving this set will aslo restore the previous stream format when the
  // Application exits. If auto hogging is set and we try to set hog mode, we will deadlock
  // From the SDK docs: "If the AudioDevice is in a non-mixable mode, the HAL will automatically take hog mode on behalf of the first process to start an IOProc."
  
  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock 
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)
  CCoreAudioHardware hw;
  bool autoHog = hw.GetAutoHogMode();
  CLog::Log(LOGDEBUG, " CoreAudioRenderer::InitializeEncoded: Auto 'hog' mode is set to '%s'.", autoHog ? "On" : "Off");
  if (!autoHog) // Try to handle this ourselves
  {
    m_AudioDevice.SetHogStatus(true); // Hog the device if it is not set to be done automatically
    m_AudioDevice.SetMixingSupport(false); // Try to disable mixing. If we cannot, it may not be a problem
  }
  
  // Configure the output stream object
  m_OutputStream.Open(outputStream); // This is the one we will keep
  m_OutputStream.SetPhysicalFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
  
  // Register for data request callbacks from the driver
  m_AudioDevice.AddIOProc(PassthroughRenderCallback, this);  
    
  return true;
}

#endif


