#ifdef __APPLE__
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

#include "IOSAudioRenderer.h"
#include "AudioContext.h"
#include "GUISettings.h"
#include "Settings.h"
#include "threads/Atomics.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "lib/DllAvCodec.h"

//***********************************************************************************************
// Contruction/Destruction
//***********************************************************************************************
CIOSAudioRenderer::CIOSAudioRenderer() :
  m_Pause(false),
  m_Initialized(false),
  m_CurrentVolume(0),
  m_OutputBufferIndex(0),
  m_BytesPerSec(0),
  m_Buffer(NULL),
  m_NumChunks(0),
  m_PacketSize(0),
  m_Passthrough(false),
  m_SamplesPerSec(0),
  m_DoRunout(0)
{
  m_dllAvUtil = new DllAvUtil;
}

CIOSAudioRenderer::~CIOSAudioRenderer()
{
  Deinitialize();
  delete m_dllAvUtil;
}

//***********************************************************************************************
// Initialization
//***********************************************************************************************

bool CIOSAudioRenderer::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic /*Useless Legacy Parameter*/, bool bPassthrough)
{
  /* Taken from ALSA */
  /*
  static enum PCMChannels IOSChannelMap[6] =
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT};
  */
  // Limit to 2.0. It is only used for anloge audio.
  static enum PCMChannels IOSChannelMap[2] =
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT};

  // Have to clean house before we start again. TODO: Should we return failure instead?
  if (m_Initialized) 
    Deinitialize();

  if (!m_dllAvUtil->Load())
    CLog::Log(LOGERROR,"CIOSAudioRenderer::Initialize - failed to load avutil library!");

  m_Passthrough = bPassthrough;

  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);

  if(bPassthrough) {
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE_DIGITAL);
  } else {
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  }

  m_DataChannels = iChannels;
  m_remap.Reset();

  if (!m_Passthrough && channelMap)
  {
    enum PCMChannels *outLayout;
  
    /* set the input format, and get the channel layout so we know what we need to open */
    outLayout = m_remap.SetInputFormat (iChannels, channelMap, uiBitsPerSample / 8);
    unsigned int outChannels = 0;
    unsigned int ch = 0, map;
    while(outLayout[ch] != PCM_INVALID)
    {
      for(map = 0; map < 8; ++map)
      {
        if (outLayout[ch] == IOSChannelMap[map])
        {
          if (map > outChannels)
            outChannels = map;
          break;
        }
      }
      ++ch;
    }
  
    m_remap.SetOutputFormat(++outChannels, IOSChannelMap);
    if (m_remap.CanRemap())
    {
      iChannels = outChannels;
      if (m_DataChannels != (unsigned int)iChannels)
        CLog::Log(LOGDEBUG, "CIOSAudioRenderer::InitializePCM: Requested channels changed from %i to %i", m_DataChannels, iChannels);
    }    

  }
  
  m_Channels = iChannels;

  // Set the input stream format for the AudioUnit
  // We use the default DefaultOuput AudioUnit, so we only can set the input stream format.
  // The autput format is automaticaly set to the input format.
  AudioStreamBasicDescription audioFormat;
  audioFormat.mFormatID = kAudioFormatLinearPCM;              //	Data encoding format

  audioFormat.mFormatFlags = kAudioFormatFlagsCanonical;
  audioFormat.mChannelsPerFrame = iChannels;                  // Number of interleaved audiochannels
  audioFormat.mSampleRate = (Float64)uiSamplesPerSec;         //	the sample rate of the audio stream
  audioFormat.mBitsPerChannel = uiBitsPerSample;              // Number of bits per sample, per channel
  audioFormat.mBytesPerFrame = (uiBitsPerSample>>3) * iChannels; // Size of a frame == 1 sample per channel		
  audioFormat.mFramesPerPacket = 1;                           // The smallest amount of indivisible data. Always 1 for uncompressed audio 	
  audioFormat.mBytesPerPacket = audioFormat.mBytesPerFrame * audioFormat.mFramesPerPacket;
  audioFormat.mReserved = 0;

  // Attach our output object to the device
  if(!m_AudioDevice.Init(/*m_Passthrough*/ true, &audioFormat, RenderCallback, this))
  {
    CLog::Log(LOGDEBUG, "CIOSAudioRenderer::Init failed");
    return false;
  }

  m_BufferFrames = m_AudioDevice.FramesPerSlice(4096);
  if(!m_BufferFrames) 
  {
    CLog::Log(LOGDEBUG, "CIOSAudioRenderer::FramesPerSlice bufferFrames == 0\n");
    return false;
  }

  m_BytesPerFrame = audioFormat.mBytesPerFrame;
  m_BitsPerChannel = audioFormat.mBitsPerChannel;
  m_BytesPerSec = uiSamplesPerSec * (uiBitsPerSample / 8) * iChannels;
  m_SamplesPerSec = uiSamplesPerSec;
  m_PacketSize = m_BufferFrames;
  m_BufferLen = m_BytesPerSec;
  if(m_BytesPerSec < m_BytesPerFrame || m_BufferLen == 0)
    m_BufferLen = m_PacketSize;

  m_Buffer = m_dllAvUtil->av_fifo_alloc(m_BufferLen);
  if(!m_Buffer || !m_BufferLen)
  {
    CLog::Log(LOGDEBUG, "CIOSAudioRenderer::Initialize: Error allocation audio buffer size %d.", m_BufferLen);
    return false;
  }

  m_EnableVolumeControl = true;

  if (!m_AudioDevice.SetSessionListener(kAudioSessionProperty_AudioRouteChange, PropertyChangeCallback, this))
    return false;

  // Start the audio device
  if (!m_AudioDevice.Open())
    return false;

  // Suspend rendering. We will start once we have some data.
  m_Pause = true;
  m_Initialized = true;

  CLog::Log(LOGDEBUG, "CIOSAudioRenderer::Initialize: Renderer Configuration - Chunk Len: %u, Max Cache: %u (%0.0fms).", m_PacketSize, m_BufferLen, 1000.0 *(float)m_BufferLen/(float)m_BytesPerSec);
  CLog::Log(LOGINFO, "CIOSAudioRenderer::Initialize: Successfully configured audio output.");

  m_DoRunout = 0;

  return true;
}

