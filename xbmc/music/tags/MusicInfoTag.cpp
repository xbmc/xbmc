/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "MusicInfoTag.h"
#include "music/Album.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "utils/Variant.h"

using namespace MUSIC_INFO;

CMusicInfoTag::CMusicInfoTag(void)
{
  Clear();
}

CMusicInfoTag::CMusicInfoTag(const CMusicInfoTag& tag)
{
  *this = tag;
}

CMusicInfoTag::~CMusicInfoTag()
{}

const CMusicInfoTag& CMusicInfoTag::operator =(const CMusicInfoTag& tag)
{
  if (this == &tag) return * this;

  m_strURL = tag.m_strURL;
  m_artist = tag.m_artist;
  m_albumArtist = tag.m_albumArtist;
  m_strAlbum = tag.m_strAlbum;
  m_genre = tag.m_genre;
  m_strTitle = tag.m_strTitle;
  m_strMusicBrainzTrackID = tag.m_strMusicBrainzTrackID;
  m_strMusicBrainzArtistID = tag.m_strMusicBrainzArtistID;
  m_strMusicBrainzAlbumID = tag.m_strMusicBrainzAlbumID;
  m_strMusicBrainzAlbumArtistID = tag.m_strMusicBrainzAlbumArtistID;
  m_strMusicBrainzTRMID = tag.m_strMusicBrainzTRMID;
  m_strComment = tag.m_strComment;
  m_strLyrics = tag.m_strLyrics;
  m_lastPlayed = tag.m_lastPlayed;
  m_iDuration = tag.m_iDuration;
  m_iTrack = tag.m_iTrack;
  m_bLoaded = tag.m_bLoaded;
  m_rating = tag.m_rating;
  m_listeners = tag.m_listeners;
  m_iTimesPlayed = tag.m_iTimesPlayed;
  m_iDbId = tag.m_iDbId;
  m_iArtistId = tag.m_iArtistId;
  m_iAlbumId = tag.m_iAlbumId;
  memcpy(&m_dwReleaseDate, &tag.m_dwReleaseDate, sizeof(m_dwReleaseDate) );
  return *this;
}

bool CMusicInfoTag::operator !=(const CMusicInfoTag& tag) const
{
  if (this == &tag) return false;
  if (m_strURL != tag.m_strURL) return true;
  if (m_strTitle != tag.m_strTitle) return true;
  for (unsigned int index = 0; index < m_artist.size(); index++)
  {
    if (tag.m_artist.at(index).compare(m_artist.at(index)) != 0)
      return true;
  }
  if (m_artist != tag.m_artist) return true;
  if (m_albumArtist != tag.m_albumArtist) return true;
  if (m_strAlbum != tag.m_strAlbum) return true;
  if (m_iDuration != tag.m_iDuration) return true;
  if (m_iTrack != tag.m_iTrack) return true;
  return false;
}

int CMusicInfoTag::GetTrackNumber() const
{
  return (m_iTrack & 0xffff);
}

int CMusicInfoTag::GetDiscNumber() const
{
  return (m_iTrack >> 16);
}

int CMusicInfoTag::GetTrackAndDiskNumber() const
{
  return m_iTrack;
}

int CMusicInfoTag::GetDuration() const
{
  return m_iDuration;
}

const CStdString& CMusicInfoTag::GetTitle() const
{
  return m_strTitle;
}

const CStdString& CMusicInfoTag::GetURL() const
{
  return m_strURL;
}

const std::vector<std::string>& CMusicInfoTag::GetArtist() const
{
  return m_artist;
}

const CStdString& CMusicInfoTag::GetAlbum() const
{
  return m_strAlbum;
}

const std::vector<std::string>& CMusicInfoTag::GetAlbumArtist() const
{
  return m_albumArtist;
}

const std::vector<std::string> CMusicInfoTag::GetGenre() const
{
  return m_genre;
}

void CMusicInfoTag::GetReleaseDate(SYSTEMTIME& dateTime) const
{
  memcpy(&dateTime, &m_dwReleaseDate, sizeof(m_dwReleaseDate) );
}

int CMusicInfoTag::GetYear() const
{
  return m_dwReleaseDate.wYear;
}

long CMusicInfoTag::GetDatabaseId() const
{
  return m_iDbId;
}

