/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "utils/SingleLock.h"
#include "utils/log.h"
#include "DVDAudio.h"
#include "Util.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDPlayerAudio.h"
#include "../AudioRenderers/AudioRendererFactory.h"

using namespace std;

CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioDecoder = NULL;
  m_pCallback = NULL;
  m_iOverflowBufferSize = 0;
  m_dwPacketSize = 0;
  m_pOverflowBuffer = NULL;
  m_iSpeed = 0;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_iChannels = 0;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }
  delete[] m_pOverflowBuffer;
}

void CDVDAudio::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  m_pCallback = pCallback;
  if (m_pCallback && m_pAudioDecoder && !m_bPassthrough)
    m_pCallback->OnInitialize(m_iChannels, m_iBitrate, m_iBitsPerSample);
}

void CDVDAudio::UnRegisterAudioCallback()
{
  CSingleLock lock (m_critSection);
  m_pCallback = NULL;
}

bool CDVDAudio::Create(const DVDAudioFormat &format, CodecID codec)
{
  CSingleLock lock (m_critSection);

  const char* codecstring="";

  if(codec == CODEC_ID_AAC)
    codecstring = "AAC";
  else if (codec == CODEC_ID_VORBIS)
    codecstring = "Vorbis";
  else if (codec == CODEC_ID_AC3)
    codecstring = "AC3";
  else if (codec == CODEC_ID_DTS)
    codecstring = "DTS";
  else if (codec == CODEC_ID_EAC3)
    codecstring = "EAC3";
  else
    codecstring = "PCM";

  switch (format.streamType)
  {
    case DVDAudioStreamType_PCM:
    {
      // Set bit-depth based on sample format
      int bitsPerSample = 0;
      if (format.pcm.sampleType == DVDAudioPCMSampleType_S16LE)
        bitsPerSample = 16;
      else 
        return false; // Unsupported sample type
      
      // Create audio renderer
      CLog::Log(LOGNOTICE, "Creating audio device with codec id: %i, channels: %i, sample rate: %i", codec, format.pcm.channels, format.bitrate);
      m_pAudioDecoder = CAudioRendererFactory::Create(m_pCallback, format.pcm.channels, format.pcm.channel_map,  format.bitrate / format.pcm.channels /bitsPerSample, bitsPerSample, false, codecstring, false, false);
      if (!m_pAudioDecoder) 
        return false;
      m_iBitsPerSample =  bitsPerSample;
      m_bPassthrough = false;
      m_iChannels = format.pcm.channels;
      break;
    }
    case DVDAudioStreamType_Encoded:
      CLog::Log(LOGNOTICE, "Creating passthrough audio device with codec id: %i bit rate: %i", codec, format.bitrate);
      m_pAudioDecoder = CAudioRendererFactory::Create(m_pCallback, 0, NULL, 48000, 0, false, codecstring, false, true); // TODO: Need better way to set sample/bit rate
      if (!m_pAudioDecoder) 
        return false;
      m_iBitsPerSample = 0;
      m_bPassthrough = true;
      break;
    default:
      CLog::Log(LOGERROR, "Invalid DVDAudioOutputFormat specified. codec: %i format: %i", codec, format.encoded.encodingType);
      return false; // Unknown format
  }
  
  m_iBitrate = format.bitrate;
  
  m_dwPacketSize = m_pAudioDecoder->GetChunkLen();
  if (m_pOverflowBuffer) 
    delete[] m_pOverflowBuffer;
  m_pOverflowBuffer = new BYTE[m_dwPacketSize];
  
  if(m_pCallback && !m_bPassthrough)
    m_pCallback->OnInitialize(m_iChannels, m_iBitrate / m_iChannels / m_iBitsPerSample, m_iBitsPerSample);

  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Stop();
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }

  delete[] m_pOverflowBuffer;
  m_pOverflowBuffer = NULL;
  
  m_dwPacketSize = 0;
  m_pAudioDecoder = NULL;
  m_iOverflowBufferSize = 0;
  m_iChannels = 0;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_iSpeed = 1;
}

void CDVDAudio::SetSpeed(int iSpeed)
{
  m_iSpeed = abs(iSpeed);

}

DWORD CDVDAudio::AddPacketsRenderer(const unsigned char* data, DWORD len, CSingleLock &lock)
{
  //Since we write same data size each time, we can drop full chunks to simulate a specific playback speed
  //m_iSpeedStep = (m_iSpeedStep+1) % m_iSpeed;
  //if( m_iSpeedStep )
  //  return m_dwPacketSize;
  //else

  if(!m_pAudioDecoder)
    return 0;

  if(!m_iBitrate)
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioDecoder->GetDelay() + (double)len / m_iBitrate);
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += CDVDClock::GetAbsoluteClock();

  DWORD  total = len;
  DWORD  copied;
  do
  {
    copied = m_pAudioDecoder->AddPackets(data, len);
    data += copied;
    len -= copied;
    if (len < m_dwPacketSize)
      break;

    if (copied == 0 && timeout < CDVDClock::GetAbsoluteClock())
    {
      CLog::Log(LOGERROR, "CDVDAudio::AddPacketsRenderer - timeout adding data to renderer");
      break;
    }

    lock.Leave();
    Sleep(1);
    lock.Enter();
  } while (!m_bStop);

  return total - len;
}

