#pragma once

class CSong;
class CAlbum;

#include "utils/Archive.h"

namespace MUSIC_INFO
{

class CMusicInfoTag : public ISerializable
{
public:
  CMusicInfoTag(void);
  CMusicInfoTag(const CMusicInfoTag& tag);
  virtual ~CMusicInfoTag();
  const CMusicInfoTag& operator =(const CMusicInfoTag& tag);
  bool operator !=(const CMusicInfoTag& tag) const;
  bool Loaded() const;
  const CStdString& GetTitle() const;
  const CStdString& GetURL() const;
  const CStdString& GetArtist() const;
  const CStdString& GetAlbum() const;
  const CStdString& GetAlbumArtist() const;
  const CStdString& GetGenre() const;
  int GetTrackNumber() const;
  int GetDiscNumber() const;
  int GetTrackAndDiskNumber() const;
  int GetDuration() const;  // may be set even if Loaded() returns false
  int GetYear() const;
  long GetDatabaseId() const;

  void GetReleaseDate(SYSTEMTIME& dateTime) const;
  CStdString GetYearString() const;
  const CStdString& GetMusicBrainzTrackID() const;
  const CStdString& GetMusicBrainzArtistID() const;
  const CStdString& GetMusicBrainzAlbumID() const;
  const CStdString& GetMusicBrainzAlbumArtistID() const;
  const CStdString& GetMusicBrainzTRMID() const;
  const CStdString& GetComment() const;
  const CStdString& GetLyrics() const;
  char  GetRating() const;

  void SetURL(const CStdString& strURL);
  void SetTitle(const CStdString& strTitle);
  void SetArtist(const CStdString& strArtist);
  void SetAlbum(const CStdString& strAlbum);
  void SetAlbumArtist(const CStdString& strAlbumArtist);
  void SetGenre(const CStdString& strGenre);
  void SetYear(int year);
  void SetDatabaseId(long id);
  void SetReleaseDate(SYSTEMTIME& dateTime);
  void SetTrackNumber(int iTrack);
  void SetPartOfSet(int m_iPartOfSet);
  void SetTrackAndDiskNumber(int iTrackAndDisc);
  void SetDuration(int iSec);
  void SetLoaded(bool bOnOff = true);
  void SetAlbum(const CAlbum& album);
  void SetSong(const CSong& song);
  void SetMusicBrainzTrackID(const CStdString& strTrackID);
  void SetMusicBrainzArtistID(const CStdString& strArtistID);
  void SetMusicBrainzAlbumID(const CStdString& strAlbumID);
  void SetMusicBrainzAlbumArtistID(const CStdString& strAlbumArtistID);
  void SetMusicBrainzTRMID(const CStdString& strTRMID);
  void SetComment(const CStdString& comment);
  void SetLyrics(const CStdString& lyrics);
  void SetRating(char rating);

  /*! \brief Append a unique artist to the artist list
   Checks if we have this artist already added, and if not adds it to the songs artist list.
   \param value artist to add.
   */
  void AppendArtist(const CStdString &artist);

  /*! \brief Append a unique album artist to the artist list
   Checks if we have this album artist already added, and if not adds it to the songs album artist list.
   \param albumArtist album artist to add.
   */
  void AppendAlbumArtist(const CStdString &albumArtist);

  /*! \brief Append a unique genre to the genre list
   Checks if we have this genre already added, and if not adds it to the songs genre list.
   \param genre genre to add.
   */
  void AppendGenre(const CStdString &genre);

  virtual void Serialize(CArchive& ar);

  void Clear();
protected:
  /*! \brief Trim whitespace off the given string
   \param value string to trim
   \return trimmed value, with spaces removed from left and right, as well as carriage returns from the right.
   */
  CStdString Trim(const CStdString &value) const;

  CStdString m_strURL;
  CStdString m_strTitle;
  CStdString m_strArtist;
  CStdString m_strAlbum;
  CStdString m_strAlbumArtist;
  CStdString m_strGenre;
  CStdString m_strMusicBrainzTrackID;
  CStdString m_strMusicBrainzArtistID;
  CStdString m_strMusicBrainzAlbumID;
  CStdString m_strMusicBrainzAlbumArtistID;
  CStdString m_strMusicBrainzTRMID;
  CStdString m_strComment;
  CStdString m_strLyrics;
  int m_iDuration;
  int m_iTrack;     // consists of the disk number in the high 16 bits, the track number in the low 16bits
  long m_iDbId;
  bool m_bLoaded;
  char m_rating;
  SYSTEMTIME m_dwReleaseDate;
};
}
