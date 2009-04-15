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

#include <stdafx.h>
#include "CoreAudioRenderer.h"
#include <Settings.h>
#include <Atomics.h>

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
// Performance Monitoring Helper Class
//***********************************************************************************************
CCoreAudioPerformance::CCoreAudioPerformance() :
  m_TotalBytesIn(0),
  m_TotalBytesOut(0),
  m_ExpectedBytesPerSec(0),
  m_ActualBytesPerSec(0),
  m_Flags(0),
  m_WatchdogEnable(false),
  m_WatchdogInterval(0),
  m_LastWatchdogCheck(0),
  m_LastWatchdogBytesIn(0),
  m_LastWatchdogBytesOut(0),
  m_WatchdogBitrateSensitivity(0.99f),
  m_WatchdogPreroll(0)
{
  
}

CCoreAudioPerformance::~CCoreAudioPerformance()
{
  
}

void CCoreAudioPerformance::Init(UInt32 expectedBytesPerSec, UInt32 watchdogInterval /*= 1000*/, UInt32 flags /*=0*/)
{
  m_ExpectedBytesPerSec = expectedBytesPerSec;
  m_WatchdogInterval = watchdogInterval;
  m_Flags = flags;
}

void CCoreAudioPerformance::ReportData(UInt32 bytesIn, UInt32 bytesOut)
{
  m_TotalBytesIn += bytesIn;
  m_TotalBytesOut += bytesOut;
  
  if (!m_WatchdogEnable)
    return;
  
  // Perform watchdog funtions
  UInt32 time = timeGetTime();
  if (!m_LastWatchdogCheck)
    m_LastWatchdogCheck = time;
  UInt32 deltaTime = time - m_LastWatchdogCheck;
  m_ActualBytesPerSec = (m_TotalBytesOut - m_LastWatchdogBytesOut) / ((float)deltaTime/1000.0f);
  if (deltaTime > m_WatchdogInterval)
  {
    if (m_TotalBytesOut > m_WatchdogPreroll) // Allow m_TotalBytesOut bytes to go by unmonitored
    {
      // Check outgoing bitrate
      if (m_ActualBytesPerSec < m_WatchdogBitrateSensitivity * m_ExpectedBytesPerSec) 
        CLog::Log(LOGWARNING, "CCoreAudioPerformance: Outgoing bitrate is lagging. Target: %u, Actual: %u. deltaTime was %u", m_ExpectedBytesPerSec, m_ActualBytesPerSec, deltaTime);
    }
    m_LastWatchdogCheck = time;
    m_LastWatchdogBytesIn = m_TotalBytesIn;
    m_LastWatchdogBytesOut = m_TotalBytesOut;    
  }
}

void CCoreAudioPerformance::EnableWatchdog(bool enable)
{
  if (!m_WatchdogEnable && enable)
    Reset();
  m_WatchdogEnable = enable;
}

void CCoreAudioPerformance::SetPreroll(UInt32 bytes)
{
  m_WatchdogPreroll = bytes;
}

void CCoreAudioPerformance::SetPreroll(float seconds)
{
  SetPreroll((UInt32)(seconds * m_ExpectedBytesPerSec));
}

void CCoreAudioPerformance::Reset()
{
  m_TotalBytesIn = 0;
  m_TotalBytesOut = 0;
  m_ActualBytesPerSec = 0;
  m_LastWatchdogCheck = 0;
  m_LastWatchdogBytesIn = 0;
  m_LastWatchdogBytesOut = 0;  
}

//***********************************************************************************************
// Contruction/Destruction
//***********************************************************************************************
CCoreAudioRenderer::CCoreAudioRenderer() :
  m_Pause(false),
  m_ChunkLen(0),
  m_MaxCacheLen(0),
  m_AvgBytesPerSec(0),
  m_CurrentVolume(0),
  m_Initialized(false),
  m_Passthrough(false),
  m_PassthroughSpoof(false),
  m_OutputBufferIndex(0),
  m_pCache(NULL),
  m_RunoutEvent(NULL)
{
  
}

