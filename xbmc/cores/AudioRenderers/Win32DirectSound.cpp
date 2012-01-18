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

#include "threads/SystemClock.h"
#include "system.h" // WIN32INCLUDES needed for the directsound stuff below
#include "Win32DirectSound.h"
#include "guilib/AudioContext.h"
#include "settings/Settings.h"
#include <initguid.h>
#include <Mmreg.h>
#include "threads/SingleLock.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/CharsetConverter.h"

#ifdef HAS_DX
#pragma comment(lib, "dxguid.lib")
#endif

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_PCM, WAVE_FORMAT_PCM, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

const enum PCMChannels dsound_default_channel_layout[][8] = 
{
  {PCM_FRONT_CENTER},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_LOW_FREQUENCY},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_CENTER, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_SIDE_LEFT, PCM_SIDE_RIGHT}
};

const enum PCMChannels dsound_channel_order[] = {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_FRONT_LEFT_OF_CENTER, PCM_FRONT_RIGHT_OF_CENTER, PCM_BACK_CENTER, PCM_SIDE_LEFT, PCM_SIDE_RIGHT};

#define DSOUND_TOTAL_CHANNELS 11

static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
  AudioSinkList& enumerator = *static_cast<AudioSinkList*>(lpContext);

  CStdString device(lpcstrDescription);
  g_charsetConverter.unknownToUTF8(device);

  enumerator.push_back(AudioSink(CStdString("DirectSound: ").append(device), CStdString("directsound:").append(device)));

  return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CWin32DirectSound::CWin32DirectSound() :
  m_Passthrough(false),
  m_AvgBytesPerSec(0),
  m_CacheLen(0),
  m_dwChunkSize(0),
  m_dwDataChunkSize(0),
  m_dwBufferLen(0),
  m_PreCacheSize(0),
  m_LastCacheCheck(0)
{
}

bool CWin32DirectSound::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels* channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic, EEncoded bAudioPassthrough)
{
  m_uiDataChannels = iChannels;

  if(!bAudioPassthrough)
  {
    //If no channel map is specified, use the default.
    if(!channelMap)
      channelMap = (PCMChannels *)dsound_default_channel_layout[iChannels - 1];

    PCMChannels *outLayout = m_remap.SetInputFormat(iChannels, channelMap, uiBitsPerSample / 8, uiSamplesPerSec);

    for(iChannels = 0; outLayout[iChannels] != PCM_INVALID;) ++iChannels;

    BuildChannelMapping(iChannels, outLayout);
    m_remap.SetOutputFormat(iChannels, m_SpeakerOrder, false);
  }

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
  m_Passthrough = (bAudioPassthrough != ENCODED_NONE);

  m_nCurrentVolume = g_settings.m_nVolumeLevel;
  m_drc = 0;

  WAVEFORMATEXTENSIBLE wfxex = {0};

  //fill waveformatex
  ZeroMemory(&wfxex, sizeof(WAVEFORMATEXTENSIBLE));
  wfxex.Format.cbSize          =  sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = iChannels;
  wfxex.Format.nSamplesPerSec  = uiSamplesPerSec;
  if (bAudioPassthrough)
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
    wfxex.Format.nChannels       = 2;
  }
  else
  {
    wfxex.dwChannelMask          = m_uiSpeakerMask;

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

  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  m_uiBytesPerFrame     = wfxex.Format.nBlockAlign;
  m_uiDataBytesPerFrame = (wfxex.Format.nBlockAlign / iChannels) * m_uiDataChannels;

  // unsure if these are the right values
  m_dwChunkSize = wfxex.Format.nBlockAlign * 3096;
  m_dwDataChunkSize = (m_dwChunkSize / iChannels) * m_uiDataChannels;
  m_dwBufferLen = m_dwChunkSize * 16;
  m_PreCacheSize = m_dwBufferLen - 2*m_dwChunkSize;

  CLog::Log(LOGDEBUG, __FUNCTION__": Packet Size = %d. Avg Bytes Per Second = %d.", m_dwChunkSize, m_AvgBytesPerSec);

  // fill in the secondary sound buffer descriptor
  DSBUFFERDESC dsbdesc;
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                  | DSBCAPS_GLOBALFOCUS         /** Allows background playing */
                  | DSBCAPS_CTRLVOLUME;         /** volume control enabled */

  if (!g_sysinfo.IsVistaOrHigher())
    dsbdesc.dwFlags |= DSBCAPS_LOCHARDWARE;     /** Needed for 5.1 on emu101k, always fails on Vista, by design  */

  dsbdesc.dwBufferBytes = m_dwBufferLen;
  dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wfxex;

  // now create the stream buffer
  HRESULT res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
  if (res != DS_OK)
  {
    if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE)
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

  m_pBuffer->Stop();

  if (DSERR_CONTROLUNAVAIL == m_pBuffer->SetVolume(g_settings.m_nVolumeLevel))
    CLog::Log(LOGINFO, __FUNCTION__": Volume control is unavailable in the current configuration");

  m_bIsAllocated = true;
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_LastCacheCheck = XbmcThreads::SystemClockMillis();

  return m_bIsAllocated;
}

