#ifdef __APPLE__
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#if !defined(__arm__)
#include "threads/SystemClock.h"
#include <CoreServices/CoreServices.h>

#include "CoreAudioRenderer.h"
#include "guilib/AudioContext.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "threads/Atomics.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"


const AudioChannelLabel g_LabelMap[] =
{
  kAudioChannelLabel_Left, // PCM_FRONT_LEFT,
  kAudioChannelLabel_Right, //  PCM_FRONT_RIGHT,
  kAudioChannelLabel_Center, //  PCM_FRONT_CENTER,
  kAudioChannelLabel_LFEScreen, //  PCM_LOW_FREQUENCY,
  kAudioChannelLabel_LeftSurroundDirect, //  PCM_BACK_LEFT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_RightSurroundDirect, //  PCM_BACK_RIGHT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_LeftCenter, //  PCM_FRONT_LEFT_OF_CENTER,
  kAudioChannelLabel_RightCenter, //  PCM_FRONT_RIGHT_OF_CENTER,
  kAudioChannelLabel_CenterSurround, //  PCM_BACK_CENTER,
  kAudioChannelLabel_LeftSurround, //  PCM_SIDE_LEFT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_RightSurround, //  PCM_SIDE_RIGHT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_VerticalHeightLeft, //  PCM_TOP_FRONT_LEFT,
  kAudioChannelLabel_VerticalHeightRight, //  PCM_TOP_FRONT_RIGHT,
  kAudioChannelLabel_VerticalHeightCenter, //  PCM_TOP_FRONT_CENTER,
  kAudioChannelLabel_TopCenterSurround, //  PCM_TOP_CENTER,
  kAudioChannelLabel_TopBackLeft, //  PCM_TOP_BACK_LEFT,
  kAudioChannelLabel_TopBackRight, //  PCM_TOP_BACK_RIGHT,
  kAudioChannelLabel_TopBackCenter //  PCM_TOP_BACK_CENTER 
};

const AudioChannelLayoutTag g_LayoutMap[] = 
{
  kAudioChannelLayoutTag_Stereo, // PCM_LAYOUT_2_0 = 0,
  kAudioChannelLayoutTag_DVD_4, // PCM_LAYOUT_2_1,
  kAudioChannelLayoutTag_MPEG_3_0_A, // PCM_LAYOUT_3_0,
  kAudioChannelLayoutTag_DVD_10, // PCM_LAYOUT_3_1,
  kAudioChannelLayoutTag_DVD_3, // PCM_LAYOUT_4_0,
  kAudioChannelLayoutTag_DVD_6, // PCM_LAYOUT_4_1,
  kAudioChannelLayoutTag_MPEG_5_0_A, // PCM_LAYOUT_5_0,
  kAudioChannelLayoutTag_MPEG_5_1_A, // PCM_LAYOUT_5_1,
  kAudioChannelLayoutTag_AudioUnit_7_0, // PCM_LAYOUT_7_0, ** This layout may be incorrect...no content to testß˚ **
  kAudioChannelLayoutTag_MPEG_7_1_A, // PCM_LAYOUT_7_1
};

/////////////////////////////////////////////////////////////////////////////////
// CAtomicAllocator: Wrapper class for lf_heap.
////////////////////////////////////////////////////////////////////////////////
CAtomicAllocator::CAtomicAllocator(size_t blockSize) :
  m_BlockSize(blockSize)
{
  lf_heap_init(&m_Heap, blockSize);
}

CAtomicAllocator::~CAtomicAllocator()
{
  lf_heap_deinit(&m_Heap);
}

void* CAtomicAllocator::Alloc()
{
  return lf_heap_alloc(&m_Heap);
}

void CAtomicAllocator::Free(void* p)
{
  lf_heap_free(&m_Heap, p);
}

size_t CAtomicAllocator::GetBlockSize()
{
  return m_BlockSize;
}

//////////////////////////////////////////////////////////////////////////////////////
// CSliceQueue: Lock-free queue for audio_slices
//////////////////////////////////////////////////////////////////////////////////////
CSliceQueue::CSliceQueue(size_t sliceSize) :
  m_TotalBytes(0),
  m_pPartialSlice(NULL),
  m_RemainderSize(0)
{
  m_pAllocator = new CAtomicAllocator(sliceSize + offsetof(audio_slice, data));
  lf_queue_init(&m_Queue);
}

CSliceQueue::~CSliceQueue()
{
  Clear();
  lf_queue_deinit(&m_Queue);
  delete m_pAllocator;
}

// NOTE: The value of m_TotalBytes is only guaranteed to be accurate to within one slice,
//  but is sufficient. This means that it may at times be one slice too high or one slice
//  tool low, but will never be < 0.
void CSliceQueue::Push(audio_slice* pSlice)
{
  if (pSlice)
  {
    lf_queue_enqueue(&m_Queue, pSlice);
    AtomicAdd((long*)&m_TotalBytes, (long)pSlice->header.data_len);
  }
}

audio_slice* CSliceQueue::Pop()
{
  audio_slice* pSlice = (audio_slice*)lf_queue_dequeue(&m_Queue);
  if (pSlice)
    AtomicSubtract((long*)&m_TotalBytes, (long)pSlice->header.data_len);
  return pSlice;
}

