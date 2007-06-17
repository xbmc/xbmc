
#include "stdafx.h"
#include "DVDAudio.h"
#ifdef _XBOX
#include "../mplayer/ASyncDirectSound.h"
#include "../mplayer/ac97directsound.h"
#elif _LINUX
#include "ALSADirectSound.h"
#else
#include "../mplayer/Win32DirectSound.h"
#endif
#include "../../Util.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"


CDVDAudio::CDVDAudio(bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioDecoder = NULL;
  m_pCallback = NULL;
  m_iBufferSize = 0;
  m_dwPacketSize = 0;
  m_pBuffer = NULL;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }
  if (m_pBuffer) delete[] m_pBuffer;
}

void CDVDAudio::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  m_pCallback = pCallback;
  if (m_pCallback && m_pAudioDecoder) m_pAudioDecoder->RegisterAudioCallback(pCallback);
}

void CDVDAudio::UnRegisterAudioCallback()
{
  CSingleLock lock (m_critSection);
  m_pCallback = NULL;
  if (m_pAudioDecoder) m_pAudioDecoder->UnRegisterAudioCallback();
}

bool CDVDAudio::Create(int iChannels, int iBitrate, int iBitsPerSample, bool bPasstrough, CodecID codec)
{
  // if passthrough isset do something else
  CSingleLock lock (m_critSection);

  char* codecstring="";
  if(codec == CODEC_ID_AAC)
    codecstring = "AAC";
  else if(codec == CODEC_ID_AC3 || codec == CODEC_ID_DTS)
    codecstring = ""; // TODO, fix ac3 and dts decoder to output standard windows mapping
  else
    codecstring = "PCM";

#ifdef _XBOX
  // we don't allow resampling now, there is a bug in sscc that causes it to return the wrong chunklen.
  if( bPasstrough )
    m_pAudioDecoder = new CAc97DirectSound(m_pCallback, iChannels, iBitrate, iBitsPerSample, true); // true = resample, 128 buffers
  else
    m_pAudioDecoder = new CASyncDirectSound(m_pCallback, iChannels, iBitrate, iBitsPerSample, codecstring);
#elif _LINUX

  m_pAudioDecoder = new CALSADirectSound(m_pCallback, iChannels, iBitrate, iBitsPerSample, false, codecstring);

#else

  if( bPasstrough )
    return false;

  m_pAudioDecoder = new CWin32DirectSound(m_pCallback, iChannels, iBitrate, iBitsPerSample, false, codecstring);
#endif

  if (!m_pAudioDecoder) return false;

  m_iChannels = iChannels;
  m_iBitrate = iBitrate;
  m_iBitsPerSample = iBitsPerSample;

  m_dwPacketSize = m_pAudioDecoder->GetChunkLen();
  if (m_pBuffer) delete[] m_pBuffer;
  m_pBuffer = new BYTE[m_dwPacketSize];

  
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

  if (m_pBuffer) delete[] m_pBuffer;
  m_pBuffer = NULL;
  m_dwPacketSize = 0;
  m_pAudioDecoder = NULL;
  m_iBufferSize = 0;
  m_iChannels = 0;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_iSpeed = 1;
}

void CDVDAudio::SetSpeed(int iSpeed)
{
  m_iSpeed = abs(iSpeed);

}

DWORD CDVDAudio::AddPacketsRenderer(unsigned char* data, DWORD len)
{ 
  //Since we write same data size each time, we can drop full chunks to simulate a specific playback speed
  //m_iSpeedStep = (m_iSpeedStep+1) % m_iSpeed;
  //if( m_iSpeedStep )
  //  return m_dwPacketSize;
  //else

    CSingleLock lock (m_critSection);
    return m_pAudioDecoder->AddPackets(data, len);
    
}

DWORD CDVDAudio::AddPackets(unsigned char* data, DWORD len)
{
  if (!m_pAudioDecoder)
    return -1;

  int iTotalSize = len;

  if (m_iBufferSize > 0)
  {
    unsigned int iBytesToCopy = m_dwPacketSize - m_iBufferSize;
    if (iBytesToCopy > len) iBytesToCopy = len;
    
    memcpy(m_pBuffer + m_iBufferSize, data, iBytesToCopy);
    data += iBytesToCopy;
    len -= iBytesToCopy;
    m_iBufferSize += iBytesToCopy;
    
    if (AddPacketsRenderer(m_pBuffer, m_iBufferSize) != m_dwPacketSize)
      return iBytesToCopy;

    m_iBufferSize = 0;
  }

  DWORD copied = 0;
  do
  {
    copied = AddPacketsRenderer(data, len);
    if (copied < 0)
    {
      m_iBufferSize = 0;
      return -1;
    }
    else if (copied == 0) Sleep(1);
    else
    {
      data += copied;
      len -= copied;
      if (len >= m_dwPacketSize) Sleep(10);
    }
  }
  while (len >= m_dwPacketSize && !m_bStop); // if we send to much data at once, we have to send more again

  // if copied is not len then the decoder didn't accept the last few bytes
  // we save it for the next call to this funtion
  if (len > 0)
  {
    m_iBufferSize = min(len, m_dwPacketSize);
    memcpy(m_pBuffer, data, m_iBufferSize);
    len -= m_iBufferSize;
  }
  return iTotalSize - len;
}

void CDVDAudio::DoWork()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->DoWork();
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

__int64 CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);

  double delay = 0.0;
  if(m_pAudioDecoder)
    delay = m_pAudioDecoder->GetDelay();

  DWORD bps = m_iChannels * m_iBitrate * m_iBitsPerSample>>3;
  if(m_iBufferSize && bps)
    delay += (double)m_iBufferSize / bps;

  return (__int64)(delay * DVD_TIME_BASE);
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