bool CIOSAudioRenderer::Deinitialize()
{

  if(m_Buffer && m_Initialized)
    WaitCompletion();

  // Stop rendering
  Stop();

  //m_AudioDevice.Close();
  //Sleep(10);
  m_AudioDevice.Close();
  m_Initialized = false;
  m_BytesPerSec = 0;
  m_BufferLen = 0;
  m_NumChunks = 0;
  m_PacketSize = 0;
  m_SamplesPerSec = 0;
  m_DoRunout = 0;
  if (m_Buffer)
    m_dllAvUtil->av_fifo_free(m_Buffer);
  m_Buffer = NULL;

  CLog::Log(LOGINFO, "CIOSAudioRenderer::Deinitialize: Renderer has been shut down.");

  return true;
}

//***********************************************************************************************
// Transport control methods
//***********************************************************************************************
bool CIOSAudioRenderer::Pause()
{
  if (!m_Pause)
  {
    m_AudioDevice.Stop();
    m_Pause = true;
  }
  return true;
}

bool CIOSAudioRenderer::Resume()
{
  if (m_Pause)
  {
    m_AudioDevice.Start();
    m_Pause = false;
  }
  return true;
}

bool CIOSAudioRenderer::Stop()
{
  m_AudioDevice.Stop();

  m_Pause = true;

  return true;
}

//***********************************************************************************************
// Volume control methods
//***********************************************************************************************
LONG CIOSAudioRenderer::GetCurrentVolume() const
{
  return m_CurrentVolume;
}

void CIOSAudioRenderer::Mute(bool bMute)
{
}

bool CIOSAudioRenderer::SetCurrentVolume(LONG nVolume)
{
  return true;
}

//***********************************************************************************************
// Data management methods
//***********************************************************************************************
unsigned int CIOSAudioRenderer::GetSpace()
{
  int free = m_BufferLen - m_dllAvUtil->av_fifo_size(m_Buffer);
  return (free / m_Channels) * m_DataChannels;
}

unsigned int CIOSAudioRenderer::AddPackets(const void* data, DWORD len)
{

  int free = m_BufferLen - m_dllAvUtil->av_fifo_size(m_Buffer);

  len = (len / m_DataChannels) * m_Channels;

  if(len > free)
    return 0;

  //int length = std::min(free, (int)len);
  len = std::min(free, (int)len);
  int frames = len / m_Channels / (m_BitsPerChannel >> 3);

  if(frames == 0)
    return 0;

  // Call channel remapping routine if available available and required
  if(m_remap.CanRemap() && !m_Passthrough)
  {
    uint8_t outData[len];
    m_remap.Remap((void*)data, outData, frames);
    m_dllAvUtil->av_fifo_generic_write(m_Buffer, outData, len, NULL);
  }
  else
  {
    m_dllAvUtil->av_fifo_generic_write(m_Buffer, (unsigned char *)data, len, NULL);
  }

  Resume();
  
  return (len / m_Channels) * m_DataChannels;
}