// TODO: Need a cleaner and more flexible mechanism to pass in data
//   audioframe should only use float data, so we may need another structure
//   that can carry multiple data types
DWORD CDVDAudio::AddPackets(const unsigned char* data, unsigned int len)
{
  CSingleLock lock (m_critSection);

  DWORD total = len;
  DWORD copied;

  //Feed audio to the visualizer if necessary.
  if(m_pCallback && !m_bPassthrough)
    m_pCallback->OnAudioData(data, len);

  if (m_iOverflowBufferSize > 0) // See if there are carryover bytes from the last call. need to add them 1st.
  {
    copied = std::min(m_dwPacketSize - m_iOverflowBufferSize, len); // Smaller of either the data provided or the leftover data

    memcpy(m_pOverflowBuffer + m_iOverflowBufferSize, data, copied); // Tack the caller's data onto the end of the buffer
    data += copied; // Move forward in caller's data
    len -= copied; // Decrease amount of data available from caller
    m_iOverflowBufferSize += copied; // Increase amount of data available in buffer

    if(m_iOverflowBufferSize < m_dwPacketSize) // If we don't have enough data to give to the renderer, wait until next time
      return copied;

    if(AddPacketsRenderer(m_pOverflowBuffer, m_iOverflowBufferSize, lock) != m_iOverflowBufferSize)
    {
      m_iOverflowBufferSize = 0;
      CLog::Log(LOGERROR, "%s - failed to add leftover bytes to render", __FUNCTION__);
      return copied;
    }

    m_iOverflowBufferSize = 0;
    if (!len)
      return copied; // We used up all the caller's data
  }

  copied = AddPacketsRenderer(data, len, lock);
  data += copied;
  len -= copied;

  // if we have more data left, save it for the next call to this funtion
  if (len > 0 && !m_bStop)
  {
    if(len > m_dwPacketSize)
      CLog::Log(LOGERROR, "%s - More bytes left than can be stored in buffer", __FUNCTION__);

    m_iOverflowBufferSize = std::min(len, m_dwPacketSize);
    memcpy(m_pOverflowBuffer, data, m_iOverflowBufferSize);
    len  -= m_iOverflowBufferSize;
    data += m_iOverflowBufferSize;
  }
  return total - len;
}

void CDVDAudio::Finish()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioDecoder)
    return;

  DWORD silence = m_dwPacketSize - m_iOverflowBufferSize % m_dwPacketSize;

  if(silence > 0 && m_iOverflowBufferSize > 0)
  {
    CLog::Log(LOGDEBUG, "CDVDAudio::Drain - adding %d bytes of silence, buffer size: %d, chunk size: %d", silence, m_iOverflowBufferSize, m_dwPacketSize);
    memset(m_pOverflowBuffer+m_iOverflowBufferSize, 0, silence);
    m_iOverflowBufferSize += silence;
  }

  if(AddPacketsRenderer(m_pOverflowBuffer, m_iOverflowBufferSize, lock) != m_iOverflowBufferSize)
    CLog::Log(LOGERROR, "CDVDAudio::Drain - failed to play the final %d bytes", m_iOverflowBufferSize);

  m_iOverflowBufferSize = 0;
}

void CDVDAudio::Drain()
{
  Finish();
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
    m_pAudioDecoder->WaitCompletion();
}

void CDVDAudio::SetVolume(int iVolume)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->SetCurrentVolume(iVolume);
}

void CDVDAudio::SetDynamicRangeCompression(long drc)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->SetDynamicRangeCompression(drc);
}

void CDVDAudio::Pause()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->Pause();
}

void CDVDAudio::Resume()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->Resume();
}

double CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);

  double delay = 0.0;
  if(m_pAudioDecoder)
    delay = m_pAudioDecoder->GetDelay();

  if(m_iOverflowBufferSize && m_iBitrate)
    delay += (double)m_iOverflowBufferSize / m_iBitrate;

  return delay * DVD_TIME_BASE;
}

void CDVDAudio::Flush()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Stop();
    m_pAudioDecoder->Resume();
  }
  m_iOverflowBufferSize = 0;
}

bool CDVDAudio::IsValidFormat(const DVDAudioFormat& format)
{
  if(!m_pAudioDecoder)
    return false;
  
  if((format.streamType == DVDAudioStreamType_Encoded) && !m_bPassthrough)
    return false;

  if (format.bitrate != (unsigned int)m_iBitrate)
    return false;

  if(format.pcm.channels != m_iChannels)
    return false;

  return true;
}

double CDVDAudio::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioDecoder)
    return 0.0;
  return m_pAudioDecoder->GetCacheTime();
}

double CDVDAudio::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioDecoder)
    return 0.0;
  return m_pAudioDecoder->GetCacheTotal();
}
