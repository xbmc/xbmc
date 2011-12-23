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

#include "threads/SingleLock.h"
#include "utils/log.h"
#include "DVDAudio.h"
#include "Util.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDPlayerAudio.h"
#include "../AudioRenderers/AudioRendererFactory.h"
#include "settings/Settings.h"

using namespace std;

CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioDecoder = NULL;
  m_pCallback = NULL;
  m_iBufferSize = 0;
  m_dwPacketSize = 0;
  m_pBuffer = NULL;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_iChannels = 0;
  m_SecondsPerByte = 0.0;
  m_bPaused = true;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }
  free(m_pBuffer);
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

bool CDVDAudio::Create(const DVDAudioFrame &audioframe, CodecID codec)
{
  CLog::Log(LOGNOTICE, "Creating audio device with codec id: %i, channels: %i, sample rate: %i, %s", codec, audioframe.channels, audioframe.sample_rate, audioframe.passthrough ? "pass-through" : "no pass-through");

  // if passthrough isset do something else
  CSingleLock lock (m_critSection);

  IAudioRenderer::EEncoded encoded = IAudioRenderer::ENCODED_NONE;
  if(audioframe.passthrough)
  {
    switch(codec) {
      case CODEC_ID_AC3 : encoded = IAudioRenderer::ENCODED_IEC61937_AC3;     break;
      case CODEC_ID_EAC3: encoded = IAudioRenderer::ENCODED_IEC61937_EAC3;    break;
      case CODEC_ID_DTS : encoded = IAudioRenderer::ENCODED_IEC61937_DTS;     break;
      case CODEC_ID_MP1 :
      case CODEC_ID_MP2 :
      case CODEC_ID_MP3 : encoded = IAudioRenderer::ENCODED_IEC61937_MPEG;    break;
      default:            encoded = IAudioRenderer::ENCODED_IEC61937_UNKNOWN; break;
    }
  }

  m_pAudioDecoder = CAudioRendererFactory::Create(m_pCallback, audioframe.channels, audioframe.channel_map, audioframe.sample_rate, audioframe.bits_per_sample, false, false, encoded);

  if (!m_pAudioDecoder) return false;

  m_iChannels = audioframe.channels;
  m_iBitrate = audioframe.sample_rate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough = audioframe.passthrough;
  if(m_iChannels && m_iBitrate && m_iBitsPerSample)
    m_SecondsPerByte = 1.0 / (m_iChannels * m_iBitrate * (m_iBitsPerSample>>3));
  else
    m_SecondsPerByte = 0.0;

  m_dwPacketSize = m_pAudioDecoder->GetChunkLen();
  if(m_bPaused)
    m_pAudioDecoder->Pause();

  m_iBufferSize = 0;

  if(m_pCallback && !m_bPassthrough)
    m_pCallback->OnInitialize(m_iChannels, m_iBitrate, m_iBitsPerSample);

  SetDynamicRangeCompression((long)(g_settings.m_currentVideoSettings.m_VolumeAmplification * 100));

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
  free(m_pBuffer);
  m_pBuffer = NULL;
  m_dwPacketSize = 0;
  m_pAudioDecoder = NULL;
  m_iBufferSize = 0;
  m_iChannels = 0;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_bPaused = true;
}

DWORD CDVDAudio::AddPacketsRenderer(unsigned char* data, DWORD len, CSingleLock &lock)
{
  if(!m_pAudioDecoder)
    return 0;

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);
  if(!bps)
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioDecoder->GetDelay() + len * m_SecondsPerByte);
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

DWORD CDVDAudio::AddPackets(const DVDAudioFrame &audioframe)
{
  CSingleLock lock (m_critSection);

  unsigned char* data = audioframe.data;
  DWORD len = audioframe.size;

  DWORD total = len;
  DWORD copied;

  //Feed audio to the visualizer if necessary.
  if(m_pCallback && !m_bPassthrough)
    m_pCallback->OnAudioData(data, len);

  // When paused, we need to buffer all data as renderers don't need to accept it
  if (m_bPaused)
  {
    m_pBuffer = (BYTE*)realloc(m_pBuffer, m_iBufferSize + len);
    memcpy(m_pBuffer+m_iBufferSize, data, len);
    m_iBufferSize += len;
    return len;
  }

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
  return total;
}

double CDVDAudio::AddSilence(double delay)
{
  CLog::Log(LOGDEBUG, "CDVDAudio::AddSilence - %f seconds", delay);
  DVDAudioFrame audioframe;
  audioframe.passthrough     = m_bPassthrough;
  audioframe.channels        = m_iChannels;
  audioframe.sample_rate     = m_iBitrate;
  audioframe.bits_per_sample = m_iBitsPerSample;
  audioframe.size            = m_iChannels * (m_iBitsPerSample>>3);
  audioframe.data            = (BYTE*)calloc(1, audioframe.size);
  if(audioframe.data == NULL)
    return 0.0;
  unsigned samples = m_iBitrate * delay;
  unsigned added = 0;
  for(; added < samples; added++)
  {
    if(AddPackets(audioframe) != audioframe.size)
      break;
  }
  if(added < samples)
    CLog::Log(LOGDEBUG, "CDVDAudio::AddSilence - failed to %d silence samples of %u", samples - added, samples);
  free(audioframe.data);
  return (double)added / m_iBitrate;
}

void CDVDAudio::Finish()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioDecoder)
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

float CDVDAudio::GetCurrentAttenuation()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
    return m_pAudioDecoder->GetCurrentAttenuation();
  else
    return 1.0f;
}

void CDVDAudio::Pause()
{
  CSingleLock lock (m_critSection);
  m_bPaused = true;
  if (m_pAudioDecoder) m_pAudioDecoder->Pause();
}

void CDVDAudio::Resume()
{
  CSingleLock lock (m_critSection);
  m_bPaused = false;
  if (m_pAudioDecoder) m_pAudioDecoder->Resume();
}

double CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);

  double delay = 0.0;
  if(m_pAudioDecoder)
    delay = m_pAudioDecoder->GetDelay();

  delay += m_SecondsPerByte * m_iBufferSize;

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
  m_iBufferSize = 0;
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioDecoder)
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(audioframe.channels != m_iChannels
  || audioframe.sample_rate != m_iBitrate
  || audioframe.bits_per_sample != m_iBitsPerSample)
    return false;

  return true;
}

double CDVDAudio::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioDecoder)
    return 0.0;

  double delay = 0.0;
  if(m_pAudioDecoder)
    delay = m_pAudioDecoder->GetCacheTime();

  delay += m_SecondsPerByte * m_iBufferSize;

  return delay;
}

double CDVDAudio::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioDecoder)
    return 0.0;
  return m_pAudioDecoder->GetCacheTotal();
}