// *** NOTE: AddData and GetData are thread-safe for multiple writers and one reader
size_t CSliceQueue::AddData(void* pBuf, size_t bufLen)
{
  size_t bytesLeft = bufLen;
  unsigned char* pData = (unsigned char*)pBuf;
  if (pBuf && bufLen)
  {
    while (bytesLeft)
    {
      // TODO: find a way to ensure success...
      audio_slice* pSlice = (audio_slice*)m_pAllocator->Alloc(); // Allocation should never fail. The heap grows automatically.
      if (!pSlice)
      {
        CLog::Log(LOGDEBUG, "CSliceQueue::AddData: Failed to allocate new slice");
        return bufLen - bytesLeft;
      }
      pSlice->header.data_len = m_pAllocator->GetBlockSize() - offsetof(audio_slice, data);
      if (pSlice->header.data_len >= bytesLeft) // plenty of room. move it all in.
      {
        memcpy(pSlice->get_data(), pData, bytesLeft);
        pSlice->header.data_len = bytesLeft; // Adjust the reported size of the container
      }
      else
      {
        memcpy(pSlice->get_data(), pData, pSlice->header.data_len); // Copy all we can into this slice. More to come.
      }
      bytesLeft -= pSlice->header.data_len;
      pData += pSlice->header.data_len;
      Push(pSlice);
    }
  }
  return  bufLen - bytesLeft;
}

size_t CSliceQueue::GetData(void* pBuf, size_t bufLen)
{
  if (!pBuf || !bufLen)
    return 0;
  
  size_t remainder = 0;
  audio_slice* pNext = NULL;
  size_t bytesUsed = 0;
  
  // See if we can fill the request out of our partial slice (if there is one)
  if (m_RemainderSize >= bufLen)
  {
    memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , bufLen);
    m_RemainderSize -= bufLen;
    bytesUsed = bufLen;
  }
  else // Pull what we can from the partial slice and get the rest from complete slices
  {
    // Take what we can from the partial slice (if there is one)
    if (m_RemainderSize)
    {
      memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , m_RemainderSize);
      bytesUsed += m_RemainderSize;
      m_RemainderSize = 0;
    }
    
    // Pull slices from the fifo until we have enough data
    do // TODO: The efficiency of this loop can be improved (a lot I imagine)
    {
      pNext = Pop();
      if (!pNext)
        break;
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > bufLen) // Check for a partial slice
        remainder = nextLen - (bufLen - bytesUsed);
      memcpy((BYTE*)pBuf + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += (nextLen - remainder); // Increment output size (remainder will be captured separately)
      if (!remainder)
        m_pAllocator->Free(pNext); // Free the copied slice
    } while (bytesUsed < bufLen);
  }
  
  // Clean up the previous partial slice
  if (!m_RemainderSize && m_pPartialSlice)
  {
    m_pAllocator->Free(m_pPartialSlice);
    m_pPartialSlice = NULL;
  }
  
  // Save off the new partial slice (if there is one)
  if (remainder)
  {
    m_pPartialSlice = pNext;
    m_RemainderSize = remainder;
  }
  
  return bytesUsed;
}

size_t CSliceQueue::GetTotalBytes()
{
  return m_TotalBytes + m_RemainderSize;
}