CStdString CMusicInfoTag::GetYearString() const
{
  CStdString strReturn;
  strReturn.Format("%i", m_dwReleaseDate.wYear);
  return m_dwReleaseDate.wYear ? strReturn : "";
}

const CStdString &CMusicInfoTag::GetComment() const
{
  return m_strComment;
}

const CStdString &CMusicInfoTag::GetLyrics() const
{
  return m_strLyrics;
}

char CMusicInfoTag::GetRating() const
{
  return m_rating;
}

int CMusicInfoTag::GetListeners() const
{
 return m_listeners;
}

int CMusicInfoTag::GetPlayCount() const
{
  return m_iTimesPlayed;
}

const CDateTime &CMusicInfoTag::GetLastPlayed() const
{
  return m_lastPlayed;
}

void CMusicInfoTag::SetURL(const CStdString& strURL)
{
  m_strURL = strURL;
}

void CMusicInfoTag::SetTitle(const CStdString& strTitle)
{
  m_strTitle = Trim(strTitle);
}

void CMusicInfoTag::SetArtist(const CStdString& strArtist)
{
  SetArtist(StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator));
}

void CMusicInfoTag::SetArtist(const std::vector<std::string>& artists)
{
  m_artist = artists;
}

void CMusicInfoTag::SetArtistId(const int iArtistId)
{
  m_iArtistId = iArtistId;
}

void CMusicInfoTag::SetAlbum(const CStdString& strAlbum)
{
  m_strAlbum = Trim(strAlbum);
}

void CMusicInfoTag::SetAlbumId(const int iAlbumId)
{
  m_iAlbumId = iAlbumId;
}

void CMusicInfoTag::SetAlbumArtist(const CStdString& strAlbumArtist)
{
  SetAlbumArtist(StringUtils::Split(strAlbumArtist, g_advancedSettings.m_musicItemSeparator));
}

void CMusicInfoTag::SetAlbumArtist(const std::vector<std::string>& albumArtists)
{
  m_albumArtist = albumArtists;
}

void CMusicInfoTag::SetGenre(const CStdString& strGenre)
{
  SetGenre(StringUtils::Split(strGenre, g_advancedSettings.m_musicItemSeparator));
}

void CMusicInfoTag::SetGenre(const std::vector<std::string>& genres)
{
  m_genre = genres;
}

void CMusicInfoTag::SetYear(int year)
{
  memset(&m_dwReleaseDate, 0, sizeof(m_dwReleaseDate) );
  m_dwReleaseDate.wYear = year;
}

void CMusicInfoTag::SetDatabaseId(long id)
{
  m_iDbId = id;
}

void CMusicInfoTag::SetReleaseDate(SYSTEMTIME& dateTime)
{
  memcpy(&m_dwReleaseDate, &dateTime, sizeof(m_dwReleaseDate) );
}

void CMusicInfoTag::SetTrackNumber(int iTrack)
{
  m_iTrack = (m_iTrack & 0xffff0000) | (iTrack & 0xffff);
}

void CMusicInfoTag::SetPartOfSet(int iPartOfSet)
{
  m_iTrack = (m_iTrack & 0xffff) | (iPartOfSet << 16);
}

void CMusicInfoTag::SetTrackAndDiskNumber(int iTrackAndDisc)
{
  m_iTrack=iTrackAndDisc;
}

void CMusicInfoTag::SetDuration(int iSec)
{
  m_iDuration = iSec;
}

void CMusicInfoTag::SetComment(const CStdString& comment)
{
  m_strComment = comment;
}

void CMusicInfoTag::SetLyrics(const CStdString& lyrics)
{
  m_strLyrics = lyrics;
}

void CMusicInfoTag::SetRating(char rating)
{
  m_rating = rating;
}

void CMusicInfoTag::SetListeners(int listeners)
{
 m_listeners = listeners;
}

void CMusicInfoTag::SetPlayCount(int playcount)
{
  m_iTimesPlayed = playcount;
}

void CMusicInfoTag::SetLastPlayed(const CStdString& lastplayed)
{
  m_lastPlayed.SetFromDBDateTime(lastplayed);
}

void CMusicInfoTag::SetLastPlayed(const CDateTime& lastplayed)
{
  m_lastPlayed = lastplayed;
}

void CMusicInfoTag::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicInfoTag::Loaded() const
{
  return m_bLoaded;
}

