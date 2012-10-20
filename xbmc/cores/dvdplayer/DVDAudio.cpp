/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SingleLock.h"
#include "utils/log.h"
#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDPlayerAudio.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "settings/Settings.h"

using namespace std;


CPTSOutputQueue::CPTSOutputQueue()
{
  Flush();
}

void CPTSOutputQueue::Add(double pts, double delay, double duration)
{
  CSingleLock lock(m_sync);

  // don't accept a re-add, since that would cause time moving back
  double last = m_queue.empty() ? m_current.pts : m_queue.back().pts;
  if(last == pts)
    return;

  TPTSItem item;
  item.pts = pts;
  item.timestamp = CDVDClock::GetAbsoluteClock() + delay;
  item.duration = duration;

  // first one is applied directly
  if(m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
    m_current = item;
  else
    m_queue.push(item);

  // call function to make sure the queue
  // doesn't grow should nobody call it
  Current();
}
void CPTSOutputQueue::Flush()
{
  CSingleLock lock(m_sync);

  while( !m_queue.empty() ) m_queue.pop();
  m_current.pts = DVD_NOPTS_VALUE;
  m_current.timestamp = 0.0;
  m_current.duration = 0.0;
}

double CPTSOutputQueue::Current()
{
  CSingleLock lock(m_sync);

  if(!m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  while( !m_queue.empty() && CDVDClock::GetAbsoluteClock() >= m_queue.front().timestamp )
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  if( m_current.timestamp == 0 ) return m_current.pts;

  return m_current.pts + min(m_current.duration, (CDVDClock::GetAbsoluteClock() - m_current.timestamp));
}


CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioStream = NULL;
  m_pAudioCallback = NULL;
  m_iBufferSize = 0;
  m_dwPacketSize = 0;
  m_pBuffer = NULL;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_SecondsPerByte = 0.0;
  m_bPaused = true;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    CAEFactory::FreeStream(m_pAudioStream);

  free(m_pBuffer);
}

bool CDVDAudio::Create(const DVDAudioFrame &audioframe, CodecID codec, bool needresampler)
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
  unsigned int options = needresampler && !audioframe.passthrough ? AESTREAM_FORCE_RESAMPLE : 0;
  options |= AESTREAM_AUTOSTART;

  m_pAudioStream = CAEFactory::MakeStream(
    audioframe.data_format,
    audioframe.sample_rate,
    audioframe.encoded_sample_rate,
    audioframe.channel_layout,
    options
  );
  if (!m_pAudioStream) return false;

  m_iBitrate       = audioframe.sample_rate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough   = audioframe.passthrough;
  m_channelLayout  = audioframe.channel_layout;
  m_dwPacketSize   = m_pAudioStream->GetFrameSize();

  if(m_channelLayout.Count() && m_iBitrate && m_iBitsPerSample)
    m_SecondsPerByte = 1.0 / (m_channelLayout.Count() * m_iBitrate * (m_iBitsPerSample>>3));
  else
    m_SecondsPerByte = 0.0;

  m_iBufferSize = 0;
  SetDynamicRangeCompression((long)(g_settings.m_currentVideoSettings.m_VolumeAmplification * 100));

  if (m_pAudioCallback)
    RegisterAudioCallback(m_pAudioCallback);

  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioStream)
    CAEFactory::FreeStream(m_pAudioStream);

  free(m_pBuffer);
  m_pBuffer = NULL;
  m_dwPacketSize = 0;
  m_pAudioStream = NULL;
  m_iBufferSize = 0;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_bPaused = true;
  m_time.Flush();
}

DWORD CDVDAudio::AddPacketsRenderer(unsigned char* data, DWORD len, CSingleLock &lock)
{
  if(!m_pAudioStream)
    return 0;

  DWORD bps = m_channelLayout.Count() * m_iBitrate * (m_iBitsPerSample>>3);
  if(!bps)
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioStream->GetDelay() + len * m_SecondsPerByte);
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
    copied = std::min(m_dwPacketSize - m_iBufferSize % m_dwPacketSize, len); // Smaller of either the data provided or the leftover data
    if(copied)
    {
      m_pBuffer = (BYTE*)realloc(m_pBuffer, m_iBufferSize + copied);
      memcpy(m_pBuffer + m_iBufferSize, data, copied); // Tack the caller's data onto the end of the buffer
      data += copied; // Move forward in caller's data
      len -= copied; // Decrease amount of data available from caller
      m_iBufferSize += copied; // Increase amount of data available in buffer
    }

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

  double time_added = DVD_SEC_TO_TIME(m_SecondsPerByte * (data - audioframe.data));
  m_time.Add(audioframe.pts, GetDelay() - time_added, audioframe.duration);

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
    m_pBuffer = (BYTE*)realloc(m_pBuffer, m_iBufferSize + silence);
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

void CDVDAudio::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  m_pAudioCallback = pCallback;
  if (m_pAudioStream)
    m_pAudioStream->RegisterAudioCallback(pCallback);
}

void CDVDAudio::UnRegisterAudioCallback()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->UnRegisterAudioCallback();
  m_pAudioCallback = NULL;
}

void CDVDAudio::SetVolume(float volume)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream) m_pAudioStream->SetVolume(volume);
}

void CDVDAudio::SetDynamicRangeCompression(long drc)
{

}

float CDVDAudio::GetCurrentAttenuation()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    return m_pAudioStream->GetVolume();
  else
    return 1.0f;
}

void CDVDAudio::Pause()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream) m_pAudioStream->Pause();
  m_time.Flush();
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

  delay += m_SecondsPerByte * m_iBufferSize;

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
  m_time.Flush();
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioStream)
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(m_iBitrate       != audioframe.sample_rate
  || m_iBitsPerSample != audioframe.bits_per_sample
  || m_channelLayout  != audioframe.channel_layout)
    return false;

  return true;
}

void CDVDAudio::SetResampleRatio(double ratio)
{
  CSingleLock lock (m_critSection);

  if(m_pAudioStream)
    m_pAudioStream->SetResampleRatio(ratio);
}

double CDVDAudio::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioStream)
    return 0.0;

  double delay = 0.0;
  if(m_pAudioStream)
    delay = m_pAudioStream->GetCacheTime();

  delay += m_SecondsPerByte * m_iBufferSize;

  return delay;
}

double CDVDAudio::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioStream)
    return 0.0;
  return m_pAudioStream->GetCacheTotal();
}

void CDVDAudio::SetPlayingPts(double pts)
{
  CSingleLock lock (m_critSection);
  m_time.Flush();
  m_time.Add(pts, GetDelay(), 0);
}