//***********************************************************************************************
CWin32DirectSound::~CWin32DirectSound()
{
  Deinitialize();
}

//***********************************************************************************************
bool CWin32DirectSound::Deinitialize()
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
  return true;
}

//***********************************************************************************************
bool CWin32DirectSound::Pause()
{
  CSingleLock lock (m_critSection);
  if (m_bPause) // Already paused
    return true;
  m_bPause = true;
  m_pBuffer->Stop();

  return true;
}

//***********************************************************************************************
bool CWin32DirectSound::Resume()
{
  CSingleLock lock (m_critSection);
  if (!m_bPause) // Already playing
    return true;
  m_bPause = false;
  if (m_CacheLen >= m_PreCacheSize) // Make sure we have some data to play (if not, playback will start when we add some)
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);

  return true;
}

//***********************************************************************************************
bool CWin32DirectSound::Stop()
{
  CSingleLock lock (m_critSection);
  // Stop and reset DirectSound buffer
  m_pBuffer->Stop();
  m_pBuffer->SetCurrentPosition(0);

  // Reset buffer management members
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_bPause = false;

  return true;
}

//***********************************************************************************************
long CWin32DirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CWin32DirectSound::Mute(bool bMute)
{
  CSingleLock lock (m_critSection);
  if (!m_bIsAllocated) return;
  if (bMute)
    m_pBuffer->SetVolume(VOLUME_MINIMUM);
  else
    m_pBuffer->SetVolume(m_nCurrentVolume);
}

//***********************************************************************************************
bool CWin32DirectSound::SetCurrentVolume(long nVolume)
{
  CSingleLock lock (m_critSection);
  if (!m_bIsAllocated) return false;
  m_nCurrentVolume = nVolume;
  return m_pBuffer->SetVolume( m_nCurrentVolume ) == S_OK;
}

//***********************************************************************************************
unsigned int CWin32DirectSound::AddPackets(const void* data, unsigned int len)
{
  CSingleLock lock (m_critSection);
  DWORD total = len;
  unsigned char* pBuffer = (unsigned char*)data;

  DWORD bufferStatus = 0;
  m_pBuffer->GetStatus(&bufferStatus);
  if (bufferStatus & DSBSTATUS_BUFFERLOST)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__ ": Buffer allocation was lost. Restoring buffer.");
    m_pBuffer->Restore();
  }

  while (len >= m_dwDataChunkSize && GetSpace() >= m_dwDataChunkSize) // We want to write at least one chunk at a time
  {
    LPVOID start = NULL, startWrap = NULL;
    DWORD size = 0, sizeWrap = 0;
    if (m_BufferOffset >= m_dwBufferLen) // Wrap-around manually
      m_BufferOffset = 0;
    HRESULT res = m_pBuffer->Lock(m_BufferOffset, m_dwChunkSize, &start, &size, &startWrap, &sizeWrap, 0);
    if (DS_OK != res)
    {
      CLog::Log(LOGERROR, __FUNCTION__ ": Unable to lock buffer at offset %u. HRESULT: 0x%08x", m_BufferOffset, res);
      break;
    }

    // Remap the data to the correct channels into the buffer
    if (m_remap.CanRemap())
      m_remap.Remap((void*)pBuffer, start, size / m_uiBytesPerFrame, m_drc);
    else
      memcpy(start, pBuffer, size);

    pBuffer += size * m_uiDataBytesPerFrame / m_uiBytesPerFrame;
    len     -= size * m_uiDataBytesPerFrame / m_uiBytesPerFrame;

    m_BufferOffset += size;
    if (startWrap) // Write-region wraps to beginning of buffer
    {
      // Remap the data to the correct channels into the buffer
      if (m_remap.CanRemap())
        m_remap.Remap((void*)pBuffer, startWrap, sizeWrap / m_uiBytesPerFrame, m_drc);
      else
        memcpy(startWrap, pBuffer, sizeWrap);
      m_BufferOffset = sizeWrap;

      pBuffer += sizeWrap * m_uiDataBytesPerFrame / m_uiBytesPerFrame;
      len     -= sizeWrap * m_uiDataBytesPerFrame / m_uiBytesPerFrame;
    }

    m_CacheLen += size + sizeWrap; // This data is now in the cache
    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
  }

  CheckPlayStatus();

  return total - len; // Bytes used
}

