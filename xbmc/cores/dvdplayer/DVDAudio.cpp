
#include "stdafx.h"
#include "dvdaudio.h"
#ifdef _XBOX
#include "..\mplayer\ASyncDirectSound.h"
#include "..\mplayer\ac97directsound.h"
#endif
#include "..\..\util.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"


CDVDAudio::CDVDAudio()
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
  m_pCallback = pCallback;
  if (m_pCallback && m_pAudioDecoder) m_pAudioDecoder->RegisterAudioCallback(pCallback);
}

void CDVDAudio::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
  if (m_pAudioDecoder) m_pAudioDecoder->UnRegisterAudioCallback();
}

bool CDVDAudio::Create(int iChannels, int iBitrate, int iBitsPerSample, bool bPasstrough, CodecID codec)
{
  // if passthrough isset do something else
  CSingleLock lock (m_critSection);

#ifdef _XBOX
  // we don't allow resampling now, there is a bug in sscc that causes it to return the wrong chunklen.
  if( bPasstrough )
  {
    m_iPackets = 16;
    m_pAudioDecoder = new CAc97DirectSound(m_pCallback, iChannels, iBitrate, iBitsPerSample, true, false, m_iPackets); // true = resample, 128 buffers
  }
  else
  {
    char* codecstring="";
    if(codec == CODEC_ID_AAC)
      codecstring = "AAC";
    else if(codec == CODEC_ID_AC3 || codec == CODEC_ID_DTS)
      codecstring = ""; // TODO, fix ac3 and dts decoder to output standard windows mapping
    else
      codecstring = "PCM";

    m_iPackets = 32; //64;// better sync with smaller buffers?
    
    m_pAudioDecoder = new CASyncDirectSound(m_pCallback, iChannels, iBitrate, iBitsPerSample, false, m_iPackets, codecstring);
  }
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
  m_iPackets = 0;
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

// we have a little bug here.
// if resampling is enabled and sscc.cpp is upsampling, m_pAudioDecoder->GetChunkLen() will return the incorrect
// chunklenght. In combination with the code below thiswill result in dropped data.
DWORD CDVDAudio::AddPackets(unsigned char* data, DWORD len)
{
  if (!m_pAudioDecoder)
  {
    m_iBufferSize = 0;
    return -1;
  }
  int iTotalSize = len;

  // wait until we can put something in the buffer, if we don't do this we have to check every time how
  // much is really written because it could be the buffer was still full.
  if ((m_iPackets * m_dwPacketSize) > len)
  {
    while (m_pAudioDecoder->GetSpace() < len) Sleep(1);
  }

  if (m_iBufferSize > 0)
  {
    unsigned int iBytesToCopy = m_dwPacketSize - m_iBufferSize;
    if (iBytesToCopy > len) iBytesToCopy = len;
    
    memcpy(m_pBuffer + m_iBufferSize, data, iBytesToCopy);
    data += iBytesToCopy;
    len -= iBytesToCopy;
    m_iBufferSize += iBytesToCopy;
    
    if (AddPacketsRenderer(m_pBuffer, m_iBufferSize) != m_dwPacketSize)
    {
      return -1;
    }
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
  while (len >= m_dwPacketSize); // if we send to much data at once, we have to send more again

  // if copied is not len then the decoder didn't accept the last few bytes
  // we save it for the next call to this funtion
  if (len > 0)
  {
    m_iBufferSize = len;
    memcpy(m_pBuffer, data, m_iBufferSize);
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
  __int64 delay;
  
  if (m_pAudioDecoder && m_iChannels != 0 && m_iBitrate != 0)
  {

    delay = (__int64)(m_pAudioDecoder->GetDelay() * DVD_TIME_BASE);
    bool bIsResampling = m_pAudioDecoder->IsResampling();
    
    if (bIsResampling) delay += ((__int64)m_pAudioDecoder->GetBytesInBuffer() * DVD_TIME_BASE) / (48000 * m_iChannels * 2);
    else delay += ((__int64)m_pAudioDecoder->GetBytesInBuffer() * DVD_TIME_BASE) / (m_iBitrate * m_iChannels * 2);

    //if( m_iSpeedStep )
    //{
    //  //To smoothout clock abit when dropping audio chunks
    //  delay += ((__int64)m_dwPacketSize*m_iSpeedStep*DVD_TIME_BASE) / ( (__int64)m_iBitrate * m_iChannels * 2 );
    //}

    return delay;
  }
  return 0LL;
}

int CDVDAudio::GetBytesInBuffer()
{
  if(!m_pAudioDecoder) return 0;
  return m_pAudioDecoder->GetBytesInBuffer();
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
