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
#include "Settings.h"
#include <initguid.h>
#include <Mmreg.h>

#pragma comment(lib, "dxguid.lib")

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_PCM, WAVE_FORMAT_PCM, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

const int dsound_channel_mask[] = 
{
  SPEAKER_FRONT_CENTER,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT   | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT     | SPEAKER_LOW_FREQUENCY
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CWin32DirectSound::CWin32DirectSound() :
  m_Passthrough(false),
  m_AvgBytesPerSec(0),
  m_CacheLen(0),
  m_dwChunkSize(0),
  m_dwBufferLen(0),
  m_PreCacheSize(0),
  m_LastCacheCheck(0),
  m_pChannelMap(NULL)
{
}

bool CWin32DirectSound::Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bAudioPassthrough)
{
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
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;
  m_Passthrough = bAudioPassthrough;

  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;
  
  WAVEFORMATEXTENSIBLE wfxex = {0};

  //fill waveformatex
  ZeroMemory(&wfxex, sizeof(WAVEFORMATEXTENSIBLE));
  wfxex.Format.cbSize          =  sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = iChannels;
  wfxex.Format.nSamplesPerSec  = uiSamplesPerSec;
  if (bAudioPassthrough == true) 
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
    wfxex.Format.nChannels       = 2;
  } 
  else
  {
    if (iChannels > 2)
      wfxex.Format.wFormatTag    = WAVE_FORMAT_EXTENSIBLE;
    else
      wfxex.Format.wFormatTag    = WAVE_FORMAT_PCM;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_PCM;
    wfxex.Format.wBitsPerSample  = uiBitsPerSample;
  }

  wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
  wfxex.Format.nBlockAlign       = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
  wfxex.dwChannelMask            = dsound_channel_mask[iChannels - 1];

  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  // unsure if these are the right values
  m_dwChunkSize = wfxex.Format.nBlockAlign * 3096;
  m_dwBufferLen = m_dwChunkSize * 16;

  CLog::Log(LOGDEBUG, __FUNCTION__": Packet Size = %d. Avg Bytes Per Second = %d.", m_dwChunkSize, m_AvgBytesPerSec);

  // fill in the secondary sound buffer descriptor
  DSBUFFERDESC dsbdesc;
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                  | DSBCAPS_GLOBALFOCUS         /** Allows background playing */
                  | DSBCAPS_CTRLVOLUME          /** volume control enabled */
                  | DSBCAPS_LOCHARDWARE;         /** Needed for 5.1 on emu101k  */

  dsbdesc.dwBufferBytes = m_dwBufferLen;
  dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wfxex;

  // now create the stream buffer
  HRESULT res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
  if (res != DS_OK) 
  {
    if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE) // DSBCAPS_LOCHARDWARE Always fails on Vista, by design
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGDEBUG, __FUNCTION__": Couldn't create secondary buffer (%s). Trying without LOCHARDWARE.", dserr2str(res));
      // Try without DSBCAPS_LOCHARDWARE
      dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
      res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
    }
    if (res != DS_OK && dsbdesc.dwFlags & DSBCAPS_CTRLVOLUME) 
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGDEBUG, __FUNCTION__": Couldn't create secondary buffer (%s). Trying without CTRLVOLUME.", dserr2str(res));
      // Try without DSBCAPS_CTRLVOLUME
      dsbdesc.dwFlags &= ~DSBCAPS_CTRLVOLUME;
      res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
    }
    if (res != DS_OK) 
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGERROR, __FUNCTION__": cannot create secondary buffer (%s)", dserr2str(res));
      return false;
    }
  }
  CLog::Log(LOGDEBUG, __FUNCTION__": secondary buffer created");

  // Set up channel mapping
  m_pChannelMap = GetChannelMap(iChannels, strAudioCodec);

  m_pBuffer->Stop();
  
  if (DSERR_CONTROLUNAVAIL == m_pBuffer->SetVolume(g_stSettings.m_nVolumeLevel))
    CLog::Log(LOGINFO, __FUNCTION__": Volume control is unavailable in the current configuration");

  m_bIsAllocated = true;
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_LastCacheCheck = timeGetTime();
  
  return m_bIsAllocated;
}

//***********************************************************************************************
CWin32DirectSound::~CWin32DirectSound()
{
  Deinitialize();
}

//***********************************************************************************************
HRESULT CWin32DirectSound::Deinitialize()
{
  if (m_bIsAllocated)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Cleaning up");
    m_bIsAllocated = false;
    if (m_pBuffer)
    {
      m_pBuffer->Stop();
      SAFE_RELEASE(m_pBuffer);
    }

    m_pBuffer = NULL;
    m_pDSound = NULL;  
    m_BufferOffset = 0;
    m_CacheLen = 0;
    m_dwChunkSize = 0;
    m_dwBufferLen = 0;

    g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  }
  return S_OK;
}

//***********************************************************************************************
HRESULT CWin32DirectSound::Pause()
{
  if (m_bPause) // Already paused
    return S_OK;
  m_bPause = true;
  m_pBuffer->Stop();

  return S_OK;
}

