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
#include "IOSAudioRingBuffer.h"
#include "AudioContext.h"
#include "GUISettings.h"
#include "Settings.h"
#include "utils/log.h"

//***********************************************************************************************
// Contruction/Destruction
//***********************************************************************************************
CIOSAudioRenderer::CIOSAudioRenderer() :
  m_Pause(false),
  m_Initialized(false),
  m_CurrentVolume(0),
  m_OutputBufferIndex(0),
  m_BytesPerSec(0),
  m_NumChunks(0),
  m_PacketSize(0),
  m_Passthrough(false),
  m_SamplesPerSec(0),
  m_DoRunout(0)
{
  m_Buffer = new IOSAudioRingBuffer();
}

CIOSAudioRenderer::~CIOSAudioRenderer()
{
  Deinitialize();
  delete m_Buffer;
}

//***********************************************************************************************
// Initialization
//***********************************************************************************************

bool CIOSAudioRenderer::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic /*Useless Legacy Parameter*/, EEncoded bPassthrough)
{
  // Limit to 2.0. It is only used for anloge audio.
  static enum PCMChannels IOSChannelMap[2] =
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT};

  m_Passthrough = bPassthrough;

  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);

  if(bPassthrough)
  {
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
    outLayout = m_remap.SetInputFormat (iChannels, channelMap, uiBitsPerSample / 8, uiSamplesPerSec);
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

  m_PacketSize = iChannels * (uiBitsPerSample / 8) * 512;

  m_BufferFrames = m_AudioDevice.FramesPerSlice(m_PacketSize);
  if(!m_BufferFrames) 
  {
    CLog::Log(LOGDEBUG, "CIOSAudioRenderer::FramesPerSlice bufferFrames == 0\n");
    //return false;
  }

  m_BytesPerFrame = audioFormat.mBytesPerFrame;
  m_BitsPerChannel = audioFormat.mBitsPerChannel;
  m_BytesPerSec = uiSamplesPerSec * (uiBitsPerSample / 8) * iChannels;
  m_SamplesPerSec = uiSamplesPerSec;
  m_BufferLen = m_PacketSize * 96;
  if(m_BufferLen < m_PacketSize || m_BufferLen == 0)
    m_BufferLen = m_PacketSize;

  bool success = m_Buffer->Create(m_BufferLen);
  if(!success || !m_BufferLen)
  {
    CLog::Log(LOGDEBUG, "CIOSAudioRenderer::Initialize: Error allocation audio buffer size %d.", m_BufferLen);
    return false;
  }

  m_EnableVolumeControl = true;

  /*
  if (!m_AudioDevice.SetSessionListener(kAudioSessionProperty_AudioRouteChange, PropertyChangeCallback, this))
    return false;
  */

  // Start the audio device
  if (!m_AudioDevice.Open())
    return false;

  // Suspend rendering. We will start once we have some data.
  m_Pause = true;
  m_Initialized = true;

  CLog::Log(LOGDEBUG, "CIOSAudioRenderer::Initialize: Renderer Configuration - Chunk Len: %u, Max Cache: %u (%0.0fms).", m_PacketSize, m_BufferLen, 1000.0 *(float)m_BufferLen/(float)m_BytesPerSec);
  CLog::Log(LOGINFO, "CIOSAudioRenderer::Initialize: Successfully configured audio output.");

  m_DoRunout = 0;

  m_drc      = 0;

  return true;
}

bool CIOSAudioRenderer::Deinitialize()
{

  if(m_Initialized)
    WaitCompletion();

  // Stop rendering
  Stop();

  Sleep(10);
  m_AudioDevice.Close();
  m_Initialized = false;
  m_BytesPerSec = 0;
  m_BufferLen   = 0;
  m_NumChunks   = 0;
  m_PacketSize  = 0;
  m_SamplesPerSec = 0;
  m_DoRunout    = 0;

  CLog::Log(LOGINFO, "CIOSAudioRenderer::Deinitialize: Renderer has been shut down.");

  return true;
}

void CIOSAudioRenderer::Flush()
{
  Pause();

  // IOSAudioRingBuffer::Reset is not threadsafe but we have
  // paused here so renderer is not reading from m_Buffer and
  // we can reset with confidence.
  m_Buffer->Reset();
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

  Flush();
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
  int free = m_Buffer->GetWriteSize();
  return (free / m_Channels) * m_DataChannels;
}