void CWin32DirectSound::UpdateCacheStatus()
{
  CSingleLock lock (m_critSection);
  // TODO: Check to see if we may have cycled around since last time
  unsigned int time = XbmcThreads::SystemClockMillis();
  if (time == m_LastCacheCheck)
    return; // Don't recalc more frequently than once/ms (that is our max resolution anyway)

  DWORD playCursor = 0, writeCursor = 0;
  HRESULT res = m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor); // Get the current playback and safe write positions
  if (DS_OK != res)
  {
    CLog::Log(LOGERROR,__FUNCTION__ ": GetCurrentPosition failed. Unable to determine buffer status. HRESULT = 0x%08x", res);
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

void CWin32DirectSound::CheckPlayStatus()
{
  DWORD status = 0;
  m_pBuffer->GetStatus(&status);

  if(!m_bPause && !(status & DSBSTATUS_PLAYING) && m_CacheLen >= m_PreCacheSize) // If we have some data, see if we can start playback
  {
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    CLog::Log(LOGDEBUG,__FUNCTION__ ": Resuming Playback");
  }
}

unsigned int CWin32DirectSound::GetSpace()
{
  CSingleLock lock (m_critSection);
  UpdateCacheStatus();
  unsigned int space = ((m_dwBufferLen - m_CacheLen) / m_uiChannels) * m_uiDataChannels;

  // We can never allow the internal buffers to fill up complete
  // as we get confused between if the buffer is full or empty
  // so never allow the last chunk to be added
  if(space > m_dwDataChunkSize)
    return space - m_dwDataChunkSize;
  else
    return 0;
}

//***********************************************************************************************
float CWin32DirectSound::GetDelay()
{
  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  CSingleLock lock (m_critSection);
  float delay  = 0.008f; // WTF?
  delay += (float)m_CacheLen / (float)m_AvgBytesPerSec;
  return delay;
}

//***********************************************************************************************
float CWin32DirectSound::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  return (float)m_CacheLen / (float)m_AvgBytesPerSec;
}

float CWin32DirectSound::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  return (float)(m_dwBufferLen - m_dwDataChunkSize) / (float)m_AvgBytesPerSec;
}

//***********************************************************************************************
unsigned int CWin32DirectSound::GetChunkLen()
{
  return m_dwDataChunkSize;
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
  CSingleLock lock (m_critSection);
  DWORD status;
  unsigned int timeout;
  unsigned char* silence;

  if (!m_pBuffer)
    return ;

  if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
    return; // We weren't playing anyway

  // The drain should complete in the time occupied by the cache
  timeout  = (unsigned int)(1000 * GetDelay());
  unsigned int startTime = XbmcThreads::SystemClockMillis();
  silence  = (unsigned char*)calloc(1,m_dwChunkSize); // Initialize 'silence' to zero...

  while(AddPackets(silence, m_dwChunkSize) == 0)
  {
    if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
      break;

    if((XbmcThreads::SystemClockMillis() - startTime) > timeout)
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

    if((XbmcThreads::SystemClockMillis() - startTime) > timeout)
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

//***********************************************************************************************
void CWin32DirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
  return;
}

//***********************************************************************************************

void CWin32DirectSound::EnumerateAudioSinks(AudioSinkList &vAudioSinks, bool passthrough)
{
  if (FAILED(DirectSoundEnumerate(DSEnumCallback, &vAudioSinks)))
    CLog::Log(LOGERROR, "%s - failed to enumerate output devices", __FUNCTION__);
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

//***********************************************************************************************
void CWin32DirectSound::BuildChannelMapping(int channels, enum PCMChannels* map)
{
  bool usedChannels[DSOUND_TOTAL_CHANNELS];

  memset(usedChannels, false, sizeof(usedChannels));

  m_uiSpeakerMask = 0;

  if(!map)
    map = (PCMChannels *)dsound_default_channel_layout[channels - 1];

  //Build the speaker mask and note which are used.
  for(int i = 0; i < channels; i++)
  {
    switch(map[i])
    {
    case PCM_FRONT_LEFT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_LEFT;
      break;
    case PCM_FRONT_RIGHT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_RIGHT;
      break;
    case PCM_FRONT_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_CENTER;
      break;
    case PCM_LOW_FREQUENCY:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_LOW_FREQUENCY;
      break;
    case PCM_BACK_LEFT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_BACK_LEFT;
      break;
    case PCM_BACK_RIGHT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_BACK_RIGHT;
      break;
    case PCM_FRONT_LEFT_OF_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_LEFT_OF_CENTER;
      break;
    case PCM_FRONT_RIGHT_OF_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_RIGHT_OF_CENTER;
      break;
    case PCM_BACK_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_BACK_CENTER;
      break;
    case PCM_SIDE_LEFT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_SIDE_LEFT;
      break;
    case PCM_SIDE_RIGHT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_SIDE_RIGHT;
      break;
    }
  }

  //Assemble a compacted channel set.
  for(int i = 0, j = 0; i < DSOUND_TOTAL_CHANNELS; i++)
  {
    if(usedChannels[i])
    {
      m_SpeakerOrder[j] = dsound_channel_order[i];
      j++;
    }
  }
}