//***********************************************************************************************
HRESULT CWin32DirectSound::Resume()
{
  if (!m_bPause) // Already playing
    return S_OK;
  m_bPause = false;
  if (m_CacheLen > m_PreCacheSize) // Make sure we have some data to play (if not, playback will start when we add some)
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);

  return S_OK;
}

//***********************************************************************************************
HRESULT CWin32DirectSound::Stop()
{
  // Stop and reset DirectSound buffer
  m_pBuffer->Stop();
  m_pBuffer->SetCurrentPosition(0);

  // Reset buffer management members
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_bPause = false;

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

#if defined(_DEBUG) // Watch for junk (unitialized) data
  short* pSamples = (short*)data;
  // Find 5 low samples in a row that == 0xCDCD
  if (pSamples[0] == -12851 && pSamples[1] == -12851 && pSamples[2] == -12851 && pSamples[3] == -12851 && pSamples[4] == -12851)
    CLog::Log(LOGDEBUG, "CWin32DirectSound::AddPackets: Uninitialized data passed to renderer. POP!");
#endif

  while (len >= m_dwChunkSize && GetSpace() >= m_dwChunkSize) // We want to write at least one chunk at a time
  {
    LPVOID start = NULL, startWrap = NULL;
    DWORD  size = 0, sizeWrap = 0;

    if (m_BufferOffset >= m_dwBufferLen) // Wrap-around manually
      m_BufferOffset = 0;

    if (FAILED(m_pBuffer->Lock(m_BufferOffset, m_dwChunkSize, &start, &size, &startWrap, &sizeWrap, 0)))
    { 
      CLog::Log(LOGERROR, __FUNCTION__ ": Unable to lock buffer at offset %u.", m_BufferOffset);
      break;
    }

    // Write data into the buffer
    MapDataIntoBuffer(data, size, (unsigned char*)start);
    m_BufferOffset += size;
    if (startWrap) // Write-region wraps to beginning of buffer
    {
      MapDataIntoBuffer(data + size, sizeWrap, (unsigned char*)startWrap);
      m_BufferOffset = sizeWrap;
    }
    
    size_t bytes = size + sizeWrap;
    m_CacheLen += bytes; // This data is now in the cache
    data += bytes; // Update buffer pointer
    len -= bytes; // Update remaining data len

    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
  }

  DWORD status = 0;
  m_pBuffer->GetStatus(&status);

  if(!m_bPause && !(status & DSBSTATUS_PLAYING) && m_CacheLen > m_PreCacheSize) // If we have some data, see if we can start playback
  {
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    CLog::Log(LOGDEBUG,__FUNCTION__ ": Resuming Playback");
  }

  return total - len; // Bytes used
}

void CWin32DirectSound::UpdateCacheStatus()
{
  // TODO: Check to see if we may have cycled around since last time
  unsigned int time = timeGetTime();
  if (time == m_LastCacheCheck)
    return; // Don't recalc more frequently than once/ms (that is our max resolution anyway)

  DWORD playCursor = 0, writeCursor = 0;
  if (FAILED(m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor))) // Get the current playback and safe write positions
  {
    CLog::Log(LOGERROR,__FUNCTION__ ": GetCurrentPosition failed. Unable to determine buffer status");
    return;
  }

  m_LastCacheCheck = time;
  // Check the state of the ring buffer (P->O->W == underrun)
  // These are the logical situations that can occur
  // O: CurrentOffset  W: WriteCursor  P: PlayCursor
  // | | | | | | | | | |
  // ***O----W----P***** < underrun   P > W && O < W (1)
  // | | | | | | | | | |
  // ---P****O----W----- < underrun   O > P && O < W (2)
  // | | | | | | | | | |
  // ---W----P****O----- < underrun   P > W && P < O (3)
  // | | | | | | | | | |
  // ***W****O----P*****              P > W && P > O (4)
  // | | | | | | | | | |
  // ---P****W****O-----              P < W && O > W (5)
  // | | | | | | | | | |
  // ***O----P****W*****              P < W && O < P (6)

  // Check for underruns
  if ((playCursor > writeCursor && m_BufferOffset < writeCursor) ||    // (1)
      (playCursor < m_BufferOffset && m_BufferOffset < writeCursor) || // (2)
      (playCursor > writeCursor && playCursor <  m_BufferOffset))      // (3)
  { 
    CLog::Log(LOGWARNING, "CWin32DirectSound::GetSpace - buffer underrun - W:%u, P:%u, O:%u.", writeCursor, playCursor, m_BufferOffset);
    m_BufferOffset = writeCursor; // Catch up
    m_pBuffer->Stop(); // Wait until someone gives us some data to restart playback (prevents glitches)
  }

  // Calculate available space in the ring buffer
  if (playCursor == m_BufferOffset && m_BufferOffset ==  writeCursor) // Playback is stopped and we are all at the same place
    m_CacheLen = 0;
  else if (m_BufferOffset > playCursor)
    m_CacheLen = m_BufferOffset - playCursor;
  else
    m_CacheLen = m_dwBufferLen - (playCursor - m_BufferOffset);
}