void CSliceQueue::Clear()
{
  while (audio_slice* pSlice = Pop())
    m_pAllocator->Free(pSlice);
  m_pAllocator->Free(m_pPartialSlice);
  m_pPartialSlice = NULL;
  m_RemainderSize = 0;
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
  UInt32 time = XbmcThreads::SystemClockMillis();
  if (!m_LastWatchdogCheck)
    m_LastWatchdogCheck = time;
  UInt32 deltaTime = time - m_LastWatchdogCheck;
  m_ActualBytesPerSec = (m_TotalBytesOut - m_LastWatchdogBytesOut) / ((float)deltaTime/1000.0f);
  if (deltaTime > m_WatchdogInterval)
  {
    if (m_TotalBytesOut > m_WatchdogPreroll) // Allow m_WatchdogPreroll bytes to go by unmonitored
    {
      // Check outgoing bitrate
      if (m_ActualBytesPerSec < m_WatchdogBitrateSensitivity * m_ExpectedBytesPerSec)
        CLog::Log(LOGWARNING, "CCoreAudioPerformance: Outgoing bitrate is lagging. Target: %lu, Actual: %lu. deltaTime was %lu", m_ExpectedBytesPerSec, m_ActualBytesPerSec, deltaTime);
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
// Surround Up/Down Mapping Class
//***********************************************************************************************
CCoreAudioMixMap::CCoreAudioMixMap() :
  m_isValid(false)
{
  m_pMap = (Float32*)calloc(sizeof(AudioChannelLayout), 1);
}

CCoreAudioMixMap::CCoreAudioMixMap(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout) :
  m_isValid(false)
{
  Rebuild(inLayout, outLayout);
}

CCoreAudioMixMap::~CCoreAudioMixMap()
{
  if (m_pMap)
  {
    free(m_pMap);
    m_pMap = NULL;
  }
}

void CCoreAudioMixMap::Rebuild(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout)
{
  // map[in][out] = mix-level of input_channel[in] into output_channel[out]

  if (m_pMap)
  {
    free(m_pMap);
    m_pMap = NULL;
  }

  m_inChannels = CCoreAudioChannelLayout::GetChannelCountForLayout(inLayout);
  m_outChannels = CCoreAudioChannelLayout::GetChannelCountForLayout(outLayout);
  
  // Try to find a 'well-known' matrix
  const AudioChannelLayout* layouts[] = {&inLayout, &outLayout};
  UInt32 propSize = 0;
  OSStatus ret = AudioFormatGetPropertyInfo(kAudioFormatProperty_MatrixMixMap, sizeof(layouts), layouts, &propSize);
  m_pMap = (Float32*)calloc(1,propSize);
  
  // Try and get a predefined mixmap
  ret = AudioFormatGetProperty(kAudioFormatProperty_MatrixMixMap, sizeof(layouts), layouts, &propSize, m_pMap);
  if (!ret)
  {
    m_isValid = true;
    return; // Nothing else to do...a map already exists
  }
  
  // No predefined mixmap was available. Going to have to build it manually
  CLog::Log(LOGDEBUG, "CCoreAudioMixMap::CreateMap: Unable to locate pre-defined mixing matrix");
  
  m_isValid = false;
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
  m_EnableVolumeControl(true),
  m_OutputBufferIndex(0),
  m_pCache(NULL),
  m_DoRunout(0)
{
  SInt32 major,  minor;
  Gestalt(gestaltSystemVersionMajor, &major);
  Gestalt(gestaltSystemVersionMinor, &minor);
  
  // By default, kAudioHardwarePropertyRunLoop points at the process's main thread on SnowLeopard,
  // If your process lacks such a run loop, you can set kAudioHardwarePropertyRunLoop to NULL which
  // tells the HAL to run it's own thread for notifications (which was the default prior to SnowLeopard).
  // So tell the HAL to use its own thread for similar behavior under all supported versions of OSX.
  if (major == 10 && minor >=6)
  {
    CFRunLoopRef theRunLoop = NULL;
    AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    OSStatus theError = AudioObjectSetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
    if (theError != noErr)
    {
      CLog::Log(LOGERROR, "CoreAudioRenderer::constructor: kAudioHardwarePropertyRunLoop error.");
    }
  }
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

bool CCoreAudioRenderer::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic /*Useless Legacy Parameter*/, EEncoded bPassthrough)
{
  // Have to clean house before we start again.
  // TODO: Should we return failure instead?
  if (m_Initialized)
    Deinitialize();
  
  if(bPassthrough)
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE_DIGITAL);
  else
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  
  // TODO: If debugging, output information about all devices/streams
  
  // Attempt to find the configured output device
  AudioDeviceID outputDevice = CCoreAudioHardware::FindAudioDevice(g_guiSettings.GetString("audiooutput.audiodevice"));
  // Fall back to the default device if no match is found
  if (!outputDevice)
  {
    CLog::Log(LOGWARNING, "CoreAudioRenderer::Initialize: "
      "Unable to locate configured device, falling-back to the system default.");
    outputDevice = CCoreAudioHardware::GetDefaultOutputDevice();
    // Not a lot to be done with no device.
    // TODO: Should we just grab the first existing device?
    if (!outputDevice)
      return false;
  }
  
  // TODO: Determine if the device is in-use/locked by another process.
  
  // Attach our output object to the device
  m_AudioDevice.Open(outputDevice);
  
  // If this is a passthrough (AC3/DTS) stream, attempt to handle it natively
  if (bPassthrough)
  {
    if (g_guiSettings.GetBool("videoplayer.adjustrefreshrate"))
    {
      int delay = g_guiSettings.GetInt("videoplayer.pauseafterrefreshchange");
      if (delay < 2)
        delay += 20;
      CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: "
        "delay(%d seconds) audio init for adjust refresh rate when passthrough",
        delay/10);
      Sleep(delay * 100);
    }
    m_Passthrough = InitializeEncoded(outputDevice, uiSamplesPerSec);
    // TODO: wait for audio device startup
    Sleep(200);
  }
  
  // If this is a PCM stream, or we failed to handle a passthrough stream natively,
  // prepare the standard interleaved PCM interface
  if (!m_Passthrough)
  {
    // Create the Output AudioUnit Component
    if (!m_AUOutput.Open(kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple))
      return false;
    
    // Hook the Ouput AudioUnit to the selected device
    if (!m_AUOutput.SetCurrentDevice(outputDevice))
      return false;
    
    // If we are here and this is a passthrough stream, native handling failed.
    // Try to handle it as IEC61937 data over straight PCM (DD-Wav)
    bool configured = false;
    if (bPassthrough)
    {
      CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: "
        "No suitable AC3 output format found. Attempting DD-Wav.");
      configured = InitializePCMEncoded(uiSamplesPerSec);
      // TODO: wait for audio device startup
      Sleep(100);
    }
    else
    {
      // Standard PCM data
      configured = InitializePCM(iChannels, uiSamplesPerSec, uiBitsPerSample, channelMap);
      // TODO: wait for audio device startup
      Sleep(100);
    }
    
    // No suitable output format was able to be configured
    if (!configured)
      return false;
    
    // Configure the maximum number of frames that the AudioUnit will ask to process at one time.
    //  If this is not called, there is no guarantee that the callback will ever be called.
    // Size of the output buffer, in Frames
    UInt32 bufferFrames = m_AUOutput.GetBufferFrameSize();
    if (!m_AUOutput.SetMaxFramesPerSlice(bufferFrames))
      return false;
    
    // This is the minimum amount of data that we will accept from a client
    m_ChunkLen = bufferFrames * m_BytesPerFrame;
    
    // Setup the callback function that the AudioUnit will use to request data	
    ICoreAudioSource* pSource = this;
    if (m_AUCompressor.IsInitialized()) // A mixer+compressor are in-use
      pSource = &m_AUCompressor;
    if (!m_AUOutput.SetInputSource(pSource))
      return false;      
    
    // Initialize the Output AudioUnit
    if (!m_AUOutput.Initialize())
      return false;
    
    // Log some information about the stream
    AudioStreamBasicDescription inputDesc_end, outputDesc_end;
    CStdString formatString;
    m_AUOutput.GetInputFormat(&inputDesc_end);
    m_AUOutput.GetOutputFormat(&outputDesc_end);
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Input Stream Format %s",
      StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: Output Stream Format %s",
      StreamDescriptionToString(outputDesc_end, formatString));
  }
  
  m_NumLatencyFrames = m_AudioDevice.GetNumLatencyFrames();
  // Set the max cache size to 1 second of data. TODO: Make this more intelligent
  m_MaxCacheLen = m_AvgBytesPerSec;
  // Suspend rendering. We will start once we have some data.
  m_Pause = true;
  // Initialize our incoming data cache
  m_pCache = new CSliceQueue(m_ChunkLen);
#ifdef _DEBUG
  // Set up the performance monitor
  m_PerfMon.Init(m_AvgBytesPerSec, 1000, CCoreAudioPerformance::FlagDefault);
  // Disable underrun detection for the first 2 seconds (after start and after resume)
  m_PerfMon.SetPreroll(2.0f);
#endif
  m_Initialized = true;
  m_DoRunout = 0;
  
  SetCurrentVolume(g_settings.m_nVolumeLevel);
  
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::Initialize: "
    "Renderer Configuration - Chunk Len: %u, Max Cache: %lu (%0.0fms).",
    m_ChunkLen, m_MaxCacheLen, 1000.0 *(float)m_MaxCacheLen/(float)m_AvgBytesPerSec);
  CLog::Log(LOGINFO, "CoreAudioRenderer::Initialize: Successfully configured audio output.");
  
  return true;
}

