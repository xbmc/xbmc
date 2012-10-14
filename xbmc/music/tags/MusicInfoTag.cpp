/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MusicInfoTag.h"
#include "music/Album.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "utils/Variant.h"

using namespace MUSIC_INFO;

EmbeddedArtInfo::EmbeddedArtInfo(size_t siz, const std::string &mim)
{
  set(siz, mim);
}

void EmbeddedArtInfo::set(size_t siz, const std::string &mim)
{
  size = siz;
  mime = mim;
}

void EmbeddedArtInfo::clear()
{
  mime.clear();
  size = 0;
}

bool EmbeddedArtInfo::empty() const
{
  return size == 0;
}

bool EmbeddedArtInfo::matches(const EmbeddedArtInfo &right) const
{
  return (size == right.size &&
          mime == right.mime);
}

EmbeddedArt::EmbeddedArt(const uint8_t *dat, size_t siz, const std::string &mim)
{
  set(dat, siz, mim);
}

void EmbeddedArt::set(const uint8_t *dat, size_t siz, const std::string &mim)
{
  EmbeddedArtInfo::set(siz, mim);
  data.resize(siz);
  memcpy(&data[0], dat, siz);
}

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
  m_bCompilation = tag.m_bCompilation;
  m_iDuration = tag.m_iDuration;
  m_iTrack = tag.m_iTrack;
  m_bLoaded = tag.m_bLoaded;
  m_rating = tag.m_rating;
  m_listeners = tag.m_listeners;
  m_iTimesPlayed = tag.m_iTimesPlayed;
  m_iDbId = tag.m_iDbId;
  m_type = tag.m_type;
  m_iAlbumId = tag.m_iAlbumId;
  m_iTrackGain = tag.m_iTrackGain;
  m_iAlbumGain = tag.m_iAlbumGain;
  m_fTrackPeak = tag.m_fTrackPeak;
  m_fAlbumPeak = tag.m_fAlbumPeak;
  m_iHasGainInfo = tag.m_iHasGainInfo;

  memcpy(&m_dwReleaseDate, &tag.m_dwReleaseDate, sizeof(m_dwReleaseDate) );
  m_coverArt = tag.m_coverArt;
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
  if (m_bCompilation != tag.m_bCompilation) return true;
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

int CMusicInfoTag::GetAlbumId() const
{
  return m_iAlbumId;
}

const std::vector<std::string>& CMusicInfoTag::GetAlbumArtist() const
{
  return m_albumArtist;
}

const std::vector<std::string>& CMusicInfoTag::GetGenre() const
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

const std::string &CMusicInfoTag::GetType() const
{
  return m_type;
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

bool CMusicInfoTag::GetCompilation() const
{
  return m_bCompilation;
}

const EmbeddedArtInfo &CMusicInfoTag::GetCoverArtInfo() const
{
  return m_coverArt;
}

int CMusicInfoTag::GetReplayGainTrackGain() const
{
  return m_iTrackGain;
}

int CMusicInfoTag::GetReplayGainAlbumGain() const
{
  return m_iAlbumGain;
}

float CMusicInfoTag::GetReplayGainTrackPeak() const
{
  return m_fTrackPeak;
}

float CMusicInfoTag::GetReplayGainAlbumPeak() const
{
  return m_fAlbumPeak;
}

int CMusicInfoTag::HasReplayGainInfo() const
{
  return m_iHasGainInfo;
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
  if (!strArtist.empty())
    SetArtist(StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator));
  else
    m_artist.clear();
}

void CMusicInfoTag::SetArtist(const std::vector<std::string>& artists)
{
  m_artist = artists;
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
  if (!strAlbumArtist.empty())
    SetAlbumArtist(StringUtils::Split(strAlbumArtist, g_advancedSettings.m_musicItemSeparator));
  else
    m_albumArtist.clear();
}

void CMusicInfoTag::SetAlbumArtist(const std::vector<std::string>& albumArtists)
{
  m_albumArtist = albumArtists;
}