DWORD CWin32DirectSound::GetSpace()
{
  UpdateCacheStatus();

  return m_dwBufferLen - m_CacheLen;
}

//***********************************************************************************************
FLOAT CWin32DirectSound::GetDelay()
{
  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  FLOAT delay  = 0.008f; // WTF?
  delay += (FLOAT)m_CacheLen / (FLOAT)m_AvgBytesPerSec;
  return delay;
}

//***********************************************************************************************
DWORD CWin32DirectSound::GetChunkLen()
{
  return m_dwChunkSize;
}

//***********************************************************************************************
int CWin32DirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

//***********************************************************************************************
void CWin32DirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

//***********************************************************************************************
void CWin32DirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

//***********************************************************************************************
void CWin32DirectSound::WaitCompletion()
{
  DWORD status, timeout;
  unsigned char* silence;

  if (!m_pBuffer)
    return ;

  if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
    return; // We weren't playing anyway

  // The drain should complete in the time occupied by the cache
  timeout  = (DWORD)(1000 * GetDelay());
  timeout += timeGetTime();
  silence  = (unsigned char*)calloc(1,m_dwChunkSize); // Initialize 'silence' to zero...

  while(AddPackets(silence, m_dwChunkSize) == 0)
  {
    if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
      break;

    if(timeout < timeGetTime())
    {
      CLog::Log(LOGWARNING, __FUNCTION__ ": timeout adding silence to buffer");
      break;
    }
  }
  free(silence);

  while(m_CacheLen)
  {
    if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
      break;

    if(timeout < timeGetTime())
    {
      CLog::Log(LOGDEBUG, "CWin32DirectSound::WaitCompletion - timeout waiting for silence");
      break;
    }
    else
      Sleep(1);
    GetSpace();
  }

  m_pBuffer->Stop();
}

void CWin32DirectSound::MapDataIntoBuffer(unsigned char* pData, DWORD len, unsigned char* pOut)
{
  // TODO: Add support for 8, 24, and 32-bit audio
  if (m_pChannelMap && !m_Passthrough && m_uiBitsPerSample == 16)
  {
    short* pOutFrame = (short*)pOut;
    for (short* pInFrame = (short*)pData;
      pInFrame < (short*)pData + (len / sizeof(short)); 
      pInFrame += m_uiChannels, pOutFrame += m_uiChannels)
    {
      // Remap a single frame
      for (unsigned int chan = 0; chan < m_uiChannels; chan++)
        pOutFrame[m_pChannelMap[chan]] = pInFrame[chan]; // Copy sample into correct position in the output buffer
    }
  }
  else
  {
    memcpy(pOut, pData, len);
  }
}

// Channel maps
// Our output order is FL, FR, C, LFE, SL, SR,
const unsigned char ac3_51_Map[] = {0,1,4,5,2,3};    // Sent as FL, FR, SL, SR, C, LFE
const unsigned char ac3_50_Map[] = {0,1,4,2,3};      // Sent as FL, FR, SL, SR, C
const unsigned char eac3_51_Map[] = {0,4,1,2,3,5};   // Sent as FL, C, FR, SL, SR, LFE
const unsigned char eac3_50_Map[] = {0,4,1,2,3};     // Sent as FL, C, FR, SL, SR, LFE
const unsigned char aac_51_Map[] = {4,0,1,2,3,5};    // Sent as C, FL, FR, SL, SR, LFE
const unsigned char aac_50_Map[] = {4,0,1,2,3};      // Sent as C, FL, FR, SL, SR
const unsigned char vorbis_51_Map[] = {0,4,1,2,3,5}; // Sent as FL, C, FR, SL, SR, LFE
const unsigned char vorbis_50_Map[] = {0,4,1,2,3};   // Sent as FL, C, FR, SL, SR

// TODO: This could be a lot more efficient
unsigned char* CWin32DirectSound::GetChannelMap(unsigned int channels, const char* strAudioCodec)
{
  if (!strcmp(strAudioCodec, "AC3") || !strcmp(strAudioCodec, "DTS"))
  {
    if (channels == 6)
      return (unsigned char*)ac3_51_Map;
    else if (channels == 5)
      return (unsigned char*)ac3_50_Map;
  }
  else if (!strcmp(strAudioCodec, "AAC"))
  {
    if (channels == 6)
      return (unsigned char*)aac_51_Map;
    else if (channels == 5)
      return (unsigned char*)aac_50_Map;
  }
  else if (!strcmp(strAudioCodec, "Vorbis"))
  {
    if (channels == 6)
      return (unsigned char*)vorbis_51_Map;
    else if (channels == 5)
      return (unsigned char*)vorbis_50_Map;
  }
  else if (!strcmp(strAudioCodec, "EAC3"))
  {
    if (channels == 6)
      return (unsigned char*)eac3_51_Map;
    else if (channels == 5)
      return (unsigned char*)eac3_50_Map;
  }
  return NULL; // We don't know how to map this, so just leave it alone
}

//***********************************************************************************************
void CWin32DirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
  return;
}

//***********************************************************************************************
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
