#include "AESinkDirectSound.h"
#include "Log.h"
#include <initguid.h>
#include <Mmreg.h>
#include <list>
#include "SingleLock.h"
#include "utils/TimeUtils.h"
#include "CharsetConverter.h"
#include "SystemGlobals.h"

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

#define SPEAKER_COUNT 8
static const unsigned int DSChannelOrder[] = {SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_CENTER, SPEAKER_LOW_FREQUENCY, SPEAKER_BACK_LEFT, SPEAKER_BACK_RIGHT, SPEAKER_SIDE_LEFT, SPEAKER_SIDE_RIGHT};
static const enum AEChannel AEChannelNames[] = {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

struct DSDevice
{
  CStdString name;
  LPGUID     lpGuid;
};

static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
  DSDevice dev;
  std::list<DSDevice> &enumerator = *static_cast<std::list<DSDevice>*>(lpContext);

  dev.name = lpcstrDescription;
  g_charsetConverter.unknownToUTF8(dev.name);

  dev.lpGuid = lpGuid;

  enumerator.push_back(dev);

  return TRUE;
}

CAESinkDirectSound::CAESinkDirectSound() :
  m_initialized  (false),
  m_pBuffer      (NULL ),
  m_pDSound      (NULL ),
  m_BufferOffset (0    ),
  m_CacheLen     (0    ),
  m_dwChunkSize  (0    ),
  m_dwBufferLen  (0    )
{
  m_channelLayout[0] = AE_CH_NULL;
}

CAESinkDirectSound::~CAESinkDirectSound()
{
  Deinitialize();
}

bool CAESinkDirectSound::Initialize(AEAudioFormat &format, CStdString &device)
{
  CSingleLock lock(m_runLock);
  if(m_initialized) return false;

  LPGUID deviceGUID = NULL;
  std::list<DSDevice> DSDeviceList;
  DirectSoundEnumerate(DSEnumCallback, &DSDeviceList);

  for(std::list<DSDevice>::iterator itt = DSDeviceList.begin(); itt != DSDeviceList.end(); itt++)
  {
    if((*itt).name == device)
    {
      deviceGUID = (*itt).lpGuid;
      break;
    }
  }

  HRESULT hr = DirectSoundCreate(deviceGUID, &m_pDSound, NULL);

  if(FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create the DirectSound device.");
    CLog::Log(LOGERROR, __FUNCTION__": DSErr: %s", dserr2str(hr));
    return false;
  }

  hr = m_pDSound->SetCooperativeLevel(g_hWnd, DSSCL_PRIORITY);

  if(FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create the DirectSound device cooperative level.");
    CLog::Log(LOGERROR, __FUNCTION__": DSErr: %s", dserr2str(hr));
    m_pDSound->Release();
    return false;
  }

  WAVEFORMATEXTENSIBLE wfxex = {0};

  //fill waveformatex
  ZeroMemory(&wfxex, sizeof(WAVEFORMATEXTENSIBLE));
  wfxex.Format.cbSize          = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = format.m_channelCount;
  wfxex.Format.nSamplesPerSec  = format.m_sampleRate;
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
    wfxex.Format.nChannels       = 2;
  }
  else
  {
    wfxex.dwChannelMask          = SpeakerMaskFromAEChannels(format.m_channelLayout);
    wfxex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    wfxex.Format.wBitsPerSample  = 32;
  }

  wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
  wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  // unsure if these are the right values
  unsigned int uiFrameCount = format.m_sampleRate / 100;
  m_dwFrameSize = wfxex.Format.nBlockAlign;
  m_dwChunkSize = m_dwFrameSize * uiFrameCount;
  m_dwBufferLen = m_dwChunkSize * 16;

  // fill in the secondary sound buffer descriptor
  DSBUFFERDESC dsbdesc;
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                  | DSBCAPS_GLOBALFOCUS         /** Allows background playing */
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
    if (res != DS_OK)
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGERROR, __FUNCTION__": cannot create secondary buffer (%s)", dserr2str(res));
      return false;
    }
  }
  CLog::Log(LOGDEBUG, __FUNCTION__": secondary buffer created");

  m_pBuffer->Stop();

  AEChannelsFromSpeakerMask(wfxex.dwChannelMask);
  format.m_channelLayout = m_channelLayout;
  format.m_dataFormat = AE_FMT_FLOAT;
  format.m_frames = uiFrameCount;
  format.m_frameSamples = format.m_frames * format.m_channelCount;
  format.m_frameSize = sizeof(float) * format.m_channelCount;

  m_format = format;
  m_device = device;

  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_LastCacheCheck = CTimeUtils::GetTimeMS();
  m_initialized = true;

  return true;
}

