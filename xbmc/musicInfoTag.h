#pragma once

class CSong;
class CAlbum;

#include "utils/archive.h"

namespace MUSIC_INFO
{

class CMusicInfoTag : public ISerializable
{
public:
  CMusicInfoTag(void);
  CMusicInfoTag(const CMusicInfoTag& tag);
  virtual ~CMusicInfoTag();
  const CMusicInfoTag& operator =(const CMusicInfoTag& tag);
  bool CMusicInfoTag::operator !=(const CMusicInfoTag& tag) const;
  bool Load(const CStdString& strFileName);
  void Save(const CStdString& strFileName);
  bool Loaded() const;
  const CStdString& GetTitle() const;
  const CStdString& GetURL() const;
  const CStdString& GetArtist() const;
  const CStdString& GetAlbum() const;
  const CStdString& GetGenre() const;
  int GetTrackNumber() const;
  int GetDiscNumber() const;
  int GetTrackAndDiskNumber() const;
  int GetDuration() const;  // may be set even if Loaded() returns false
  void GetReleaseDate(SYSTEMTIME& dateTime) const;
  CStdString GetYear() const;
  const CStdString& GetMusicBrainzTrackID() const;
  const CStdString& GetMusicBrainzArtistID() const;
  const CStdString& GetMusicBrainzAlbumID() const;
  const CStdString& GetMusicBrainzAlbumArtistID() const;
  const CStdString& GetMusicBrainzTRMID() const;

  void SetURL(const CStdString& strURL) ;
  void SetTitle(const CStdString& strTitle) ;
  void SetArtist(const CStdString& strArtist) ;
  void SetAlbum(const CStdString& strAlbum) ;
  void SetGenre(const CStdString& strGenre) ;
  void SetReleaseDate(SYSTEMTIME& dateTime);
  void SetTrackNumber(int iTrack);
  void SetPartOfSet(int m_iPartOfSet);
  void SetDuration(int iSec);
  void SetLoaded(bool bOnOff = true);
  void SetAlbum(const CAlbum& album);
  void SetSong(const CSong& song);
  void SetMusicBrainzTrackID(const CStdString& strTrackID);
  void SetMusicBrainzArtistID(const CStdString& strArtistID);
  void SetMusicBrainzAlbumID(const CStdString& strAlbumID);
  void SetMusicBrainzAlbumArtistID(const CStdString& strAlbumArtistID);
  void SetMusicBrainzTRMID(const CStdString& strTRMID);

  virtual void Serialize(CArchive& ar);

  void Clear();
protected:
  CStdString m_strURL;
  CStdString m_strTitle;
  CStdString m_strArtist;
  CStdString m_strAlbum;
  CStdString m_strGenre;
  CStdString m_strMusicBrainzTrackID;
  CStdString m_strMusicBrainzArtistID;
  CStdString m_strMusicBrainzAlbumID;
  CStdString m_strMusicBrainzAlbumArtistID;
  CStdString m_strMusicBrainzTRMID;
  int m_iDuration;
  int m_iTrack;     // consists of the disk number in the high 16 bits, the track number in the low 16bits
  bool m_bLoaded;
  SYSTEMTIME m_dwReleaseDate;
};
};