bool CCoreAudioRenderer::Deinitialize()
{
  VERIFY_INIT(true); // Not really a failure if we weren't initialized
  
  // Stop rendering
  Stop();
  // Reset our state
  m_ChunkLen = 0;
  m_MaxCacheLen = 0;
  m_AvgBytesPerSec = 0;
  if (m_Passthrough)
    m_AudioDevice.RemoveIOProc();
  m_AUCompressor.Close();
  m_MixerUnit.Close();
  m_AUConverter.Close();
  m_AUOutput.Close();
  m_OutputStream.Close();
  Sleep(10);
  m_AudioDevice.Close();
  delete m_pCache;
  m_pCache = NULL;
  m_Initialized = false;
  m_DoRunout = 0;
  m_EnableVolumeControl = true;
  
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  
  CLog::Log(LOGINFO, "CoreAudioRenderer::Deinitialize: Renderer has been shut down.");
  
  return true;
}

//***********************************************************************************************
// Transport control methods
//***********************************************************************************************
bool CCoreAudioRenderer::Pause()
{
  VERIFY_INIT(false);
  
  if (!m_Pause)
  {
    //CLog::Log(LOGDEBUG, "CoreAudioRenderer::Pause: Pausing Playback.");
    if (m_Passthrough)
      m_AudioDevice.Stop();
    else
      m_AUOutput.Stop();
    m_Pause = true;
  }
#ifdef _DEBUG
  m_PerfMon.EnableWatchdog(false); // Stop monitoring, we're paused
#endif
  return true;
}

bool CCoreAudioRenderer::Resume()
{
  VERIFY_INIT(false);
  
  if (m_Pause)
  {
    //CLog::Log(LOGDEBUG, "CoreAudioRenderer::Resume: Resuming Playback.");
    if (m_Passthrough)
      m_AudioDevice.Start();
    else
      m_AUOutput.Start();
    m_Pause = false;
  }
#ifdef _DEBUG
  m_PerfMon.EnableWatchdog(true); // Resume monitoring
#endif
  return true;
}

bool CCoreAudioRenderer::Stop()
{
  VERIFY_INIT(false);
  
  if (m_Passthrough)
    m_AudioDevice.Stop();
  else
    m_AUOutput.Stop();
  
  m_Pause = true;
#ifdef _DEBUG
  m_PerfMon.EnableWatchdog(false);
#endif
  m_pCache->Clear();
  
  return true;
}

//***********************************************************************************************
// Volume control methods
//***********************************************************************************************
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