CCoreAudioRenderer::~CCoreAudioRenderer()
{
    Deinitialize();
}

//***********************************************************************************************
// Initialization
//***********************************************************************************************

// Macro to use for sanity checks in each method. 'x' is the return value if not initialized
#define VERIFY_INIT(x) \
if (!m_Initialized) \
return x

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
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Input Stream Format %s", StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Output Stream Format % s", StreamDescriptionToString(outputDesc_end, formatString));    
  }
  
  m_MaxCacheLen = m_AvgBytesPerSec;     // Set the max cache size to 1 second of data. TODO: Make this more intelligent
  m_Pause = true;                       // Suspend rendering. We will start once we have some data.
  m_pCache = new CSliceQueue(m_MaxCacheLen);
  m_PerfMon.Init(m_AvgBytesPerSec, 1000, CCoreAudioPerformance::FlagDefault); // Set up the performance monitor
  //m_PerfMon.SetPreroll(5.0f); // Disable underrun detection for the first 5 seconds (after start and after resume)
  m_Initialized = true;
  
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Renderer Configuration - Chunk Len: %u, Max Cache: %u (%0.0fms).", m_ChunkLen, m_MaxCacheLen, 1000.0 *(float)m_MaxCacheLen/(float)m_AvgBytesPerSec);
  CLog::Log(LOGINFO, "CoreAudioRenderer::Initialize: Successfully configured audio output.");  
  return true;
}

HRESULT CCoreAudioRenderer::Deinitialize()
{
  VERIFY_INIT(S_OK); // Not really a failure if we weren't initialized

  // Stop rendering
  Stop();
  // Reset our state
  m_ChunkLen = 0;
  m_MaxCacheLen = 0;
  m_AvgBytesPerSec = 0;
  if (m_Passthrough)
    m_AudioDevice.RemoveIOProc();
  m_AudioUnit.Close();
  m_OutputStream.Close();
  m_AudioDevice.Close();
  delete m_pCache;
  m_pCache = NULL;
  m_Initialized = false;
  m_RunoutEvent = NULL;
  
  CLog::Log(LOGINFO, "CoreAudioRenderer::Deinitialize: Renderer has been shut down.");
  
  return S_OK;
}

//***********************************************************************************************
// Transport control methods
//***********************************************************************************************
HRESULT CCoreAudioRenderer::Pause()
{
  VERIFY_INIT(E_FAIL);
  
  if (!m_Pause)
  {
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Pause: Pausing Playback.");
    if (m_Passthrough)
      m_AudioDevice.Stop();
    else
      m_AudioUnit.Stop();
    m_Pause = true;
  }
  m_PerfMon.EnableWatchdog(false); // Stop monitoring, we're paused
  return S_OK;
}

HRESULT CCoreAudioRenderer::Resume()
{
  VERIFY_INIT(E_FAIL);

  if (m_Pause)
  {
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Resume: Resuming Playback.");
    if (m_Passthrough)
      m_AudioDevice.Start();
    else
     m_AudioUnit.Start();
    m_Pause = false;
  }
  m_PerfMon.EnableWatchdog(true); // Resume monitoring
  return S_OK;
}

HRESULT CCoreAudioRenderer::Stop()
{
  VERIFY_INIT(E_FAIL);

  if (m_Passthrough)
    m_AudioDevice.Stop();
  else
    m_AudioUnit.Stop();

  m_Pause = true;
  m_PerfMon.EnableWatchdog(false);
  m_pCache->Clear();

  return S_OK;
}

//***********************************************************************************************
// Volume control methods
//***********************************************************************************************
LONG CCoreAudioRenderer::GetMinimumVolume() const
{
  return -6000;
}

