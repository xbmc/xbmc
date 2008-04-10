#pragma once
#include "Song.h"
#include "Album.h"

class TiXmlDocument;
class CScraperUrl;
struct SScraperInfo;
class CHTTP;

namespace MUSIC_GRABBER
{
class CMusicAlbumInfo
{
public:
  CMusicAlbumInfo(void);
  CMusicAlbumInfo(const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL);
  CMusicAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL);
  virtual ~CMusicAlbumInfo(void);
  bool Loaded() const;
  void SetLoaded(bool bOnOff);
  void SetAlbum(CAlbum& album);
  const CAlbum &GetAlbum() const;
  CAlbum& GetAlbum();
  void SetSongs(VECSONGS &songs);
  const VECSONGS &GetSongs() const;
  const CStdString& GetTitle2() const;
  const CStdString& GetDateOfRelease() const;
  const CScraperUrl& GetAlbumURL() const;
  void SetTitle(const CStdString& strTitle);
  bool Load(CHTTP& http, const SScraperInfo& info, const CStdString& strFunction="GetAlbumDetails", const CScraperUrl* url=NULL);
  bool Parse(const TiXmlElement* album, bool bChained=false);
protected:
  CAlbum m_album;
  CStdString m_strTitle2;
  CScraperUrl m_albumURL;
  bool m_bLoaded;
};
}