bool CCoreAudioRenderer::SetCurrentVolume(LONG nVolume)
{
  VERIFY_INIT(false);
  
  if (m_EnableVolumeControl) // Don't change actual volume for encoded streams
  {
    // Convert milliBels to percent
    Float32 volPct = pow(10.0f, (float)nVolume/2000.0f);
    
    // Try to set the volume. If it fails there is not a lot to be done.
    if (!m_AUOutput.SetCurrentVolume(volPct))
      return false;
  }
  m_CurrentVolume = nVolume; // Store the volume setpoint. We need this to check for 'mute'
  return true;
}

void CCoreAudioRenderer::SetDynamicRangeCompression(long drc)
{
  if (m_AUCompressor.IsInitialized())
    m_AUCompressor.SetMasterGain(((float)drc)/100.0f);
}

//***********************************************************************************************
// Data management methods
//***********************************************************************************************
unsigned int CCoreAudioRenderer::GetSpace()
{
  VERIFY_INIT(0);
  return m_MaxCacheLen - m_pCache->GetTotalBytes(); // This is just an estimate, since the driver is asynchonously pulling data.
}

unsigned int CCoreAudioRenderer::AddPackets(const void* data, DWORD len)
{
  VERIFY_INIT(0);
  
  // Require at least one 'chunk'. This allows us at least some measure of control over efficiency
  if (len < m_ChunkLen || m_pCache->GetTotalBytes() >= m_MaxCacheLen)
    return 0;
  
  unsigned int cacheSpace = GetSpace();
  if (len > cacheSpace)
    return 0; // Wait until we can accept all of it
  
  size_t bytesUsed = m_pCache->AddData((void*)data, len);
  
#ifdef _DEBUG
  // Update tracking variable
  m_PerfMon.ReportData(bytesUsed, 0);
#endif
  Resume();  // We have some data. Attmept to resume playback
  
  return bytesUsed; // Number of bytes added to cache;
}

float CCoreAudioRenderer::GetDelay()
{
  VERIFY_INIT(0);
  // Calculate the duration of the data in the cache
  float delay = (float)m_pCache->GetTotalBytes()/(float)m_AvgBytesPerSec;
  // TODO: Obtain hardware/os latency for better accuracy
  delay += (float)m_NumLatencyFrames/(float)m_AvgBytesPerSec;
  
  return delay;
}

float CCoreAudioRenderer::GetCacheTime()
{
  return GetDelay();
}

float CCoreAudioRenderer::GetCacheTotal()
{
  return (float)m_MaxCacheLen / m_AvgBytesPerSec;
}

unsigned int CCoreAudioRenderer::GetChunkLen()
{
  return m_ChunkLen;
}

void CCoreAudioRenderer::WaitCompletion()
{
  VERIFY_INIT();
  
  // The cache is already empty. There is nothing to wait for.
  if (m_pCache->GetTotalBytes() == 0)
    return;
  
  // Signal that we are waiting
  AtomicIncrement(&m_DoRunout);
  // Signal that a buffer underrun is OK
  m_DoRunout = 1;
  // TODO: Should we pad the wait time to allow for preemption?
  // This is how much time 'should' be in the cache (plus 10ms for preemption hedge)
  UInt32 delay = (UInt32)(GetDelay() * 1000.0f) + 10; 
  if (delay)
  {
    // Wait for the callback thread to process the whole cache,
    //  but only wait as long as we expect it to take.
    m_RunoutEvent.WaitMSec(delay);
    if (!m_RunoutEvent.WaitMSec(delay))
    {
      CLog::Log(LOGERROR, "CCoreAudioRenderer::WaitCompletion: "
        "Timed-out waiting for runout. Remaining data will be truncated.");
    }
  }
  
  Stop();
}

//***********************************************************************************************
// Rendering Methods
//***********************************************************************************************
OSStatus CCoreAudioRenderer::Render(AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList)
{
  OSStatus ret = OnRender(actionFlags, pTimeStamp, busNumber, frameCount, pBufList);
  return ret;
}

OSStatus CCoreAudioRenderer::OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  if (!m_Initialized)
    CLog::Log(LOGERROR, "CCoreAudioRenderer::OnRender: Callback to de/unitialized renderer.");
  
  // Process the request
  // Data length requested, based on the input data format
  UInt32 bytesRequested = m_BytesPerFrame * inNumberFrames; 
  UInt32 bytesRead = (UInt32)m_pCache->GetData(ioData->mBuffers[m_OutputBufferIndex].mData, bytesRequested);
  if (bytesRead < bytesRequested)
  {
    // Stop further requests until we have more data.
    // The AddPackets method will resume playback
    Pause(); 
    // Tell anyone who cares that the cache is empty
    m_RunoutEvent.Set();
    // We were waiting for a runout. This is not an error.
    if (m_DoRunout)
    {
      //CLog::Log(LOGDEBUG, "CCoreAudioRenderer::OnRender: Runout complete");
      m_DoRunout = 0;
    }
    /*
     else
     CLog::Log(LOGDEBUG, "CCoreAudioRenderer::OnRender: Buffer underrun.");
     */
  }
  // Hard mute for formats that do not allow standard volume control. Throw away any actual data to keep the stream moving.
  if (!m_EnableVolumeControl && m_CurrentVolume <= VOLUME_MINIMUM)
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
  else
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = bytesRead;
  
