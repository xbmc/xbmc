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



void CWin32DirectSound::DoWork()
{

}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CWin32DirectSound::CWin32DirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, char* strAudioCodec, bool bIsMusic)
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
  wfxex.SubFormat              = KSDATAFORMAT_SUBTYPE_PCM;  
  wfxex.Samples.wValidBitsPerSample = uiBitsPerSample;


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
    CLog::Log(LOGERROR, __FUNCTION__" - CreateSoundBuffer failed with error code %d", result);
    return;
  }
  
  m_pBuffer->Stop();
  m_pBuffer->SetVolume( g_stSettings.m_nVolumeLevel );

  m_bFirstPackets = true;
  m_bIsAllocated = true;
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
DWORD CWin32DirectSound::GetSpace()
{
  DWORD playCursor, writeCursor;
  if (FAILED(m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
    return 0;

  DWORD bytes = writeCursor - playCursor;
  if(bytes > m_dwPacketSize * m_dwNumPackets)
    bytes = 0;

  return bytes;
}

//***********************************************************************************************
DWORD CWin32DirectSound::AddPackets(unsigned char *data, DWORD len)
{
  LPVOID start;
  DWORD  size;
  LPVOID startWrap;
  DWORD  sizeWrap;

  len = min(m_dwPacketSize, len);
  if(GetSpace() < len)
    return 0;

  if (FAILED(m_pBuffer->Lock(0, len, &start, &size, &startWrap, &sizeWrap, DSBLOCK_FROMWRITECURSOR)))
    return 0;

  memcpy(start, data, size);
  if (startWrap)
    memcpy(startWrap, data + size, sizeWrap);

  if (FAILED(m_pBuffer->Unlock(start, size, startWrap, sizeWrap)))
    return 0;

  if(m_bFirstPackets && !m_bPause)
  {
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    m_bFirstPackets = false;
  }
  return len;
}

//***********************************************************************************************
FLOAT CWin32DirectSound::GetDelay()
{
  FLOAT delay = 0.0;

  DWORD playCursor, writeCursor;
  if (FAILED(m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
    return 0;

  DWORD bytes = m_dwPacketSize * m_dwNumPackets + playCursor - writeCursor;
  if(bytes > m_dwPacketSize * m_dwNumPackets)
    bytes = m_dwPacketSize * m_dwNumPackets;

  delay += (FLOAT)bytes / ( m_uiChannels * m_uiSamplesPerSec * m_uiBitsPerSample / 8 );
  delay += 0.008f;

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