float CIOSAudioRenderer::GetDelay()
{
  return (float)m_dllAvUtil->av_fifo_size(m_Buffer) / (float)m_BytesPerSec;
}

float CIOSAudioRenderer::GetCacheTime()
{
  return (float)(m_BufferLen - GetSpace()) / (float)m_BytesPerSec;
}

float CIOSAudioRenderer::GetCacheTotal()
{
  return (float)m_BufferLen / (float)m_BytesPerSec;
}

unsigned int CIOSAudioRenderer::GetChunkLen()
{
  //return (m_PacketSize / m_Channels) * m_DataChannels;
  return m_PacketSize;
}

void CIOSAudioRenderer::WaitCompletion()
{
  if (m_dllAvUtil->av_fifo_size(m_Buffer) == 0) // The cache is already empty. There is nothing to wait for.
    return;

  m_DoRunout = 1;
  
  UInt32 delay =  (UInt32)(GetDelay() * 1000.0f) + 10;
  if (!delay)
  {
    bool ret = m_RunoutEvent.WaitMSec(delay);
    if (!ret && m_dllAvUtil->av_fifo_size(m_Buffer) )
    {
      //See if there is still some data left in the cache that didn't get played
      CLog::Log(LOGERROR, "CIOSAudioRenderer::WaitCompletion: Timed-out waiting for runout. Remaining data will be truncated.");
    }
  }

  Stop();
}

//***********************************************************************************************
// Rendering Methods
//***********************************************************************************************
OSStatus CIOSAudioRenderer::OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  if (!m_Initialized) {
    CLog::Log(LOGERROR, "CIOSAudioRenderer::OnRender: Callback to de/unitialized renderer.");
    return noErr;
  }

  if(m_Pause) {
    return noErr;
  }

  UInt32 bytesRead = m_dllAvUtil->av_fifo_size(m_Buffer);
  UInt32 bytesRequested = inNumberFrames * m_BytesPerFrame;

  if (bytesRead < bytesRequested)
  {
    Pause(); // Stop further requests until we have more data.  The AddPackets method will resume playback
    m_RunoutEvent.Set(); // Tell anyone who cares that the cache is empty
    if (m_DoRunout) // We were waiting for a runout. This is not an error.
    {
      m_DoRunout = 0;
    }
  } 
  
    
  m_dllAvUtil->av_fifo_generic_read(m_Buffer, (unsigned char *)ioData->mBuffers[m_OutputBufferIndex].mData, bytesRequested, NULL);    

  if (!m_EnableVolumeControl && m_CurrentVolume <= VOLUME_MINIMUM)
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
  else
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = bytesRequested;
 
  /*
	// walk the samples
	for (int bufCount=0; bufCount<ioData->mNumberBuffers; bufCount++) {
		AudioBuffer buf = ioData->mBuffers[bufCount];
		// AudioSampleType* bufferedSample = (AudioSampleType*) &buf.mData;
		int currentFrame = 0;
		while ( currentFrame < inNumberFrames ) {
			// copy sample to buffer, across all channels
			for (int currentChannel=0; currentChannel<buf.mNumberChannels; currentChannel++) {
        //CLog::Log(LOGERROR, "CIOSAudioRenderer::OnRender: Channel %d size %ld.\n", currentChannel, sizeof(AudioSampleType));
        m_dllAvUtil->av_fifo_generic_read(m_Buffer, (unsigned char *)(buf.mData) + (currentFrame * 4) + (currentChannel*2), sizeof(AudioSampleType), NULL);
			}	
			currentFrame++;
		}
  }
  */
  
  return noErr;
}

OSStatus CIOSAudioRenderer::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  return ((CIOSAudioRenderer*)inRefCon)->OnRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

// Static Callback from AudioUnit
void  CIOSAudioRenderer::PropertyChanged(AudioSessionPropertyID inID, UInt32 inDataSize, const void* inPropertyValue)
{
  CLog::Log(LOGERROR, "CIOSAudioRenderer::PropertyChanged: inID %d.", (int)inID);
}

void  CIOSAudioRenderer::PropertyChangeCallback(void* inClientData, AudioSessionPropertyID inID, UInt32 inDataSize, const void* inPropertyValue)
{

  ((CIOSAudioRenderer*)inClientData)->PropertyChanged(inID, inDataSize, inPropertyValue);
}

#endif