const CStdString& CMusicInfoTag::GetMusicBrainzTrackID() const
{
  return m_strMusicBrainzTrackID;
}

const CStdString& CMusicInfoTag::GetMusicBrainzArtistID() const
{
  return m_strMusicBrainzArtistID;
}

const CStdString& CMusicInfoTag::GetMusicBrainzAlbumID() const
{
  return m_strMusicBrainzAlbumID;
}

const CStdString& CMusicInfoTag::GetMusicBrainzAlbumArtistID() const
{
  return m_strMusicBrainzAlbumArtistID;
}

const CStdString& CMusicInfoTag::GetMusicBrainzTRMID() const
{
  return m_strMusicBrainzTRMID;
}

void CMusicInfoTag::SetMusicBrainzTrackID(const CStdString& strTrackID)
{
  m_strMusicBrainzTrackID=strTrackID;
}

void CMusicInfoTag::SetMusicBrainzArtistID(const CStdString& strArtistID)
{
  m_strMusicBrainzArtistID=strArtistID;
}

void CMusicInfoTag::SetMusicBrainzAlbumID(const CStdString& strAlbumID)
{
  m_strMusicBrainzAlbumID=strAlbumID;
}

void CMusicInfoTag::SetMusicBrainzAlbumArtistID(const CStdString& strAlbumArtistID)
{
  m_strMusicBrainzAlbumArtistID=strAlbumArtistID;
}

void CMusicInfoTag::SetMusicBrainzTRMID(const CStdString& strTRMID)
{
  m_strMusicBrainzTRMID=strTRMID;
}

void CMusicInfoTag::SetAlbum(const CAlbum& album)
{
  SetArtist(album.artist);
  SetAlbum(album.strAlbum);
  SetAlbumArtist(StringUtils::Join(album.artist, g_advancedSettings.m_musicItemSeparator));
  SetGenre(album.genre);
  SetRating('0' + (album.iRating + 1) / 2);
  SYSTEMTIME stTime;
  stTime.wYear = album.iYear;
  SetReleaseDate(stTime);
  m_iDbId = album.idAlbum;
  m_bLoaded = true;
  m_iArtistId = album.idArtist;
}

void CMusicInfoTag::SetSong(const CSong& song)
{
  SetTitle(song.strTitle);
  SetGenre(song.genre);
  SetArtist(song.artist);
  SetAlbum(song.strAlbum);
  SetAlbumArtist(song.albumArtist);
  SetMusicBrainzTrackID(song.strMusicBrainzTrackID);
  SetMusicBrainzArtistID(song.strMusicBrainzArtistID);
  SetMusicBrainzAlbumID(song.strMusicBrainzAlbumID);
  SetMusicBrainzAlbumArtistID(song.strMusicBrainzAlbumArtistID);
  SetMusicBrainzTRMID(song.strMusicBrainzTRMID);
  SetComment(song.strComment);
  SetPlayCount(song.iTimesPlayed);
  SetLastPlayed(song.lastPlayed);
  m_rating = song.rating;
  m_strURL = song.strFileName;
  SYSTEMTIME stTime;
  stTime.wYear = song.iYear;
  SetReleaseDate(stTime);
  m_iTrack = song.iTrack;
  m_iDuration = song.iDuration;
  m_iDbId = song.idSong;
  m_bLoaded = true;
  m_iTimesPlayed = song.iTimesPlayed;
  m_iArtistId = song.iArtistId;
  m_iAlbumId = song.iAlbumId;
}