void CMusicInfoTag::SetGenre(const CStdString& strGenre)
{
  if (!strGenre.empty())
    SetGenre(StringUtils::Split(strGenre, g_advancedSettings.m_musicItemSeparator));
  else
    m_genre.clear();
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

void CMusicInfoTag::SetDatabaseId(long id, const std::string &type)
{
  m_iDbId = id;
  m_type = type;
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

void CMusicInfoTag::SetCompilation(bool compilation)
{
  m_bCompilation = compilation;
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

void CMusicInfoTag::SetCoverArtInfo(size_t size, const std::string &mimeType)
{
  m_coverArt.set(size, mimeType);
}

void CMusicInfoTag::SetReplayGainTrackGain(int trackGain)
{
  m_iTrackGain = trackGain;
  m_iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
}

void CMusicInfoTag::SetReplayGainAlbumGain(int albumGain)
{
  m_iAlbumGain = albumGain;
  m_iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
}

void CMusicInfoTag::SetReplayGainTrackPeak(float trackPeak)
{
  m_fTrackPeak = trackPeak;
  m_iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
}

void CMusicInfoTag::SetReplayGainAlbumPeak(float albumPeak)
{
  m_fAlbumPeak = albumPeak;
  m_iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
}

void CMusicInfoTag::SetAlbum(const CAlbum& album)
{
  SetArtist(album.artist);
  SetAlbumId(album.idAlbum);
  SetAlbum(album.strAlbum);
  SetTitle(album.strAlbum);
  SetAlbumArtist(album.artist);
  SetGenre(album.genre);
  SetRating('0' + album.iRating);
  SetCompilation(album.bCompilation);
  SYSTEMTIME stTime;
  stTime.wYear = album.iYear;
  SetReleaseDate(stTime);
  m_iTimesPlayed = album.iTimesPlayed;
  m_iDbId = album.idAlbum;
  m_type = "album";
  m_bLoaded = true;
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
  m_type = "song";
  m_bLoaded = true;
  m_iTimesPlayed = song.iTimesPlayed;
  m_iAlbumId = song.iAlbumId;
}

void CMusicInfoTag::Serialize(CVariant& value) const
{
  value["url"] = m_strURL;
  value["title"] = m_strTitle;
  if (m_type.compare("artist") == 0 && m_artist.size() == 1)
    value["artist"] = m_artist[0];
  else
    value["artist"] = m_artist;
  value["album"] = m_strAlbum;
  value["albumartist"] = m_albumArtist;
  value["genre"] = m_genre;
  value["duration"] = m_iDuration;
  value["track"] = GetTrackNumber();
  value["disc"] = GetDiscNumber();
  value["loaded"] = m_bLoaded;
  value["year"] = m_dwReleaseDate.wYear;
  value["musicbrainztrackid"] = m_strMusicBrainzTrackID;
  value["musicbrainzartistid"] = m_strMusicBrainzArtistID;
  value["musicbrainzalbumid"] = m_strMusicBrainzAlbumID;
  value["musicbrainzalbumartistid"] = m_strMusicBrainzAlbumArtistID;
  value["musicbrainztrmid"] = m_strMusicBrainzTRMID;
  value["comment"] = m_strComment;
  value["rating"] = (int)(m_rating - '0');
  value["playcount"] = m_iTimesPlayed;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::EmptyString;
  value["lyrics"] = m_strLyrics;
  value["albumid"] = m_iAlbumId;
}

void CMusicInfoTag::ToSortable(SortItem& sortable)
{
  sortable[FieldTitle] = m_strTitle;
  sortable[FieldArtist] = m_artist;
  sortable[FieldAlbum] = m_strAlbum;
  sortable[FieldAlbumArtist] = FieldAlbumArtist;
  sortable[FieldGenre] = m_genre;
  sortable[FieldTime] = m_iDuration;
  sortable[FieldTrackNumber] = m_iTrack;
  sortable[FieldYear] = m_dwReleaseDate.wYear;
  sortable[FieldComment] = m_strComment;
  sortable[FieldRating] = (float)(m_rating - '0');
  sortable[FieldPlaycount] = m_iTimesPlayed;
  sortable[FieldLastPlayed] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::EmptyString;
  sortable[FieldListeners] = m_listeners;
  sortable[FieldId] = (int64_t)m_iDbId;
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
  m_bCompilation = false;
  m_strComment.Empty();
  m_rating = '0';
  m_iDbId = -1;
  m_type.clear();
  m_iTimesPlayed = 0;
  memset(&m_dwReleaseDate, 0, sizeof(m_dwReleaseDate) );
  m_iAlbumId = -1;
  m_coverArt.clear();
  m_iTrackGain = 0;
  m_iAlbumGain = 0;
  m_fTrackPeak = 0.0f;
  m_fAlbumPeak = 0.0f;
  m_iHasGainInfo = 0;
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

  m_albumArtist.push_back(albumArtist);
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