LONG CCoreAudioRenderer::GetMaximumVolume() const
{
  return 0;
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
  VERIFY_INIT(E_FAIL);

  if (!m_Passthrough && !m_PassthroughSpoof) // Don't change actual volume for encoded streams
  {  
    // Scale the provided value to a range of 0.0 -> 1.0
    Float32 volPct = (Float32)(nVolume + 6000.0f)/6000.0f;
    
    // Try to set the volume. If it fails there is not a lot to be done.
    if (!m_AudioUnit.SetCurrentVolume(volPct))
      return E_FAIL;
  }
  m_CurrentVolume = nVolume;
  return S_OK;
}

//***********************************************************************************************
// Data management methods
//***********************************************************************************************
DWORD CCoreAudioRenderer::GetSpace()
{
  VERIFY_INIT(0);
  return m_MaxCacheLen - m_pCache->GetTotalBytes(); // This is just an estimate, since the driver is asynchonously pulling data.
}

DWORD CCoreAudioRenderer::AddPackets(unsigned char *data, DWORD len)
{  
  VERIFY_INIT(0);
  
  // Require at least one 'chunk'. This allows us at least some measure of control over efficiency
  if (len < m_ChunkLen ||  m_pCache->GetTotalBytes() >= m_MaxCacheLen)
    return 0;

  DWORD cacheSpace = GetSpace();  
  if (len > cacheSpace)
    return 0; // Wait until we can accept all of it
  
  size_t bytesUsed = m_pCache->AddData(data, len);
  
  // Update tracking variable
  m_PerfMon.ReportData(bytesUsed, 0);
  Resume();  // We have some data. Attmept to resume playback
  
  return bytesUsed; // Number of bytes added to cache;
}

FLOAT CCoreAudioRenderer::GetDelay()
{
  VERIFY_INIT(0);
  // Calculate the duration of the data in the cache
  float delay = (float)m_pCache->GetTotalBytes()/(float)m_AvgBytesPerSec;
  // TODO: Obtain hardware/os latency for better accuracy
  return delay;
}

DWORD CCoreAudioRenderer::GetChunkLen()
{
  return m_ChunkLen;
}

void CCoreAudioRenderer::WaitCompletion()
{
  VERIFY_INIT();
  OSStatus ret =  MPCreateEvent(&m_RunoutEvent);
  if (!ret)
  {
    // TODO: If we want to be REALLY tight, we could add silence to run-out the hardware/os delay as well (not really necessary)
    UInt32 delay =  m_pCache->GetTotalBytes()/(m_AvgBytesPerSec/1000); // This is how much time 'should' be in the cache
    // TODO: Should we add a little padding here to allow for very small differences? Yes, but how much?
    ret = MPWaitForEvent(m_RunoutEvent, NULL, kDurationMillisecond * delay); // Wait for the callback thread to process the whole cache, but only wait as long as we expect it to take
    switch (ret)
    {
      case kMPTimeoutErr:
        CLog::Log(LOGERROR, "CCoreAudioRenderer::WaitCompletion: Timed-out waiting for runout. Remaining data will be truncated.");
        break;
      case noErr:
        break;
      default:
        CLog::Log(LOGERROR, "CCoreAudioRenderer::WaitCompletion: An error occurred while waiting for runout. Remaining data will be truncated. Error = 0x%08X [%4.4s]", ret, UInt32ToFourCC((UInt32*)&ret));
    }
    MPDeleteEvent(m_RunoutEvent);
    m_RunoutEvent = NULL;
  }
  else
    CLog::Log(LOGERROR, "CCoreAudioRenderer::WaitCompletion: Unable to create runout event. Remaining data will be truncated. Error = 0x%08X [%4.4s]", ret, UInt32ToFourCC((UInt32*)&ret));
  Stop();
}

