/*
* XBMC Media Center
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
#include "Settings.h"
#include <Mmreg.h>

#pragma comment(lib, "dxguid.lib")


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CWin32DirectSound::CWin32DirectSound()
{
}

bool CWin32DirectSound::Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bAudioPassthrough)
{
  //////////////////////////////////
  // taken from mplayers ao_dsound.c
  //////////////////////////////////
  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  if(bAudioPassthrough)
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE_DIGITAL);
  else
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  m_pDSound=g_audioContext.GetDirectSoundDevice();

  m_bPause = false;
  m_bIsAllocated = false;
  m_pBuffer = NULL;
  m_pNotify = NULL;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;

  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;
  
  WAVEFORMATEXTENSIBLE wfxex = {0};
  DSBUFFERDESC dsbdesc;

  //fill waveformatex
  ZeroMemory(&wfxex, sizeof(WAVEFORMATEXTENSIBLE));
  wfxex.Format.cbSize          =  sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = iChannels;
  wfxex.Format.nSamplesPerSec  = uiSamplesPerSec;
  if (bAudioPassthrough == true) 
  {
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
  } 
  else 
  {
    wfxex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    wfxex.Format.wBitsPerSample  = uiBitsPerSample;
  }
  wfxex.Format.nBlockAlign     = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

  // unsure if this are the right values
  m_dwPacketSize = wfxex.Format.nBlockAlign * 3096;
  m_dwNumPackets = 16;

  // fill in the secondary sound buffer (=stream buffer) descriptor
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                  | DSBCAPS_GLOBALFOCUS         /** Allows background playing */
                  | DSBCAPS_CTRLVOLUME          /** volume control enabled */
                  | DSBCAPS_LOCHARDWARE         /** Needed for 5.1 on emu101k - shit soundblaster */
                  | DSBCAPS_CTRLPOSITIONNOTIFY; /** To know where in the buffer we are */
  const int channel_mask[] = 
  {
    SPEAKER_FRONT_CENTER,
    SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT,
    SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY,
    SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT,
    SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT   | SPEAKER_LOW_FREQUENCY,
    SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT     | SPEAKER_LOW_FREQUENCY
  };

  wfxex.dwChannelMask = channel_mask[iChannels - 1];
  wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
  wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
  

  dsbdesc.dwBufferBytes = m_dwNumPackets * m_dwPacketSize;

  dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wfxex;

  HRESULT res;
  // now create the stream buffer

  res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
  if (res != DS_OK) 
  {
    if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE) 
    {
      SAFE_RELEASE(m_pBuffer);
      // Try without DSBCAPS_LOCHARDWARE
      dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
      res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
    }
    if (res != DS_OK && dsbdesc.dwFlags & DSBCAPS_CTRLVOLUME) 
    {
      SAFE_RELEASE(m_pBuffer);
      // Try without DSBCAPS_CTRLVOLUME
      dsbdesc.dwFlags &= ~DSBCAPS_CTRLVOLUME;
      res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
    }
    if (res != DS_OK) 
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGERROR, __FUNCTION__" - cannot create secondary (stream)buffer (%s)", dserr2str(res));
      return false;
    }
  }
  CLog::Log(LOGDEBUG, __FUNCTION__" - secondary sound (stream)buffer created");

  
  m_pBuffer->Stop();
  m_pBuffer->SetVolume( g_stSettings.m_nVolumeLevel );

  LPDIRECTSOUNDNOTIFY8 lpDsNotify;
  if(SUCCEEDED(m_pBuffer->QueryInterface(IID_IDirectSoundNotify8, (LPVOID*)&lpDsNotify)))
  {
    m_pNotify = new DSBPOSITIONNOTIFY[m_dwNumPackets];
    for(DWORD i = 0; i < m_dwNumPackets; i++)
    {
      m_pNotify[i].dwOffset     = i * m_dwPacketSize;
      m_pNotify[i].hEventNotify = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    if(FAILED(lpDsNotify->SetNotificationPositions(m_dwNumPackets, m_pNotify)))
    {
      for(DWORD i = 0; i < m_dwNumPackets; i++)
        CloseHandle(m_pNotify[i].hEventNotify);
      delete[] m_pNotify;
    }
    SAFE_RELEASE(lpDsNotify);
  }

  m_bIsAllocated = true;
  m_nextPacket = 0;
  return m_bIsAllocated;
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

  if (m_pNotify)
  {
    for(DWORD i = 0; i < m_dwNumPackets; i++)
      CloseHandle(m_pNotify[i].hEventNotify);
    delete[] m_pNotify;
    m_pNotify = NULL;
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
  m_pBuffer->Stop();
  m_pBuffer->SetCurrentPosition(0);
  m_nextPacket = 0;
  m_bPause = false;

  if (m_pNotify)
  {
    for(DWORD i = 0; i < m_dwNumPackets; i++)
      ResetEvent(m_pNotify[i].hEventNotify);
  }
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
  DWORD total = len;

  if (len < m_dwPacketSize)
    return 0;

  while (GetSpace() >= m_dwPacketSize)
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

    if(m_pNotify)  // this packet hasn't player
      ResetEvent(m_pNotify[m_nextPacket].hEventNotify);

    m_nextPacket = (m_nextPacket + 1) % m_dwNumPackets;

    if(m_pNotify)  // next packet we detect underrun on
      ResetEvent(m_pNotify[m_nextPacket].hEventNotify);


    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
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
  if (FAILED(m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
    return 0;

  // the packet we can write to must be after the packet containing the
  // write cursor and before the packet containing the play cursor.
  DWORD writablePacket = (writeCursor + m_dwPacketSize - 1) / m_dwPacketSize % m_dwNumPackets;
  DWORD playingPacket = playCursor / m_dwPacketSize;

  // these are the logical situation that can occure
  // | | | | | | | | | |
  // ***N----W----P*****
  // | | | | | | | | | |
  // ---P****N----W----- < underrun
  // | | | | | | | | | |
  // ---W----P****N----- < underrun
  // | | | | | | | | | |
  // ***W****N----P*****
  // | | | | | | | | | |
  // ---P****W****N-----
  // | | | | | | | | | |
  // ***N----P****W*****



  if(m_pNotify)
  {
    // if the buffer we want to write next have started playing, we have had an underrun
    if(WaitForSingleObject(m_pNotify[m_nextPacket].hEventNotify, 0) == WAIT_OBJECT_0)
    {
      CLog::Log(LOGWARNING, "CWin32DirectSound::GetSpace - buffer underrun (notify) - W:%u, P:%u, N:%u", writablePacket, playingPacket, m_nextPacket);
      Stop();
      return (m_dwNumPackets - 1) * m_dwPacketSize - m_dwPacketSize;
    }
  }

  // we never allow the buffer to fill completely 
  // thus P=N means buffer is empty, if we are playing
  // check for underruns
  if(writablePacket < playingPacket && playingPacket < m_nextPacket
  || playingPacket <  m_nextPacket  && m_nextPacket  < writablePacket
  || playingPacket == m_nextPacket  && playCursor != writeCursor)
  { 
    CLog::Log(LOGWARNING, "CWin32DirectSound::GetSpace - buffer underrun (calc) - W:%u, P:%u, N:%u", writablePacket, playingPacket, m_nextPacket);
//    Stop();
//    return (m_dwNumPackets - 1) * m_dwPacketSize - m_dwPacketSize;
  }

  DWORD freePackets = 0;
  if (m_nextPacket > playingPacket)
    return playCursor - m_nextPacket * m_dwPacketSize - m_dwPacketSize + m_dwNumPackets * m_dwPacketSize;
  else if(m_nextPacket < playingPacket)
    return playCursor - m_nextPacket * m_dwPacketSize - m_dwPacketSize;
  else
    return (m_dwNumPackets - 1) * m_dwPacketSize - m_dwPacketSize;
}

//***********************************************************************************************
FLOAT CWin32DirectSound::GetDelay()
{
  DWORD bytes = m_dwPacketSize * (m_dwNumPackets - 1) - GetSpace();
  FLOAT delay;

  delay  = 0.008f;
  delay += (FLOAT)bytes / ( m_uiChannels * m_uiSamplesPerSec * m_uiBitsPerSample / 8 );
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
  DWORD status, timeout, packet;
  unsigned char* silence;

  if (!m_pBuffer)
    return ;

  GetSpace(); // this will check for underruns

  if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
    return;

  timeout  = (DWORD)(1000 * GetDelay());
  timeout += timeGetTime();
  packet   = m_nextPacket;
  silence  = (unsigned char*)calloc(m_dwPacketSize, 1);

  while(AddPackets(silence, m_dwPacketSize) == 0)
  {
    if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
      break;

    if(timeout < timeGetTime())
    {
      CLog::Log(LOGWARNING, "CWin32DirectSound::WaitCompletion - timeout adding final silent packet");
      break;
    }

    Sleep(1);
  }
  free(silence);

  while(GetSpace() < m_dwPacketSize * (m_dwNumPackets - 2))
  {
    if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
      break;

    if(timeout < timeGetTime())
    {
      CLog::Log(LOGDEBUG, "CWin32DirectSound::WaitCompletion - timeout waiting for silence");
      break;
    }

    if(m_pNotify)
    {
      if(WaitForSingleObject(m_pNotify[packet].hEventNotify, 1) == WAIT_OBJECT_0)
        break;
    }
    else
      Sleep(1);
  }

  m_pBuffer->Stop();
}

void CWin32DirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}

char * CWin32DirectSound::dserr2str(int err)
{
  switch (err) 
  {
    case DS_OK: return "DS_OK";
    case DS_NO_VIRTUALIZATION: return "DS_NO_VIRTUALIZATION";
    case DSERR_ALLOCATED: return "DS_NO_VIRTUALIZATION";
    case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
    case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
    case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
    case DSERR_GENERIC: return "DSERR_GENERIC";
    case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
    case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
    case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
    case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
    case DSERR_NODRIVER: return "DSERR_NODRIVER";
    case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
    case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
    case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
    case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
    case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
    case DSERR_NOINTERFACE: return "DSERR_NOINTERFACE";
    case DSERR_ACCESSDENIED: return "DSERR_ACCESSDENIED";
    default: return "unknown";
  }
}