void CAESinkDirectSound::Deinitialize()
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return;

  CLog::Log(LOGDEBUG, __FUNCTION__": Cleaning up");

  if (m_pBuffer)
  {
    m_pBuffer->Stop();
    SAFE_RELEASE(m_pBuffer);
  }

  if(m_pDSound)
  {
    m_pDSound->Release();
  }

  m_initialized = false;
  m_pBuffer = NULL;
  m_pDSound = NULL;
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;
}

bool CAESinkDirectSound::IsCompatible(const AEAudioFormat format, const CStdString device)
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return false;

  if(m_device == device &&
     m_format.m_sampleRate   == format.m_sampleRate  &&
     m_format.m_dataFormat   == format.m_dataFormat  &&
     m_format.m_channelCount == format.m_channelCount)
     return true;

  return false;
}

unsigned int CAESinkDirectSound::AddPackets(uint8_t *data, unsigned int frames)
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return 0;

  DWORD total = m_dwFrameSize * frames;
  DWORD len = total;
  unsigned char* pBuffer = (unsigned char*)data;

  DWORD bufferStatus = 0;
  m_pBuffer->GetStatus(&bufferStatus);
  if (bufferStatus & DSBSTATUS_BUFFERLOST)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__ ": Buffer allocation was lost. Restoring buffer.");
    m_pBuffer->Restore();
  }

  while(GetSpace() < total)
    Sleep(1);

  while (len)
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

    memcpy(start, pBuffer, size);

    pBuffer += size;
    len     -= size;

    m_BufferOffset += size;
    if (startWrap) // Write-region wraps to beginning of buffer
    {
      memcpy(startWrap, pBuffer, sizeWrap);
      m_BufferOffset = sizeWrap;

      pBuffer += sizeWrap;
      len     -= sizeWrap;
    }

    m_CacheLen += size + sizeWrap; // This data is now in the cache
    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
  }

  CheckPlayStatus();

  return (total - len) / m_dwFrameSize; // Frames used
}

void CAESinkDirectSound::Stop()
{
  CSingleLock lock(m_runLock);

  if(m_pBuffer)
    m_pBuffer->Stop();
}

float CAESinkDirectSound::GetDelay()
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return 0.0f;

   // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  float delay  = 0.008f; // WTF?
  delay += (float)m_CacheLen / (float)m_AvgBytesPerSec;
  return delay;
}

void CAESinkDirectSound::EnumerateDevices(AEDeviceList &devices, bool passthrough)
{
  std::list<DSDevice> dev;
  if (FAILED(DirectSoundEnumerate(DSEnumCallback, &dev)))
    CLog::Log(LOGERROR, "%s - failed to enumerate output devices", __FUNCTION__);

  std::list<DSDevice>::iterator it;

  for(it = dev.begin(); it != dev.end(); it++)
  {
    devices.push_back(AEDevice((*it).name, CStdString("DIRECTSOUND:") + (*it).name));
  }
}

///////////////////////////////////////////////////////////////////////////////

void CAESinkDirectSound::CheckPlayStatus()
{
  DWORD status = 0;
  m_pBuffer->GetStatus(&status);

  if(!(status & DSBSTATUS_PLAYING) && m_CacheLen != 0) // If we have some data, see if we can start playback
  {
    HRESULT hr = m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    dserr2str(hr);
    CLog::Log(LOGDEBUG,__FUNCTION__ ": Resuming Playback");
  }
}

void CAESinkDirectSound::UpdateCacheStatus()
{
  CSingleLock lock (m_runLock);
  // TODO: Check to see if we may have cycled around since last time
  unsigned int time = CTimeUtils::GetTimeMS();
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

unsigned int CAESinkDirectSound::GetSpace()
{
  CSingleLock lock (m_runLock);
  UpdateCacheStatus();
  unsigned int space = m_dwBufferLen - m_CacheLen;

  // We can never allow the internal buffers to fill up complete
  // as we get confused between if the buffer is full or empty
  // so never allow the last chunk to be added
  if(space > m_dwChunkSize)
    return space - m_dwChunkSize;
  else
    return 0;
}

void CAESinkDirectSound::AEChannelsFromSpeakerMask(DWORD speakers)
{
  int j = 0;
  for(int i = 0; i < SPEAKER_COUNT; i++)
  {
    if(speakers & DSChannelOrder[i])
      m_channelLayout[j++] = AEChannelNames[i];
  }

  m_channelLayout[j] = AE_CH_NULL;
}

DWORD CAESinkDirectSound::SpeakerMaskFromAEChannels(AEChLayout channels)
{
  DWORD mask = 0;

  for(int i = 0; channels[i] != AE_CH_NULL; i++)
  {
    for(int j = 0; j < SPEAKER_COUNT; j++)
      if(channels[i] == AEChannelNames[j])
        mask |= DSChannelOrder[j];
  }

  return mask;
}

char *CAESinkDirectSound::dserr2str(int err)
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
