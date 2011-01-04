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
#include "cores/AudioEngine/AEFactory.h"

using namespace std;

CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioStream = NULL;
  m_iBufferSize = 0;
  m_dwPacketSize = 0;
  m_pBuffer = NULL;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_iChannels = 0;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
  {
    m_pAudioStream->Destroy();
  }
  free(m_pBuffer);
}

bool CDVDAudio::Create(const DVDAudioFrame &audioframe, CodecID codec)
{
  CLog::Log(LOGNOTICE,
    "Creating audio stream (codec id: %i, channels: %i, sample rate: %i, %s)",
    codec,
    audioframe.channel_count,
    audioframe.sample_rate,
    audioframe.passthrough ? "pass-through" : "no pass-through"
  );

  // if passthrough isset do something else
  CSingleLock lock(m_critSection);
  m_pAudioStream = AE.GetStream(
    audioframe.data_format,
    audioframe.sample_rate,
    audioframe.channel_count,
    audioframe.channel_layout
  );
  if (!m_pAudioStream) return false;

  m_iChannels      = audioframe.channel_count;
  m_iBitrate       = audioframe.sample_rate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough   = audioframe.passthrough;
  m_dwPacketSize   = m_pAudioStream->GetFrameSize();

  if (m_pBuffer) free(m_pBuffer);
  m_pBuffer = (BYTE*)malloc(m_dwPacketSize);

  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioStream)
  {
    m_pAudioStream->Destroy();
  }
  free(m_pBuffer);
  m_pBuffer = NULL;
  m_dwPacketSize = 0;
  m_pAudioStream = NULL;
  m_iBufferSize = 0;
  m_iChannels = 0;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
}

DWORD CDVDAudio::AddPacketsRenderer(unsigned char* data, DWORD len, CSingleLock &lock)
{
  //Since we write same data size each time, we can drop full chunks to simulate a specific playback speed
  //m_iSpeedStep = (m_iSpeedStep+1) % m_iSpeed;
  //if( m_iSpeedStep )
  //  return m_dwPacketSize;
  //else

  if(!m_pAudioStream)
    return 0;

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);
  if(!bps)
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioStream->GetDelay() + (double)len / bps);
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += CDVDClock::GetAbsoluteClock();

  DWORD  total = len;
  DWORD  copied;
  do
  {
    copied = m_pAudioStream->AddData(data, len);
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

DWORD CDVDAudio::AddPackets(const DVDAudioFrame &audioframe)
{
  CSingleLock lock (m_critSection);

  unsigned char* data = audioframe.data;
  DWORD len = audioframe.size;

  DWORD total = len;
  DWORD copied;

  if (m_iBufferSize > 0) // See if there are carryover bytes from the last call. need to add them 1st.
  {
    copied = std::min(m_dwPacketSize - m_iBufferSize, len); // Smaller of either the data provided or the leftover data

    memcpy(m_pBuffer + m_iBufferSize, data, copied); // Tack the caller's data onto the end of the buffer
    data += copied; // Move forward in caller's data
    len -= copied; // Decrease amount of data available from caller
    m_iBufferSize += copied; // Increase amount of data available in buffer

    if(m_iBufferSize < m_dwPacketSize) // If we don't have enough data to give to the renderer, wait until next time
      return copied;

    if(AddPacketsRenderer(m_pBuffer, m_iBufferSize, lock) != m_iBufferSize)
    {
      m_iBufferSize = 0;
      CLog::Log(LOGERROR, "%s - failed to add leftover bytes to render", __FUNCTION__);
      return copied;
    }

    m_iBufferSize = 0;
    if (!len)
      return copied; // We used up all the caller's data
  }

  copied = AddPacketsRenderer(data, len, lock);
  data += copied;
  len -= copied;

  // if we have more data left, save it for the next call to this funtion
  if (len > 0 && !m_bStop)
  {
    m_pBuffer     = (BYTE*)realloc(m_pBuffer, len);
    m_iBufferSize = len;
    memcpy(m_pBuffer, data, len);
  }
  return total;
}

void CDVDAudio::Finish()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioStream)
    return;

  DWORD silence = m_dwPacketSize - m_iBufferSize % m_dwPacketSize;

  if(silence > 0 && m_iBufferSize > 0)
  {
    CLog::Log(LOGDEBUG, "CDVDAudio::Drain - adding %d bytes of silence, buffer size: %d, chunk size: %d", silence, m_iBufferSize, m_dwPacketSize);
    m_pBuffer = (BYTE*)realloc(m_pBuffer, m_dwPacketSize);
    memset(m_pBuffer+m_iBufferSize, 0, silence);
    m_iBufferSize += silence;
  }

  if(AddPacketsRenderer(m_pBuffer, m_iBufferSize, lock) != m_iBufferSize)
    CLog::Log(LOGERROR, "CDVDAudio::Drain - failed to play the final %d bytes", m_iBufferSize);

  m_iBufferSize = 0;
}

void CDVDAudio::Drain()
{
  Finish();
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Drain();
}

void CDVDAudio::SetVolume(float volume)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream) m_pAudioStream->SetVolume(volume);
}

void CDVDAudio::Pause()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream) m_pAudioStream->Pause();
}

void CDVDAudio::Resume()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream) m_pAudioStream->Resume();
}

double CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);

  double delay = 0.0;
  if(m_pAudioStream)
    delay = m_pAudioStream->GetDelay();

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);
  if(m_iBufferSize && bps)
    delay += (double)m_iBufferSize / bps;

  return delay * DVD_TIME_BASE;
}

void CDVDAudio::Flush()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioStream)
  {
    m_pAudioStream->Flush();
  }
  m_iBufferSize = 0;
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioStream)
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(audioframe.channel_count   != m_iChannels
  || audioframe.sample_rate     != m_iBitrate
  || audioframe.bits_per_sample != m_iBitsPerSample)
    return false;

  //FIXME: compare channel layout

  return true;
}

double CDVDAudio::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioStream)
    return 0.0;
  // FIXME: whats this for?, doesn't GetDelay handle this
  return 0.0;
  //return m_pAudioStream->GetCacheTime();
}

double CDVDAudio::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioStream)
    return 0.0;
  // FIXME: whats this for?, doesn't GetDelay handle this
  return 0.0;
  //return m_pAudioDecoder->GetCacheTotal();
}
