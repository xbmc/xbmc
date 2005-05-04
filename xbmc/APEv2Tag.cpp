
#include "stdafx.h"
#include "APEv2tag.h"

using namespace MUSIC_INFO;

#define APE_DLL "Q:\\system\\players\\PAPlayer\\MACDll.dll"

CAPEv2Tag::CAPEv2Tag()
{
  m_nTrackNum = 0;
  m_bDllLoaded = false;
  m_pDll = NULL;
  GetAPETag = NULL;
  GetAPEDuration = NULL;
}

CAPEv2Tag::~CAPEv2Tag()
{
  if (m_pDll)
    CSectionLoader::UnloadDLL(APE_DLL);
  m_pDll = NULL;
}

bool CAPEv2Tag::ReadTag(const char* filename)
{
  if (!filename || !LoadDLL())
    return false;

  // Read in our tag using our dll
  IAPETag *tag = GetAPETag(filename);
  if (!tag)
    return false;

  int chars = 256;
  char buffer[256];
  if (tag->GetFieldString(L"Title", buffer, &chars, TRUE) != -1)
    m_strTitle = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Album", buffer, &chars, TRUE) != -1)
    m_strAlbum = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Artist", buffer, &chars, TRUE) != -1)
    m_strArtist = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Genre", buffer, &chars, TRUE) != -1)
    m_strGenre = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Year", buffer, &chars, TRUE) != -1)
    m_strYear = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Track", buffer, &chars, TRUE) != -1)
    m_nTrackNum = atoi(buffer);

  // Replay gain info
  GetReplayGainFromTag(tag);

  delete tag;
  return true;
}

void CAPEv2Tag::GetReplayGainFromTag(IAPETag *tag)
{
  if (!tag) return;
  char buffer[16];
  int chars = 16;
  if (tag->GetFieldString(L"replaygain_track_gain", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.iTrackGain = (int)(atof(buffer)*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  chars = 16;
  if (tag->GetFieldString(L"replaygain_track_peak", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.fTrackPeak = (float)atof(buffer);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  chars = 16;
  if (tag->GetFieldString(L"replaygain_album_gain", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.iAlbumGain = (int)(atof(buffer)*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  chars = 16;
  if (tag->GetFieldString(L"replaygain_album_peak", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.fAlbumPeak = (float)atof(buffer);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }
}

__int64 CAPEv2Tag::ReadDuration(const char* filename)
{
  if (!filename || !LoadDLL())
    return 0;
  return GetAPEDuration(filename);
}

bool CAPEv2Tag::LoadDLL()
{
  if (m_bDllLoaded)
    return true;
  m_pDll = CSectionLoader::LoadDLL(APE_DLL);
  if (!m_pDll)
  {
    CLog::Log(LOGERROR, "CAPEv2Tag::Unable to load dll %s", APE_DLL);
    return false;
  }

  // get handle to the functions in the dll
  m_pDll->ResolveExport("_GetAPETag@4", (void**)&GetAPETag);
  m_pDll->ResolveExport("_GetAPEDuration@4", (void**)&GetAPEDuration);

  // Check resolves + version number
  if ( !GetAPETag || !GetAPEDuration )
  {
    CLog::Log(LOGERROR, "CApeTag: Unable to get needed export functions from our dll %s", APE_DLL);
    delete m_pDll;
    m_pDll = NULL;
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

