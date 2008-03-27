/*
* XBoxMediaPlayer
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
#include "Win32DirectSound.h"
#include "AudioContext.h"
#include "KS.h"
#include "Ksmedia.h"
#include <Mmreg.h>

#define STATIC_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF \
  DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_DOLBY_AC3_SPDIF)

DEFINE_GUIDSTRUCT("00000092-0000-0010-8000-00aa00389b71",
  KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF);

#define KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF \
  DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF)





void CWin32DirectSound::DoWork()
{

}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CWin32DirectSound::CWin32DirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, char* strAudioCodec, bool bIsMusic, bool bAudioPassthrough)
{

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  m_pDSound=g_audioContext.GetDirectSoundDevice();

  m_bPause = false;
  m_bIsAllocated = false;
  m_pBuffer = NULL;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;

  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;

  WAVEFORMATEXTENSIBLE wfxex = {0};
  wfxex.Format.nChannels       = iChannels;
  wfxex.Format.nSamplesPerSec  = uiSamplesPerSec;
  wfxex.Format.wBitsPerSample  = uiBitsPerSample;
  wfxex.Format.nBlockAlign     = uiBitsPerSample / 8 * iChannels;
  wfxex.Format.nAvgBytesPerSec = wfxex.Format.nBlockAlign * wfxex.Format.nSamplesPerSec;
  wfxex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
  wfxex.Format.cbSize          = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ;
  wfxex.Samples.wValidBitsPerSample = uiBitsPerSample;

  if(bAudioPassthrough == false)
    wfxex.SubFormat            = KSDATAFORMAT_SUBTYPE_PCM;  
  else
    wfxex.SubFormat            = KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;  


  m_dwPacketSize = wfxex.Format.nBlockAlign * 512;
  m_dwNumPackets = 16;


  DWORD dwMask[] = 
  { SPEAKER_FRONT_CENTER,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
  };

  if( iChannels > 0 && iChannels < 7 )
    wfxex.dwChannelMask = dwMask[iChannels-1];
  else
    wfxex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;

  DSBUFFERDESC dssd = {};
  dssd.dwSize        = sizeof(DSBUFFERDESC);
  dssd.dwFlags       = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
  dssd.dwBufferBytes = m_dwNumPackets * m_dwPacketSize;
  dssd.lpwfxFormat   = &wfxex.Format;

  HRESULT result = m_pDSound->CreateSoundBuffer(&dssd, &m_pBuffer, NULL);
  if(FAILED(result))
  {
    SAFE_RELEASE(m_pBuffer);

    // Try to create buffer without volume controls
    dssd.dwFlags       = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    HRESULT result = m_pDSound->CreateSoundBuffer(&dssd, &m_pBuffer, NULL);
    if(FAILED(result))
    {
      CLog::Log(LOGERROR, __FUNCTION__" - CreateSoundBuffer failed with error code 0x%x", result);
      SAFE_RELEASE(m_pBuffer);
      return;
    }
  }
  
  m_pBuffer->Stop();
  m_pBuffer->SetVolume( g_stSettings.m_nVolumeLevel );

  m_bIsAllocated = true;
  m_nextPacket = 0;
}

//***********************************************************************************************
CWin32DirectSound::~CWin32DirectSound()
{
  OutputDebugString("CWin32DirectSound() dtor\n");
  Deinitialize();
}


//***********************************************************************************************
HRESULT CWin32DirectSound::Deinitialize()
{
  OutputDebugString("CWin32DirectSound::Deinitialize\n");

  m_bIsAllocated = false;
  if (m_pBuffer)
  {
    m_pBuffer->Stop();
    SAFE_RELEASE(m_pBuffer);
  }

  m_pDSound = NULL;  
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return S_OK;
}


//***********************************************************************************************
HRESULT CWin32DirectSound::Pause()
{
  if (m_bPause) return S_OK;
  m_bPause = true;
  m_pBuffer->Stop();
  return S_OK;
}

//***********************************************************************************************
HRESULT CWin32DirectSound::Resume()
{
  if (!m_bPause) return S_OK;
  m_bPause = false;
  m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);

  return S_OK;
}

//***********************************************************************************************
HRESULT CWin32DirectSound::Stop()
{
  if (m_bPause) return S_OK;
  m_pBuffer->Stop();
  return S_OK;
}

//***********************************************************************************************
LONG CWin32DirectSound::GetMinimumVolume() const
{
  return DSBVOLUME_MIN;
}

//***********************************************************************************************
LONG CWin32DirectSound::GetMaximumVolume() const
{
  return DSBVOLUME_MAX;
}

//***********************************************************************************************
LONG CWin32DirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CWin32DirectSound::Mute(bool bMute)
{
  if (!m_bIsAllocated) return;
  if (bMute)
    m_pBuffer->SetVolume(GetMinimumVolume());
  else
    m_pBuffer->SetVolume(m_nCurrentVolume);
}

//***********************************************************************************************
HRESULT CWin32DirectSound::SetCurrentVolume(LONG nVolume)
{
  if (!m_bIsAllocated) return -1;
  m_nCurrentVolume = nVolume;
  return m_pBuffer->SetVolume( m_nCurrentVolume );
}

//***********************************************************************************************
DWORD CWin32DirectSound::AddPackets(unsigned char *data, DWORD len)
{
  DWORD playCursor, writeCursor;
  if (FAILED(m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
    return 0;

  // the packet we can write to must be after the packet containing the
  // write cursor and before the packet containing the play cursor.
  DWORD writablePacket = (writeCursor + m_dwPacketSize - 1) / m_dwPacketSize;
  DWORD playingPacket = playCursor / m_dwPacketSize;

  DWORD total = len;

  // see GetSpace() for an explanation of the logic here
  while ((playingPacket < writablePacket && (m_nextPacket >= writablePacket || m_nextPacket < playingPacket)) ||
         (playingPacket > writablePacket && (m_nextPacket >= writablePacket && m_nextPacket < playingPacket)))
  {
    if (len < m_dwPacketSize)
      break;
    LPVOID start, startWrap;
    DWORD  size, sizeWrap;

    if (FAILED(m_pBuffer->Lock(m_nextPacket * m_dwPacketSize, m_dwPacketSize, &start, &size, &startWrap, &sizeWrap, 0)))
    { // bad news :(
      CLog::Log(LOGERROR, "Error adding packet %i to stream", m_nextPacket);
      break;
    }

    // write data into our packet
    memcpy(start, data, size);
    if (startWrap)
      memcpy(startWrap, data + size, sizeWrap);

    data += size + sizeWrap;
    len -= size + sizeWrap;

    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
    m_nextPacket = (m_nextPacket + 1) % m_dwNumPackets;
  }
  DWORD status;
  m_pBuffer->GetStatus(&status);

  if(!m_bPause && !(status & DSBSTATUS_PLAYING))
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);

  return total - len;
}

DWORD CWin32DirectSound::GetSpace()
{
  DWORD playCursor, writeCursor;
  if (SUCCEEDED(m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
  {
    if (!writeCursor && !playCursor)
    { // just starting, so we have all packets free
      return m_dwPacketSize * m_dwNumPackets;
    }

    // the packet we can write to must be after the packet containing the
    // write cursor and before the packet containing the play cursor.
    DWORD writablePacket = (writeCursor + m_dwPacketSize - 1) / m_dwPacketSize;
    DWORD playingPacket = playCursor / m_dwPacketSize;

    DWORD freePackets = 0;
    if (playingPacket > writablePacket)
    { // easy case - the next packet to write must be between the two
      // | | | | | | | | | |
      //    W   N-----P
      if (m_nextPacket >= writablePacket && m_nextPacket < playingPacket)
        freePackets = playingPacket - m_nextPacket;
    }
    else if (playingPacket < writablePacket)
    { // wrapping, our next packet has to be either after our write packet, or before the playing packet
      // | | | | | | | | | |        | | | | | | | | | |
      // -----P     W   N---   OR    N---P     W
      if (m_nextPacket >= writablePacket)
        freePackets = m_dwNumPackets - m_nextPacket + playingPacket;
      else if (m_nextPacket < playingPacket)
        freePackets = playingPacket - m_nextPacket;
    }
    return freePackets * m_dwPacketSize;
  }
  return 0;
}

//***********************************************************************************************
FLOAT CWin32DirectSound::GetDelay()
{
  FLOAT delay = 0.008f;

  DWORD playCursor, writeCursor;
  if (SUCCEEDED(m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
  {
    DWORD bytes;
    if (playCursor < m_nextPacket * m_dwPacketSize)
      bytes = m_nextPacket * m_dwPacketSize - playCursor;
    else
      bytes = m_dwPacketSize * m_dwNumPackets - playCursor + m_nextPacket * m_dwPacketSize;

    delay += (FLOAT)bytes / ( m_uiChannels * m_uiSamplesPerSec * m_uiBitsPerSample / 8 );
  }
  return delay;
}

//***********************************************************************************************
DWORD CWin32DirectSound::GetChunkLen()
{
  return m_dwPacketSize;
}
//***********************************************************************************************
int CWin32DirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CWin32DirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

void CWin32DirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void CWin32DirectSound::WaitCompletion()
{
  if (!m_pBuffer)
    return ;

}

void CWin32DirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}