void CMusicInfoTag::Serialize(CVariant& value)
{
  /* TODO:
     All the StringUtils::Join() calls can be removed once backwards-compatibility to
     JSON-RPC v4 can be broken */
  value["url"] = m_strURL;
  value["title"] = m_strTitle;
  value["artist"] = StringUtils::Join(m_artist, g_advancedSettings.m_musicItemSeparator);
  value["album"] = m_strAlbum;
  value["albumartist"] = StringUtils::Join(m_albumArtist, g_advancedSettings.m_musicItemSeparator);
  value["genre"] = StringUtils::Join(m_genre, g_advancedSettings.m_musicItemSeparator);
  value["duration"] = m_iDuration;
  value["track"] = m_iTrack;
  value["loaded"] = m_bLoaded;
  value["year"] = m_dwReleaseDate.wYear;
  value["musicbrainztrackid"] = m_strMusicBrainzTrackID;
  value["musicbrainzartistid"] = m_strMusicBrainzArtistID;
  value["musicbrainzalbumid"] = m_strMusicBrainzAlbumID;
  value["musicbrainzalbumartistid"] = m_strMusicBrainzAlbumArtistID;
  value["musicbrainztrmid"] = m_strMusicBrainzTRMID;
  value["comment"] = m_strComment;
  value["rating"] = m_rating;
  value["playcount"] = m_iTimesPlayed;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::EmptyString;
  value["lyrics"] = m_strLyrics;
  value["artistid"] = m_iArtistId;
  value["albumid"] = m_iAlbumId;
}
void CMusicInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strURL;
    ar << m_strTitle;
    ar << m_artist;
    ar << m_strAlbum;
    ar << m_albumArtist;
    ar << m_genre;
    ar << m_iDuration;
    ar << m_iTrack;
    ar << m_bLoaded;
    ar << m_dwReleaseDate;
    ar << m_strMusicBrainzTrackID;
    ar << m_strMusicBrainzArtistID;
    ar << m_strMusicBrainzAlbumID;
    ar << m_strMusicBrainzAlbumArtistID;
    ar << m_strMusicBrainzTRMID;
    ar << m_lastPlayed;
    ar << m_strComment;
    ar << m_rating;
    ar << m_iTimesPlayed;
    ar << m_iArtistId;
    ar << m_iAlbumId;
  }
  else
  {
    ar >> m_strURL;
    ar >> m_strTitle;
    ar >> m_artist;
    ar >> m_strAlbum;
    ar >> m_albumArtist;
    ar >> m_genre;
    ar >> m_iDuration;
    ar >> m_iTrack;
    ar >> m_bLoaded;
    ar >> m_dwReleaseDate;
    ar >> m_strMusicBrainzTrackID;
    ar >> m_strMusicBrainzArtistID;
    ar >> m_strMusicBrainzAlbumID;
    ar >> m_strMusicBrainzAlbumArtistID;
    ar >> m_strMusicBrainzTRMID;
    ar >> m_lastPlayed;
    ar >> m_strComment;
    ar >> m_rating;
    ar >> m_iTimesPlayed;
    ar >> m_iArtistId;
    ar >> m_iAlbumId;
 }
}

void CMusicInfoTag::Clear()
{
  m_strURL.Empty();
  m_artist.clear();
  m_strAlbum.Empty();
  m_albumArtist.clear();
  m_genre.clear();
  m_strTitle.Empty();
  m_strMusicBrainzTrackID.Empty();
  m_strMusicBrainzArtistID.Empty();
  m_strMusicBrainzAlbumID.Empty();
  m_strMusicBrainzAlbumArtistID.Empty();
  m_strMusicBrainzTRMID.Empty();
  m_iDuration = 0;
  m_iTrack = 0;
  m_bLoaded = false;
  m_lastPlayed.Reset();
  m_strComment.Empty();
  m_rating = '0';
  m_iDbId = -1;
  m_iTimesPlayed = 0;
  memset(&m_dwReleaseDate, 0, sizeof(m_dwReleaseDate) );
  m_iArtistId = -1;
  m_iAlbumId = -1;
}

void CMusicInfoTag::AppendArtist(const CStdString &artist)
{
  for (unsigned int index = 0; index < m_artist.size(); index++)
  {
    if (artist.Equals(m_artist.at(index).c_str()))
      return;
  }

  m_artist.push_back(artist);
}

void CMusicInfoTag::AppendAlbumArtist(const CStdString &albumArtist)
{
  for (unsigned int index = 0; index < m_albumArtist.size(); index++)
  {
    if (albumArtist.Equals(m_albumArtist.at(index).c_str()))
      return;
  }

  m_artist.push_back(albumArtist);
}

void CMusicInfoTag::AppendGenre(const CStdString &genre)
{
  for (unsigned int index = 0; index < m_genre.size(); index++)
  {
    if (genre.Equals(m_genre.at(index).c_str()))
      return;
  }

  m_genre.push_back(genre);
}

CStdString CMusicInfoTag::Trim(const CStdString &value) const
{
  CStdString trimmedValue(value);
  trimmedValue.TrimLeft(' ');
  trimmedValue.TrimRight(" \n\r");
  return trimmedValue;
}
