#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

class CSong;
class CAlbum;

#include "utils/Archive.h"
#include "utils/ISerializable.h"
#include "XBDateTime.h"

namespace MUSIC_INFO
{

class CMusicInfoTag : public IArchivable, public ISerializable
{
public:
  CMusicInfoTag(const CStdString& strMediaFile = CStdString());
  CMusicInfoTag(const CMusicInfoTag& tag);
  virtual ~CMusicInfoTag();
  const CMusicInfoTag& operator =(const CMusicInfoTag& tag);
  bool operator !=(const CMusicInfoTag& tag) const;
  bool Loaded() const;
  const CStdString& GetTitle() const;
  const CStdString& GetURL() const;
  const std::vector<std::string>& GetArtist() const;
  const CStdString& GetAlbum() const;
  const std::vector<std::string>& GetAlbumArtist() const;
  const std::vector<std::string> GetGenre() const;
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
  const CDateTime& GetLastPlayed() const;
  char  GetRating() const;
  int  GetListeners() const;
  int  GetPlayCount() const;

  void SetURL(const CStdString& strURL);
  void SetTitle(const CStdString& strTitle);
  void SetArtist(const CStdString& strArtist);
  void SetArtist(const std::vector<std::string>& artists);
  void SetArtistId(const int iArtistId);
  void SetAlbum(const CStdString& strAlbum);
  void SetAlbumId(const int iAlbumId);
  void SetAlbumArtist(const CStdString& strAlbumArtist);
  void SetAlbumArtist(const std::vector<std::string>& albumArtists);
  void SetGenre(const CStdString& strGenre);
  void SetGenre(const std::vector<std::string>& genres);
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
  void SetListeners(int listeners);
  void SetPlayCount(int playcount);
  void SetLastPlayed(const CStdString& strLastPlayed);
  void SetLastPlayed(const CDateTime& strLastPlayed);

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

  bool HasEmbeddedCue() const;
  const CStdString& GetEmbeddedCue() const;
  void setEmbeddedCue(const CStdString& cuesheet);

  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& ar);

  void Clear();
protected:
  /*! \brief Trim whitespace off the given string
   \param value string to trim
   \return trimmed value, with spaces removed from left and right, as well as carriage returns from the right.
   */
  CStdString Trim(const CStdString &value) const;

  CStdString m_strURL;
  CStdString m_strTitle;
  std::vector<std::string> m_artist;
  CStdString m_strAlbum;
  std::vector<std::string> m_albumArtist;
  std::vector<std::string> m_genre;
  CStdString m_strMusicBrainzTrackID;
  CStdString m_strMusicBrainzArtistID;
  CStdString m_strMusicBrainzAlbumID;
  CStdString m_strMusicBrainzAlbumArtistID;
  CStdString m_strMusicBrainzTRMID;
  CStdString m_strComment;
  CStdString m_strLyrics;
  CStdString m_strCue;
  CDateTime m_lastPlayed;
  int m_iDuration;
  int m_iTrack;     // consists of the disk number in the high 16 bits, the track number in the low 16bits
  long m_iDbId;
  bool m_bLoaded;
  char m_rating;
  int m_listeners;
  int m_iTimesPlayed;
  int m_iArtistId;
  int m_iAlbumId;
  SYSTEMTIME m_dwReleaseDate;
};
}