unsigned int CIOSAudioRenderer::AddPackets(const void* data, DWORD len)
{
  int status;

  // call channel remapping routine if available and required
  if (m_remap.CanRemap() && !m_Passthrough)
  {
    int length, frames;

    // we might be up or down converting, so convert to number of bytes
    // that we will get out of remapping the channels and see if that fits. 
    length = (len / m_DataChannels) * m_Channels;
    if (length > GetSpace())
      return 0;

    // check buffer fit, we can only accept unit frames, so if less than
    // a complete frame, we punt.
    frames = length / m_Channels / (m_BitsPerChannel >> 3);
    if (frames == 0)
    {
      CLog::Log(LOGINFO, "IOSAudioRenderer::AddPackets() - Need complete frame.");
      return 0;
    }

    uint8_t outData[length];
    // remap the audio channels using the frame count
    m_remap.Remap((void*)data, outData, frames, m_drc);

    status = m_Buffer->Write(outData, length);
    // return the number of input bytes we accepted
    len = (length / m_Channels) * m_DataChannels;
  }
  else
  {
    // simple case, not remaping or passthough, only have to check 
    // that we have free space in our buffer.
    status = m_Buffer->Write((unsigned char *)data, len);
  }

  Resume();
  //only return the length if buffer accepted the data
  return status == 0 ? len : 0;
}

float CIOSAudioRenderer::GetDelay()
{
  return (float)m_Buffer->GetReadSize() / (float)m_BytesPerSec;
}

float CIOSAudioRenderer::GetCacheTime()
{
  unsigned int nBufferLenFull = (m_BufferLen / m_Channels) * m_DataChannels;
  return (float)(nBufferLenFull - GetSpace()) / (float)m_BytesPerSec;
}

float CIOSAudioRenderer::GetCacheTotal()
{
  return (float)m_BufferLen / (float)m_BytesPerSec;
}

unsigned int CIOSAudioRenderer::GetChunkLen()
{
  return (m_PacketSize / m_Channels) * m_DataChannels;
}

void CIOSAudioRenderer::WaitCompletion()
{
  // we don't lock here as we are just checking for zero or non-zero.

  // The cache is already empty. There is nothing to wait for.
  if (m_Buffer->GetReadSize() == 0)
    return;

  m_DoRunout = 1;
  
  UInt32 delay =  (UInt32)(GetDelay() * 1000.0f) + 10;
  if (!delay)
  {
    bool ret = m_RunoutEvent.WaitMSec(delay);
    if (!ret && m_Buffer->GetReadSize() )
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
  if (!m_Initialized)
  {
    CLog::Log(LOGERROR, "CIOSAudioRenderer::OnRender: Callback to de/unitialized renderer.");
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
    return noErr;
  }

  if(m_Pause)
  {
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
    return noErr;
  }

  UInt32 bytesRead = m_Buffer->GetReadSize();
  UInt32 bytesRequested = inNumberFrames * m_BytesPerFrame;

  if (bytesRead < bytesRequested)
  {
    m_RunoutEvent.Set(); // Tell anyone who cares that the cache is empty
    if (m_DoRunout) // We were waiting for a runout. This is not an error.
    {
      m_DoRunout = 0;
    }
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
    return noErr;
  }

  m_Buffer->Read((unsigned char *)ioData->mBuffers[m_OutputBufferIndex].mData, bytesRequested);

  if (!m_EnableVolumeControl && m_CurrentVolume <= VOLUME_MINIMUM)
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = 0;
  else
    ioData->mBuffers[m_OutputBufferIndex].mDataByteSize = bytesRequested;

  return noErr;
}

OSStatus CIOSAudioRenderer::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  return ((CIOSAudioRenderer*)inRefCon)->OnRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

// Static Callback from AudioUnit
void CIOSAudioRenderer::PropertyChanged(AudioSessionPropertyID inID, UInt32 inDataSize, const void* inPropertyValue)
{
  CLog::Log(LOGERROR, "CIOSAudioRenderer::PropertyChanged: inID %d.", (int)inID);
}

void  CIOSAudioRenderer::PropertyChangeCallback(void* inClientData, AudioSessionPropertyID inID, UInt32 inDataSize, const void* inPropertyValue)
{
  ((CIOSAudioRenderer*)inClientData)->PropertyChanged(inID, inDataSize, inPropertyValue);
}

#endif
