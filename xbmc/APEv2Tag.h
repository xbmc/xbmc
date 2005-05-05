//------------------------------
// CApeTag in 2005 by JMarshall
//------------------------------
#include "cores/DllLoader/dll.h"
#include "cores/paplayer/ReplayGain.h"

#define APE_DLL "Q:\\system\\players\\PAPlayer\\MACDll.dll"

typedef wchar_t str_utf16;
typedef char str_ansi;

class IAPETag
{
public:
    virtual ~IAPETag() {}
    virtual int GetFieldString(const str_utf16 * pFieldName, str_ansi * pBuffer, int * pBufferCharacters, BOOL bUTF8Encode = FALSE)=0;
};

namespace MUSIC_INFO
{

#pragma once

class CAPEv2Tag
{
public:
  CAPEv2Tag(void);
  virtual ~CAPEv2Tag(void);
  bool ReadTag(const char* filename);
  __int64 ReadMPCDuration(const char* filename);
  CStdString GetTitle() { return m_strTitle; }
  CStdString GetArtist() { return m_strArtist; }
  CStdString GetYear() { return m_strYear; }
  CStdString GetAlbum() { return m_strAlbum; }
  CStdString GetGenre() { return m_strGenre; }
  int GetTrackNum() { return m_nTrackNum; }
  CStdString GetMusicBrainzTrackID() { return m_strMusicBrainzTrackID; }
  CStdString GetMusicBrainzArtistID() { return m_strMusicBrainzArtistID; }
  CStdString GetMusicBrainzAlbumID() { return m_strMusicBrainzAlbumID; }
  CStdString GetMusicBrainzAlbumArtistID() { return m_strMusicBrainzAlbumArtistID; }
  CStdString GetMusicBrainzTRMID() { return m_strMusicBrainzTRMID; }
  void GetReplayGainFromTag(IAPETag *pTag);
  const CReplayGain &GetReplayGain() { return m_replayGain; };
protected:
  CStdString m_strTitle;
  CStdString m_strArtist;
  CStdString m_strYear;
  CStdString m_strAlbum;
  CStdString m_strGenre;
  int m_nTrackNum;
  CStdString m_strMusicBrainzTrackID;
  CStdString m_strMusicBrainzArtistID;
  CStdString m_strMusicBrainzAlbumID;
  CStdString m_strMusicBrainzAlbumArtistID;
  CStdString m_strMusicBrainzTRMID;
  CReplayGain m_replayGain;
  __int64 m_nDuration;
private:
  DllLoader *m_pDll;
  IAPETag * (__stdcall* GetAPETag)(const char *filename);
  bool LoadDLL();                     // load the DLL in question
  bool m_bDllLoaded;                  // whether our dll is loaded
};
};
