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
 
#include "stdafx.h"
#include "DVDAudio.h"
#include "../../Util.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDPlayerAudio.h"
#include "cores/masteraudio/PCMAudioClient.h"
#include "cores/masteraudio/PassthroughAudioClient.h"

using namespace std;

CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pCallback = NULL;
  m_iBufferSize = 0;
  m_dwPacketSize = 0;
  m_pBuffer = NULL;
  m_iSpeed = 0;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_iChannels = 0;
  m_pAudioClient = NULL;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pBuffer) 
    delete[] m_pBuffer;
  delete m_pAudioClient;
}

void CDVDAudio::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  m_pCallback = pCallback;
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

  const char* codecstring="";

  if(codec == CODEC_ID_AAC)
    codecstring = "AAC";
  else if(codec == CODEC_ID_VORBIS)
    codecstring = "Vorbis";
  else if(codec == CODEC_ID_AC3 || codec == CODEC_ID_DTS)
    codecstring = ""; // TODO, fix ac3 and dts decoder to output standard windows mapping
  else
    codecstring = "PCM";

  // TODO: Select proper client type based on stream format, i.e. PCM, float, encoded/passthrough (Maybe use a factory/dictionary)
  delete m_pAudioClient;
  bool clientInitialized = false;
  if (audioframe.passthrough)
  {
    CPassthroughAudioClient* pClient = new CPassthroughAudioClient(&g_AudioLibManager);
    m_pAudioClient = pClient;
    if (codec == CODEC_ID_AC3)
      clientInitialized = pClient->OpenAC3Stream();
    else if (codec == CODEC_ID_DTS)
      clientInitialized = pClient->OpenDTSStream();
  }
  else
  {
    CPCMAudioClient* pClient = new CPCMAudioClient(&g_AudioLibManager);
    m_pAudioClient = pClient;
    clientInitialized = pClient->OpenStream(audioframe.channels, audioframe.bits_per_sample, audioframe.sample_rate);
  }

  if (!clientInitialized)
  {
    delete m_pAudioClient;
    m_pAudioClient = NULL;
    return false;
  }

  m_iChannels = audioframe.channels;
  m_iBitrate = audioframe.sample_rate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough = audioframe.passthrough;

  // TODO: eliminate buffer. Send data as it arrives
  m_dwPacketSize = 4096;
  if (m_pBuffer) 
    delete[] m_pBuffer;
  m_pBuffer = new BYTE[m_dwPacketSize];
  
  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioClient)
  {
    m_pAudioClient->Stop();
    m_pAudioClient->CloseStream();
  }

  if (m_pBuffer) 
    delete[] m_pBuffer;
  
  m_pBuffer = NULL;
  m_dwPacketSize = 0;
  m_iBufferSize = 0;
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

DWORD CDVDAudio::AddPacketsRenderer(unsigned char* data, DWORD len, CSingleLock &lock)
{ 
  if(!m_pAudioClient || !m_pAudioClient->IsStreamOpen())
    return 0;

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);
  if(!bps)
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioClient->GetDelay() + (double)len / bps);
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += CDVDClock::GetAbsoluteClock();

  DWORD  total = len;
  DWORD  copied;
  do
  {
    // TODO: Verify that we are writing in compatible sizes
    copied = m_pAudioClient->AddDataToStream(data, len);

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

  if (m_iBufferSize > 0)
  {
    copied = std::min(m_dwPacketSize - m_iBufferSize, len);

    memcpy(m_pBuffer + m_iBufferSize, data, copied);
    data += copied;
    len -= copied;
    m_iBufferSize += copied;

    if(m_iBufferSize < m_dwPacketSize)
      return copied;

    if(AddPacketsRenderer(m_pBuffer, m_iBufferSize, lock) != m_iBufferSize)
    {
      m_iBufferSize = 0;
      CLog::Log(LOGERROR, "%s - failed to add leftover bytes to render", __FUNCTION__);
      return copied;
    }

    m_iBufferSize = 0;
  }

  copied = AddPacketsRenderer(data, len, lock);
  data += copied;
  len -= copied;

  // if we have more data left, save it for the next call to this funtion
  if (len > 0 && !m_bStop)
  {
    if(len > m_dwPacketSize)
      CLog::Log(LOGERROR, "%s - More bytes left than can be stored in buffer", __FUNCTION__);

    m_iBufferSize = std::min(len, m_dwPacketSize);
    memcpy(m_pBuffer, data, m_iBufferSize);
    len  -= m_iBufferSize;
    data += m_iBufferSize;
  }
  return total - len;
}

void CDVDAudio::Drain()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioClient || !m_pAudioClient->IsStreamOpen()) 
    return;

  // TODO: The AudioManager is not very good at draining a stream yet. Be advised.
  m_pAudioClient->DrainStream(0xFFFFFFFF);
  m_iBufferSize = 0;
}

void CDVDAudio::SetVolume(int iVolume)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioClient) 
    m_pAudioClient->SetVolume(iVolume);
}

void CDVDAudio::SetDynamicRangeCompression(long drc)
{
  CSingleLock lock (m_critSection);
  // TODO: Implement (or remove)
}

void CDVDAudio::Pause()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioClient) 
    m_pAudioClient->Pause();
}

void CDVDAudio::Resume()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioClient) 
    m_pAudioClient->Resume();
}

double CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioClient || !m_pAudioClient->IsStreamOpen())
    return 0.0;

  double delay = 0.0;
  delay = m_pAudioClient->GetDelay();

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);
  if(m_iBufferSize && bps)
    delay += (double)m_iBufferSize / bps;

  return delay * DVD_TIME_BASE;
}

void CDVDAudio::Flush()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioClient && m_pAudioClient->IsStreamOpen())
    m_pAudioClient->FlushStream();
 
  m_iBufferSize = 0;
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioClient || !m_pAudioClient->IsStreamOpen())
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(audioframe.channels != m_iChannels 
  || audioframe.sample_rate != m_iBitrate 
  || audioframe.bits_per_sample != m_iBitsPerSample)
    return false;

  return true;
}