//***********************************************************************************************
// Rendering Methods
//***********************************************************************************************
OSStatus CCoreAudioRenderer::OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{  
  // TODO: May need to remove all logging from this method since it is called from a realtime thread
  if (!m_Initialized)
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Callback to de/unitialized renderer.");
  
  // Make a local copy of the buffer index
  UInt32 buf = m_OutputBufferIndex; // This determines which device stream we send our data to.
  
  // Check for adequate space in the output buffer
  UInt32 bytesRequested = m_BytesPerFrame * inNumberFrames;
  if (bytesRequested > ioData->mBuffers[buf].mDataByteSize)
  {
    bytesRequested = ioData->mBuffers[buf].mDataByteSize;
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Supplied buffer is too small(%u) to accept requested sample data(%u). Truncating data.", ioData->mBuffers[0].mDataByteSize, bytesRequested);
  }

  if (m_pCache->GetTotalBytes() < bytesRequested) // Not enough data to satisfy the request
  {
    Pause(); // Stop further requests until we have more data.  The AddPackets method will resume playback
    memset(ioData->mBuffers[buf].mData, 0, ioData->mBuffers[buf].mDataByteSize);  // Return only silence
    if (m_RunoutEvent) // We were waiting for a runout. This is not an error
    {
      MPSetEvent(m_RunoutEvent, 0); // Tell the waiting thread we are done
      CLog::Log(LOGDEBUG, "CCoreAudioRenderer::OnRender: Runout complete");
    }
    else
      CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Buffer underrun.");
  }
  else // Fetch some data for the caller
  {
     if (m_PassthroughSpoof) // Need to convert the data
    {
      m_pCache->GetData(g_SampleBuffer, bytesRequested); // Retrieve data from queue (16-bit Signed Integer)
      ShortToFloat(g_SampleBuffer, (float*)ioData->mBuffers[buf].mData, ioData->mBuffers[buf].mNumberChannels * inNumberFrames); // Convert to 32-bit float
    }
    else // Copy data as-is
    {
      m_pCache->GetData(ioData->mBuffers[buf].mData, bytesRequested);
    }    
    if ((m_Passthrough || m_PassthroughSpoof) && m_CurrentVolume == -6000) // Mute for passthrough. Throw away any actual data to keep the stream moving.
      memset(ioData->mBuffers[buf].mData, 0, ioData->mBuffers[buf].mDataByteSize); // TODO: is there a way to do this without memcpy AND memset...?    
    // Calculate stats and perform a sanity check
    m_PerfMon.ReportData(0, bytesRequested); // TODO: Should we check the result?
  }
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

//***********************************************************************************************
// Audio Device Initialization Methods
//***********************************************************************************************
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
  m_AvgBytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame;      // 1 sample per channel per frame
  
  return true;
}

bool CCoreAudioRenderer::SpoofPCM()
{
  // Set the Sample Rate as defined by the spec.
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
  
  // These are based on the 16-bit data coming from the client
  m_BytesPerFrame = inputFormat.mBytesPerFrame / 2;
  m_AvgBytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame / 2;      // 1 sample per channel per frame
  m_PassthroughSpoof = true;
  
  return true;  
}

bool CCoreAudioRenderer::InitializeEncoded(AudioDeviceID outputDevice)
{
  return false; // un-comment to force PCM Spoofing (DD-Wav)
  
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
      if (desc.mFormat.mFormatID == kAudioFormat60958AC3 && desc.mFormat.mSampleRate == 48000) // TODO: Do we want to support other passthrough sample rates?
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
  m_AvgBytesPerSec = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3) * outputFormat.mSampleRate; // mBytesPerFrame is 0 for a cac3 stream
  m_BytesPerFrame = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: Selected stream[%u] - id: 0x%04X, Physical Format: %s (%u Bytes/sec.)", streamIndex, outputStream, StreamDescriptionToString(outputFormat, formatString), m_AvgBytesPerSec);  
  
  // TODO: Auto hogging sets this for us. Figure out how/when to turn it off or use it
  // It appears that leaving this set will aslo restore the previous stream format when the
  // Application exits. If auto hogging is set and we try to set hog mode, we will deadlock
  // From the SDK docs: "If the AudioDevice is in a non-mixable mode, the HAL will automatically take hog mode on behalf of the first process to start an IOProc."
  
  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock 
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)
  bool autoHog = CCoreAudioHardware::GetAutoHogMode();
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


