#pragma once
#include "./HTTP.h"

namespace MUSIC_GRABBER
{
class CMusicAlbumInfo
{
public:
  CMusicAlbumInfo(void);
  CMusicAlbumInfo(const CStdString& strAlbumInfo, const CStdString& strAlbumURL);
  CMusicAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strAlbumInfo, const CStdString& strAlbumURL);
  virtual ~CMusicAlbumInfo(void);
  bool Loaded() const;
  void SetLoaded(bool bOnOff);
  void SetAlbum(CAlbum& album);
  const CAlbum &GetAlbum() const;
  void SetSongs(VECSONGS &songs);
  const VECSONGS &GetSongs() const;
  const CStdString& GetTitle2() const;
  const CStdString& GetDateOfRelease() const;
  const CStdString& GetAlbumURL() const;
  void SetTitle(const CStdString& strTitle);
  bool Load(CHTTP& http);
  bool Parse(const CStdString& strHTML, CHTTP& http);
protected:
  CAlbum m_album;
  CStdString m_strTitle2;
  CStdString m_strDateOfRelease;
  CStdString m_strAlbumURL;
  bool m_bLoaded;
  VECSONGS m_songs;
};
}