#ifdef _DEBUG
  // Calculate stats and perform a sanity check
  m_PerfMon.ReportData(0, bytesRead); // TODO: Should we check the result?
#endif
  return noErr;
}

// Static Callback from AudioDevice
OSStatus CCoreAudioRenderer::DirectRenderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData)
{
  CCoreAudioRenderer* pThis = (CCoreAudioRenderer*)inClientData;
  return pThis->OnRender(NULL, inInputTime, 0, outOutputData->mBuffers[0].mDataByteSize / pThis->m_BytesPerFrame, outOutputData);
}

//***********************************************************************************************
// Audio Device Initialization Methods
//***********************************************************************************************
bool CCoreAudioRenderer::InitializePCM(UInt32 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample, enum PCMChannels *channelMap, bool allowMixing /*= true*/)
{
  // Set the input stream format for the first AudioUnit (this is what is being sent to us)
  AudioStreamBasicDescription inputFormat;
  AudioStreamBasicDescription outputFormat;
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
  inputFormat.mReserved = 0;  
  
  // Configure up/down mixing if necessary, if caller allows it (and provides enough information to complete it)
  if (allowMixing && channelMap)
  {
    bool hasLFE = false;
    // Convert XBMC input channel layout format to CoreAudio layout format
    AudioChannelLayout* pInLayout = (AudioChannelLayout*)malloc(sizeof(AudioChannelLayout) + sizeof(AudioChannelDescription) * channels);
    pInLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
    pInLayout->mChannelBitmap = 0;
    pInLayout->mNumberChannelDescriptions = channels;
    for (unsigned int chan=0; chan < channels; chan++)
    {
      AudioChannelDescription* pDesc = &pInLayout->mChannelDescriptions[chan];
      pDesc->mChannelLabel = g_LabelMap[(unsigned int)channelMap[chan]]; // Convert from XBMC channel tag to CoreAudio channel tag
      pDesc->mChannelFlags = kAudioChannelFlags_AllOff;
      pDesc->mCoordinates[0] = 0.0f;
      pDesc->mCoordinates[1] = 0.0f;
      pDesc->mCoordinates[2] = 0.0f;
      if (pDesc->mChannelLabel == kAudioChannelLabel_LFEScreen)
        hasLFE = true;
    }
    
    // HACK: Fix broken channel layouts coming from some aac sources that include rear channel but no side channels.
    // 5.1 streams should include front and side channels. Rear channels are added by 6.1 and 7.1, so any 5.1 
    // source that claims to have rear channels is wrong.
    if (inputFormat.mChannelsPerFrame == 6 && hasLFE) // Check for 5.1 configuration (as best we can without getting too silly)
    {
      for (unsigned int chan=0; chan < inputFormat.mChannelsPerFrame; chan++)
      {
        AudioChannelDescription* pDesc = &pInLayout->mChannelDescriptions[chan];
        if (pDesc->mChannelLabel == kAudioChannelLabel_LeftSurround || pDesc->mChannelLabel == kAudioChannelLabel_LeftSurround)
          break; // Required condition cannot be true
        
        if (pDesc->mChannelLabel == kAudioChannelLabel_LeftSurroundDirect)
        {
          pDesc->mChannelLabel = kAudioChannelLabel_LeftSurround; // Change [Back Left] to [Side Left]
          CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: Detected faulty input channel map...fixing(Back Left-->Side Left)");
        }
        if (pDesc->mChannelLabel == kAudioChannelLabel_RightSurroundDirect)
        {
          pDesc->mChannelLabel = kAudioChannelLabel_RightSurround; // Change [Back Left] to [Side Left]
          CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: Detected faulty input channel map...fixing(Back Right-->Side Right)");
        }
      }
    }

    CCoreAudioChannelLayout sourceLayout(*pInLayout);
    free(pInLayout);
    pInLayout = NULL;

    CStdString strInLayout;
    CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: Source Stream Layout: %s", CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)sourceLayout, strInLayout));  
    
    // Get User-Configured (XBMC) Speaker Configuration
    AudioChannelLayout guiLayout;
    guiLayout.mChannelLayoutTag = g_LayoutMap[(PCMLayout)g_guiSettings.GetInt("audiooutput.channellayout")];
    CCoreAudioChannelLayout userLayout(guiLayout);
    CStdString strUserLayout;
    CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: User-Configured Speaker Layout: %s", CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)userLayout, strUserLayout));  
        
    // Get OS-Configured (Audio MIDI Setup) Speaker Configuration (Channel Layout)
    CCoreAudioChannelLayout deviceLayout;
    if (!m_AudioDevice.GetPreferredChannelLayout(deviceLayout))
      return false;

    CStdString strOutLayout;
    CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: Output Device Layout: %s", CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)deviceLayout, strOutLayout));  

    // TODO: 
    // Reconcile the OS and GUI layout configurations. Clamp to the minimum number of speakers
    // For each OS-defined output, see if it exists in the GUI configuration
    // If it does, add it to the 'union' layout (bitmap?)
    // User may have configured 5.1 in GUI, but only 2.0 in OS
    // Resulting layout would be {FL, FR}
    // User may have configured 2.0 in GUI, and 5.1 in OS
    // Resulting layout would be {FL, FR}

    // Correct any configuration incompatibilities
    //    if (CCoreAudioChannelLayout::GetChannelCountForLayout(guiLayout) < CCoreAudioChannelLayout::GetChannelCountForLayout(deviceLayout))
    //      deviceLayout.CopyLayout(guiLayout);
    
    // TODO: Skip matrix mixer if input/output are compatible

    AudioChannelLayout* layoutCandidates[] = {(AudioChannelLayout*)deviceLayout, (AudioChannelLayout*)userLayout, NULL};

    // Try to construct a mapping matrix for the mixer. Work through the layout candidates and see if any will work
    CCoreAudioMixMap mixMap;
    for(AudioChannelLayout** pLayout = layoutCandidates; *pLayout != NULL; pLayout++)
    {
      mixMap.Rebuild(*sourceLayout, **pLayout);
      if (mixMap.IsValid())
        break;
    }
    
    if (mixMap.IsValid())
    {
      if (!m_AUConverter.Open(kAudioUnitType_FormatConverter, kAudioUnitSubType_AUConverter, kAudioUnitManufacturer_Apple) || 
          !m_AUConverter.SetInputFormat(&inputFormat))
        return false;
      
      // Audio units use noninterleaved 32-bit floating point linear PCM data for input and output,
      // ...except in the case of an audio unit that is a data format converter, which converts to or from this format.
      AudioStreamBasicDescription fmt;
      fmt.mFormatID = kAudioFormatLinearPCM;
      fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
      fmt.mBitsPerChannel = sizeof(Float32)<<3;
      fmt.mSampleRate = (Float64)samplesPerSecond;
      fmt.mFramesPerPacket = 1;
      fmt.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
      fmt.mBytesPerFrame = sizeof(Float32);
      fmt.mBytesPerPacket = sizeof(Float32);
      
      // Set-up the format converter
      if (!m_AUConverter.SetOutputFormat(&fmt) ||
          !m_AUConverter.SetInputSource(this) || // Converter's data comes from the renderer 
          !m_AUConverter.Initialize())
        return false;
          
      // Set-up the mixer input
      if (!m_MixerUnit.Open() || // Create the MatrixMixer AudioUnit Component to handle up/down mix)
          !m_MixerUnit.SetInputBusCount(1) || // Configure the mixer
          !m_MixerUnit.SetOutputBusCount(1) || 
          !m_MixerUnit.SetInputFormat(&fmt)) // Same input format as the outpur from the converter
        return false;
      
      // Update format structure to reflect the desired format from the mixer
      fmt.mChannelsPerFrame = mixMap.GetOutputChannels(); // The output format of the mixer is identical to the input format, except for the channel count
      
      // Set-up the mixer output
      if (!m_MixerUnit.SetOutputFormat(&fmt) ||
        !m_MixerUnit.SetInputSource(&m_AUConverter) || // The mixer gets its data from the converter
        !m_MixerUnit.Initialize())
      return false;
      
      // Configure the mixing matrix
      Float32* val = (Float32*)mixMap;
      CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: Loading matrix mixer configuration");
      for (UInt32 i = 0; i < inputFormat.mChannelsPerFrame; ++i) 
      {
        for (UInt32 j = 0; j < fmt.mChannelsPerFrame; ++j) 
        {
          AudioUnitSetParameter(m_MixerUnit.GetComponent(),
            kMatrixMixerParam_Volume, kAudioUnitScope_Global, (i<<16) | j, *val++, 0);
          CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: \t[%d][%d][%0.1f]",
            (int)i, (int)j, *(val-1));
        }
      }
      
      CLog::Log(LOGDEBUG, "CCoreAudioRenderer::InitializePCM: "
        "Mixer Output Format: %d channels, %0.1f kHz, %d bits, %d bytes per frame",
        (int)fmt.mChannelsPerFrame, fmt.mSampleRate / 1000.0f, (int)fmt.mBitsPerChannel, (int)fmt.mBytesPerFrame);
      
      // Set-up the compander
      if (!m_AUCompressor.Open() ||
          !m_AUCompressor.SetOutputFormat(&fmt) ||
          !m_AUCompressor.SetInputFormat(&fmt) ||
          !m_AUCompressor.SetInputSource(&m_MixerUnit) || // The compressor gets its data from the mixer
          !m_AUCompressor.Initialize())
        return false;
      
      // Configure compander parameters
      // TODO: Uncomment when limiter params are pushed for other platforms
      m_AUCompressor.SetAttackTime(g_advancedSettings.m_limiterHold);
      m_AUCompressor.SetReleaseTime(g_advancedSettings.m_limiterRelease);
      
      // Copy format for the Output AU
      outputFormat = fmt;
    }
    else
    {
      outputFormat = inputFormat; // We don't know how to map this...let CoreAudio handle it
    }    
  }
  else 
  {
    outputFormat = inputFormat;
  }
  
  if (!m_AUOutput.SetInputFormat(&outputFormat))
    return false;
  
  m_BytesPerFrame = inputFormat.mBytesPerFrame;
  m_AvgBytesPerSec = inputFormat.mSampleRate * inputFormat.mBytesPerFrame;      // 1 sample per channel per frame
  m_EnableVolumeControl = true;
  
  return true;
}

