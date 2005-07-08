
#include "stdafx.h"
#include "APEv2tag.h"

using namespace MUSIC_INFO;

CAPEv2Tag::CAPEv2Tag()
{
  m_nTrackNum = 0;
  m_nDiscNum = 0;
  m_bDllLoaded = false;
  m_pDll = NULL;
  GetAPETag = NULL;
}

CAPEv2Tag::~CAPEv2Tag()
{
  if (m_pDll)
    CSectionLoader::UnloadDLL(APE_DLL);
  m_pDll = NULL;
}

bool CAPEv2Tag::ReadTag(const char* filename, bool checkID3Tag)
{
  if (!filename || !LoadDLL())
    return false;

  // Read in our tag using our dll
  IAPETag *tag = GetAPETag(filename, checkID3Tag);
  if (!tag)
    return false;

  int chars = 256;
  char buffer[256];
  if (tag->GetFieldString(L"Title", buffer, &chars, TRUE) != -1)
    g_charsetConverter.utf8ToStringCharset(buffer, m_strTitle);
  chars = 256;
  if (tag->GetFieldString(L"Album", buffer, &chars, TRUE) != -1)
    g_charsetConverter.utf8ToStringCharset(buffer, m_strAlbum);
  chars = 256;
  if (tag->GetFieldString(L"Artist", buffer, &chars, TRUE) != -1)
    g_charsetConverter.utf8ToStringCharset(buffer, m_strArtist);
  chars = 256;
  if (tag->GetFieldString(L"Genre", buffer, &chars, TRUE) != -1)
    g_charsetConverter.utf8ToStringCharset(buffer, m_strGenre);
  chars = 256;
  if (tag->GetFieldString(L"Year", buffer, &chars, TRUE) != -1)
    g_charsetConverter.utf8ToStringCharset(buffer, m_strYear);
  chars = 256;
  if (tag->GetFieldString(L"Track", buffer, &chars, TRUE) != -1)
    m_nTrackNum = atoi(buffer);
  chars = 256;
  if (tag->GetFieldString(L"Media", buffer, &chars, TRUE) != -1)
  {
    // cd number is usually "CD 1/3"
    char *num = buffer;
    while (!isdigit(*num) && num < buffer + chars) num++;
    if (isdigit(*num))
      m_nDiscNum = atoi(num);
  }

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
  m_pDll->ResolveExport("_GetAPETag@8", (void**)&GetAPETag);

  // Check resolves + version number
  if ( !GetAPETag )
  {
    CLog::Log(LOGERROR, "CApeTag: Unable to get needed export functions from our dll %s", APE_DLL);
    CSectionLoader::UnloadDLL(APE_DLL);
    m_pDll = NULL;
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