bool CCoreAudioRenderer::InitializePCMEncoded(UInt32 sampleRate)
{
  m_AudioDevice.SetHogStatus(true); // Prevent any other application from using this device.
  m_AudioDevice.SetMixingSupport(false); // Try to disable mixing support. Effectiveness depends on the device.
  
  // Set the Sample Rate as defined by the spec.
  m_AudioDevice.SetNominalSampleRate((float)sampleRate);
  
  if (!InitializePCM(2, sampleRate, 16, NULL, false))
    return false;
  
  m_EnableVolumeControl = false; // Prevent attempts to change the output volume. It is not possible with encoded audio
  return true;
}

bool CCoreAudioRenderer::InitializeEncoded(AudioDeviceID outputDevice, UInt32 sampleRate)
{
  //return false; // un-comment to force PCM Spoofing (DD-Wav). For testing use only.
  
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
    
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: "
      "Found %s stream - id: 0x%04X, Terminal Type: 0x%04lX",
      stream.GetDirection() ? "Input" : "Output",
      (int)stream.GetId(),
      stream.GetTerminalType());
    
    // Probe physical formats
    StreamFormatList physicalFormats;
    stream.GetAvailablePhysicalFormats(&physicalFormats);
    while (!physicalFormats.empty())
    {
      AudioStreamRangedDescription& desc = physicalFormats.front();
      CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded:    Considering Physical Format: %s", StreamDescriptionToString(desc.mFormat, formatString));
      if (desc.mFormat.mFormatID == kAudioFormat60958AC3 && desc.mFormat.mSampleRate == sampleRate)
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
    CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: Unable to identify suitable output format.");
    return false;
  }
  
  m_ChunkLen = outputFormat.mBytesPerPacket; // 1 Chunk == 1 Packet
  m_AvgBytesPerSec = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3) * outputFormat.mSampleRate; // mBytesPerFrame is 0 for a cac3 stream
  m_BytesPerFrame = outputFormat.mChannelsPerFrame * (outputFormat.mBitsPerChannel>>3);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: Selected stream[%lu] - id: 0x%04lX, Physical Format: %s (%lu Bytes/sec.)", streamIndex, outputStream, StreamDescriptionToString(outputFormat, formatString), m_AvgBytesPerSec);
  
  // TODO: Auto hogging sets this for us. Figure out how/when to turn it off or use it
  // It appears that leaving this set will aslo restore the previous stream format when the
  // Application exits. If auto hogging is set and we try to set hog mode, we will deadlock
  // From the SDK docs: "If the AudioDevice is in a non-mixable mode, the HAL will automatically take hog mode on behalf of the first process to start an IOProc."
  
  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)
  
  CCoreAudioHardware::SetAutoHogMode(false); // Auto-Hog does not always un-hog the device when changing back to a mixable mode. Handle this on our own until it is fixed.
  bool autoHog = CCoreAudioHardware::GetAutoHogMode();
  CLog::Log(LOGDEBUG, " CoreAudioRenderer::InitializeEncoded: Auto 'hog' mode is set to '%s'.", autoHog ? "On" : "Off");
  if (!autoHog) // Try to handle this ourselves
  {
    m_AudioDevice.SetHogStatus(true); // Hog the device if it is not set to be done automatically
    m_AudioDevice.SetMixingSupport(false); // Try to disable mixing. If we cannot, it may not be a problem
  }
  m_NumLatencyFrames = m_AudioDevice.GetNumLatencyFrames();
  
  // Configure the output stream object
  m_OutputStream.Open(outputStream); // This is the one we will keep
  AudioStreamBasicDescription virtualFormat;
  m_OutputStream.GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: Previous Virtual Format: %s (%lu Bytes/sec.)", StreamDescriptionToString(virtualFormat, formatString), m_AvgBytesPerSec);
  AudioStreamBasicDescription previousPhysicalFormat;
  m_OutputStream.GetPhysicalFormat(&previousPhysicalFormat);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: Previous Physical Format: %s (%lu Bytes/sec.)", StreamDescriptionToString(previousPhysicalFormat, formatString), m_AvgBytesPerSec);
  m_OutputStream.SetPhysicalFormat(&outputFormat); // Set the active format (the old one will be reverted when we close)
  m_NumLatencyFrames += m_OutputStream.GetNumLatencyFrames();
  m_OutputStream.GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: New Virtual Format: %s (%lu Bytes/sec.)", StreamDescriptionToString(virtualFormat, formatString), m_AvgBytesPerSec);
  CLog::Log(LOGDEBUG, "CoreAudioRenderer::InitializeEncoded: New Physical Format: %s (%lu Bytes/sec.)", StreamDescriptionToString(outputFormat, formatString), m_AvgBytesPerSec);
  
  // Register for data request callbacks from the driver
  m_AudioDevice.AddIOProc(DirectRenderCallback, this);
  
  m_EnableVolumeControl = false; // Prevent attempts to change the output volume. It is not possible with encoded audio
  return true;
}

#endif
#endif

