/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <algorithm>

#include "MusicInfoTag.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "utils/Variant.h"
#include "utils/Archive.h"

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

void EmbeddedArtInfo::Archive(CArchive &ar)
{
  if (ar.IsStoring())
  {
    ar << size;
    ar << mime;
  }
  else
  {
    ar >> size;
    ar >> mime;
  }
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
  m_strArtistDesc = tag.m_strArtistDesc;
  m_albumArtist = tag.m_albumArtist;
  m_strAlbumArtistDesc = tag.m_strAlbumArtistDesc;
  m_strAlbum = tag.m_strAlbum;
  m_genre = tag.m_genre;
  m_strTitle = tag.m_strTitle;
  m_strMusicBrainzTrackID = tag.m_strMusicBrainzTrackID;
  m_musicBrainzArtistID = tag.m_musicBrainzArtistID;
  m_musicBrainzArtistHints = tag.m_musicBrainzArtistHints;
  m_strMusicBrainzAlbumID = tag.m_strMusicBrainzAlbumID;
  m_musicBrainzAlbumArtistID = tag.m_musicBrainzAlbumArtistID;
  m_musicBrainzAlbumArtistHints = tag.m_musicBrainzAlbumArtistHints;
  m_musicRoles = tag.m_musicRoles;
  m_strComment = tag.m_strComment;
  m_strMood = tag.m_strMood;
  m_strRecordLabel = tag.m_strRecordLabel;
  m_strLyrics = tag.m_strLyrics;
  m_cuesheet = tag.m_cuesheet;
  m_lastPlayed = tag.m_lastPlayed;
  m_dateAdded = tag.m_dateAdded;
  m_bCompilation = tag.m_bCompilation;
  m_iDuration = tag.m_iDuration;
  m_iTrack = tag.m_iTrack;
  m_bLoaded = tag.m_bLoaded;
  m_Rating = tag.m_Rating;
  m_Userrating = tag.m_Userrating;
  m_Votes = tag.m_Votes;
  m_listeners = tag.m_listeners;
  m_iTimesPlayed = tag.m_iTimesPlayed;
  m_iDbId = tag.m_iDbId;
  m_type = tag.m_type;
  m_iAlbumId = tag.m_iAlbumId;
  m_replayGain = tag.m_replayGain;
  m_albumReleaseType = tag.m_albumReleaseType;

  memcpy(&m_dwReleaseDate, &tag.m_dwReleaseDate, sizeof(m_dwReleaseDate));
  m_coverArt = tag.m_coverArt;
  return *this;
}

bool CMusicInfoTag::operator !=(const CMusicInfoTag& tag) const
{
  if (this == &tag) return false;
  if (m_strURL != tag.m_strURL) return true;
  if (m_strTitle != tag.m_strTitle) return true;
  if (m_bCompilation != tag.m_bCompilation) return true;
  if (m_artist != tag.m_artist) return true;
  if (m_albumArtist != tag.m_albumArtist) return true;
  if (m_strAlbum != tag.m_strAlbum) return true;
  if (m_iDuration != tag.m_iDuration) return true;
  if (m_iTrack != tag.m_iTrack) return true;
  if (m_albumReleaseType != tag.m_albumReleaseType) return true;
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

int CMusicInfoTag::GetTrackAndDiscNumber() const
{
  return m_iTrack;
}

int CMusicInfoTag::GetDuration() const
{
  return m_iDuration;
}

const std::string& CMusicInfoTag::GetTitle() const
{
  return m_strTitle;
}

const std::string& CMusicInfoTag::GetURL() const
{
  return m_strURL;
}

const std::vector<std::string>& CMusicInfoTag::GetArtist() const
{
  return m_artist;
}

const std::string CMusicInfoTag::GetArtistString() const
{
  if (!m_strArtistDesc.empty())
    return m_strArtistDesc;
  else if (!m_artist.empty())
    return StringUtils::Join(m_artist, g_advancedSettings.m_musicItemSeparator);
  else
    return StringUtils::Empty;
}

const std::string& CMusicInfoTag::GetAlbum() const
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

const std::string CMusicInfoTag::GetAlbumArtistString() const
{
  if (!m_strAlbumArtistDesc.empty())
    return m_strAlbumArtistDesc;
  if (!m_albumArtist.empty())
    return StringUtils::Join(m_albumArtist, g_advancedSettings.m_musicItemSeparator);
  else
    return StringUtils::Empty;
}


const std::vector<std::string>& CMusicInfoTag::GetGenre() const
{
  return m_genre;
}

void CMusicInfoTag::GetReleaseDate(SYSTEMTIME& dateTime) const
{
  memcpy(&dateTime, &m_dwReleaseDate, sizeof(m_dwReleaseDate));
}

int CMusicInfoTag::GetYear() const
{
  return m_dwReleaseDate.wYear;
}

int CMusicInfoTag::GetDatabaseId() const
{
  return m_iDbId;
}

const std::string &CMusicInfoTag::GetType() const
{
  return m_type;
}

std::string CMusicInfoTag::GetYearString() const
{
  return m_dwReleaseDate.wYear ? StringUtils::Format("%i", m_dwReleaseDate.wYear) : StringUtils::Empty;
}

const std::string &CMusicInfoTag::GetComment() const
{
  return m_strComment;
}

const std::string &CMusicInfoTag::GetMood() const
{
  return m_strMood;
}

const std::string &CMusicInfoTag::GetRecordLabel() const
{
  return m_strRecordLabel;
}

const std::string &CMusicInfoTag::GetLyrics() const
{
  return m_strLyrics;
}

const std::string &CMusicInfoTag::GetCueSheet() const
{
  return m_cuesheet;
}

float CMusicInfoTag::GetRating() const
{
  return m_Rating;
}

int CMusicInfoTag::GetUserrating() const
{
  return m_Userrating;
}

int CMusicInfoTag::GetVotes() const
{
  return m_Votes;
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

const CDateTime &CMusicInfoTag::GetDateAdded() const
{
  return m_dateAdded;
}

bool CMusicInfoTag::GetCompilation() const
{
  return m_bCompilation;
}

const EmbeddedArtInfo &CMusicInfoTag::GetCoverArtInfo() const
{
  return m_coverArt;
}

const ReplayGain& CMusicInfoTag::GetReplayGain() const
{
  return m_replayGain;
}

CAlbum::ReleaseType CMusicInfoTag::GetAlbumReleaseType() const
{
  return m_albumReleaseType;
}

void CMusicInfoTag::SetURL(const std::string& strURL)
{
  m_strURL = strURL;
}

void CMusicInfoTag::SetTitle(const std::string& strTitle)
{
  m_strTitle = Trim(strTitle);
}

void CMusicInfoTag::SetArtist(const std::string& strArtist)
{
  if (!strArtist.empty())
  {
    SetArtistDesc(strArtist);
    SetArtist(StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator));
  }
  else
  {
    m_strArtistDesc.clear();
    m_artist.clear();
  }
}

void CMusicInfoTag::SetArtist(const std::vector<std::string>& artists, bool FillDesc /* = false*/)
{
  m_artist = artists;
  if (m_strArtistDesc.empty() || FillDesc) 
  { 
    SetArtistDesc(StringUtils::Join(artists, g_advancedSettings.m_musicItemSeparator));
  }
}

void CMusicInfoTag::SetArtistDesc(const std::string& strArtistDesc)
{
  m_strArtistDesc = strArtistDesc;
}

void CMusicInfoTag::SetAlbum(const std::string& strAlbum)
{
  m_strAlbum = Trim(strAlbum);
}

void CMusicInfoTag::SetAlbumId(const int iAlbumId)
{
  m_iAlbumId = iAlbumId;
}

void CMusicInfoTag::SetAlbumArtist(const std::string& strAlbumArtist)
{
  if (!strAlbumArtist.empty())
  {
    SetAlbumArtistDesc(strAlbumArtist);
    SetAlbumArtist(StringUtils::Split(strAlbumArtist, g_advancedSettings.m_musicItemSeparator));
  }
  else
  {
    m_strAlbumArtistDesc.clear();
    m_albumArtist.clear();
  }
}

void CMusicInfoTag::SetAlbumArtist(const std::vector<std::string>& albumArtists, bool FillDesc /* = false*/)
{
  m_albumArtist = albumArtists;
  if (m_strAlbumArtistDesc.empty() || FillDesc) 
    SetAlbumArtistDesc(StringUtils::Join(albumArtists, g_advancedSettings.m_musicItemSeparator));
}

void CMusicInfoTag::SetAlbumArtistDesc(const std::string& strAlbumArtistDesc)
{
  m_strAlbumArtistDesc = strAlbumArtistDesc;
}

void CMusicInfoTag::SetGenre(const std::string& strGenre)
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

void CMusicInfoTag::SetDiscNumber(int iDiscNumber)
{
  m_iTrack = (m_iTrack & 0xffff) | (iDiscNumber << 16);
}

void CMusicInfoTag::SetTrackAndDiscNumber(int iTrackAndDisc)
{
  m_iTrack = iTrackAndDisc;
}

void CMusicInfoTag::SetDuration(int iSec)
{
  m_iDuration = iSec;
}

void CMusicInfoTag::SetComment(const std::string& comment)
{
  m_strComment = comment;
}

void CMusicInfoTag::SetMood(const std::string& mood)
{
  m_strMood = mood;
}

void CMusicInfoTag::SetRecordLabel(const std::string& publisher)
{
  m_strRecordLabel = publisher;
}

void CMusicInfoTag::SetCueSheet(const std::string& cueSheet)
{
  m_cuesheet = cueSheet;
}

void CMusicInfoTag::SetLyrics(const std::string& lyrics)
{
  m_strLyrics = lyrics;
}

void CMusicInfoTag::SetRating(float rating)
{
  //This value needs to be between 0-10 - 0 will unset the rating
  rating = std::max(rating, 0.f);
  rating = std::min(rating, 10.f);

  m_Rating = rating;
}

void CMusicInfoTag::SetVotes(int votes)
{
  m_Votes = votes;
}

void CMusicInfoTag::SetUserrating(int rating)
{
  //This value needs to be between 0-10 - 0 will unset the userrating
  rating = std::max(rating, 0);
  rating = std::min(rating, 10);

  m_Userrating = rating;
}

void CMusicInfoTag::SetListeners(int listeners)
{
  m_listeners = std::max(listeners, 0);
}

void CMusicInfoTag::SetPlayCount(int playcount)
{
  m_iTimesPlayed = playcount;
}

void CMusicInfoTag::SetLastPlayed(const std::string& lastplayed)
{
  m_lastPlayed.SetFromDBDateTime(lastplayed);
}

void CMusicInfoTag::SetLastPlayed(const CDateTime& lastplayed)
{
  m_lastPlayed = lastplayed;
}

void CMusicInfoTag::SetDateAdded(const std::string& dateAdded)
{
  m_dateAdded.SetFromDBDateTime(dateAdded);
}

void CMusicInfoTag::SetDateAdded(const CDateTime& dateAdded)
{
  m_dateAdded = dateAdded;
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

const std::string& CMusicInfoTag::GetMusicBrainzTrackID() const
{
  return m_strMusicBrainzTrackID;
}

const std::vector<std::string>& CMusicInfoTag::GetMusicBrainzArtistID() const
{
  return m_musicBrainzArtistID;
}

const std::vector<std::string>& CMusicInfoTag::GetMusicBrainzArtistHints() const
{
  return m_musicBrainzArtistHints;
}

const std::string& CMusicInfoTag::GetMusicBrainzAlbumID() const
{
  return m_strMusicBrainzAlbumID;
}

const std::vector<std::string>& CMusicInfoTag::GetMusicBrainzAlbumArtistID() const
{
  return m_musicBrainzAlbumArtistID;
}

const std::vector<std::string>& CMusicInfoTag::GetMusicBrainzAlbumArtistHints() const
{
    return m_musicBrainzAlbumArtistHints;
}

const std::string &CMusicInfoTag::GetMusicBrainzReleaseType() const
{
  return m_strMusicBrainzReleaseType;
}

void CMusicInfoTag::SetMusicBrainzTrackID(const std::string& strTrackID)
{
  m_strMusicBrainzTrackID = strTrackID;
}

void CMusicInfoTag::SetMusicBrainzArtistID(const std::vector<std::string>& musicBrainzArtistId)
{
  m_musicBrainzArtistID = musicBrainzArtistId;
}

void CMusicInfoTag::SetMusicBrainzArtistHints(const std::vector<std::string>& musicBrainzArtistHints)
{
  m_musicBrainzArtistHints = musicBrainzArtistHints;
}

void CMusicInfoTag::SetMusicBrainzAlbumID(const std::string& strAlbumID)
{
  m_strMusicBrainzAlbumID = strAlbumID;
}

void CMusicInfoTag::SetMusicBrainzAlbumArtistID(const std::vector<std::string>& musicBrainzAlbumArtistId)
{
  m_musicBrainzAlbumArtistID = musicBrainzAlbumArtistId;
}

void CMusicInfoTag::SetMusicBrainzAlbumArtistHints(const std::vector<std::string>& musicBrainzAlbumArtistHints)
{
    m_musicBrainzAlbumArtistHints = musicBrainzAlbumArtistHints;
}

void CMusicInfoTag::SetMusicBrainzReleaseType(const std::string& ReleaseType)
{
  m_strMusicBrainzReleaseType = ReleaseType;
}

void CMusicInfoTag::SetCoverArtInfo(size_t size, const std::string &mimeType)
{
  m_coverArt.set(size, mimeType);
}

void CMusicInfoTag::SetReplayGain(const ReplayGain& aGain)
{
  m_replayGain = aGain;
}

void CMusicInfoTag::SetAlbumReleaseType(CAlbum::ReleaseType releaseType)
{
  m_albumReleaseType = releaseType;
}

void CMusicInfoTag::SetType(const MediaType mediaType)
{
  m_type = mediaType;
}

void CMusicInfoTag::SetArtist(const CArtist& artist)
{
  SetArtist(artist.strArtist);
  SetAlbumArtist(artist.strArtist);
  SetMusicBrainzArtistID({ artist.strMusicBrainzArtistID });
  SetMusicBrainzAlbumArtistID({ artist.strMusicBrainzArtistID });
  SetGenre(artist.genre);
  SetMood(StringUtils::Join(artist.moods, g_advancedSettings.m_musicItemSeparator));
  SetDateAdded(artist.dateAdded);
  SetDatabaseId(artist.idArtist, MediaTypeArtist);

  SetLoaded();
}

void CMusicInfoTag::SetAlbum(const CAlbum& album)
{
  Clear();
  //Set all artist infomation from album artist credits and artist description
  SetArtistDesc(album.GetAlbumArtistString());
  SetArtist(album.GetAlbumArtist());
  SetMusicBrainzArtistID(album.GetMusicBrainzAlbumArtistID());
  SetAlbumArtistDesc(album.GetAlbumArtistString());
  SetAlbumArtist(album.GetAlbumArtist());
  SetMusicBrainzAlbumArtistID(album.GetMusicBrainzAlbumArtistID());
  SetAlbumId(album.idAlbum);
  SetAlbum(album.strAlbum);
  SetTitle(album.strAlbum);
  SetMusicBrainzAlbumID(album.strMusicBrainzAlbumID);
  SetMusicBrainzReleaseType(album.strType);
  SetGenre(album.genre);
  SetMood(StringUtils::Join(album.moods, g_advancedSettings.m_musicItemSeparator));
  SetRecordLabel(album.strLabel);
  SetRating(album.fRating);
  SetUserrating(album.iUserrating);
  SetVotes(album.iVotes);
  SetCompilation(album.bCompilation);
  SYSTEMTIME stTime;
  stTime.wYear = album.iYear;
  SetReleaseDate(stTime);
  SetAlbumReleaseType(album.releaseType);
  SetDateAdded(album.dateAdded);
  SetPlayCount(album.iTimesPlayed);
  SetDatabaseId(album.idAlbum, MediaTypeAlbum);
  SetLastPlayed(album.lastPlayed);

  SetLoaded();
}

void CMusicInfoTag::SetSong(const CSong& song)
{
  Clear();
  SetTitle(song.strTitle);
  SetGenre(song.genre);
  /* Set all artist infomation from song artist credits and artist description.
     During processing e.g. Cue Sheets, song may only have artist description string 
     rather than a fully populated artist credits vector.
  */
  if (!song.HasArtistCredits())
    SetArtist(song.GetArtistString()); //Sets both artist description string and artist vector from string
  else
  {
    SetArtistDesc(song.GetArtistString());
    SetArtist(song.GetArtist());
    SetMusicBrainzArtistID(song.GetMusicBrainzArtistID());
  }
  SetAlbum(song.strAlbum);
  SetAlbumArtist(song.GetAlbumArtist()); //Only have album artist in song as vector, no desc or MBID
  SetMusicBrainzTrackID(song.strMusicBrainzTrackID);
  SetContributors(song.GetContributors());
  SetComment(song.strComment);
  SetCueSheet(song.strCueSheet);
  SetPlayCount(song.iTimesPlayed);
  SetLastPlayed(song.lastPlayed);
  SetDateAdded(song.dateAdded);
  SetCoverArtInfo(song.embeddedArt.size, song.embeddedArt.mime);
  SetRating(song.rating);
  SetUserrating(song.userrating);
  SetVotes(song.votes);
  SetURL(song.strFileName);
  SYSTEMTIME stTime;
  stTime.wYear = song.iYear;
  SetReleaseDate(stTime);
  SetTrackAndDiscNumber(song.iTrack);
  SetDuration(song.iDuration);
  SetMood(song.strMood);
  SetCompilation(song.bCompilation);
  SetAlbumId(song.idAlbum);
  SetDatabaseId(song.idSong, MediaTypeSong);

  if (song.replayGain.Get(ReplayGain::TRACK).Valid())
    m_replayGain.Set(ReplayGain::TRACK, song.replayGain.Get(ReplayGain::TRACK));
  if (song.replayGain.Get(ReplayGain::ALBUM).Valid())
    m_replayGain.Set(ReplayGain::ALBUM, song.replayGain.Get(ReplayGain::ALBUM));

  SetLoaded();
}

void CMusicInfoTag::Serialize(CVariant& value) const
{
  value["url"] = m_strURL;
  value["title"] = m_strTitle;
  if (m_type.compare(MediaTypeArtist) == 0 && m_artist.size() == 1)
    value["artist"] = m_artist[0];
  else
    value["artist"] = m_artist;
  // There are situations where the individual artist(s) are not queried from the song_artist and artist tables e.g. playlist,
  // only artist description from song table. Since processing of the ARTISTS tag was added the individual artists may not always
  // be accurately derrived by simply splitting the artist desc. Hence m_artist is only populated when the individual artists are
  // queried, whereas GetArtistString() will always return the artist description.
  // To avoid empty artist array in JSON, when m_artist is empty then an attempt is made to split the artist desc into artists.
  // A longer term soltion would be to ensure that when individual artists are to be returned then the song_artist and artist tables
  // are queried.
  if (m_artist.empty())
    value["artist"] = StringUtils::Split(GetArtistString(), g_advancedSettings.m_musicItemSeparator);

  value["displayartist"] = GetArtistString();
  value["displayalbumartist"] = GetAlbumArtistString();
  value["album"] = m_strAlbum;
  value["albumartist"] = m_albumArtist;
  value["genre"] = m_genre;
  value["duration"] = m_iDuration;
  value["track"] = GetTrackNumber();
  value["disc"] = GetDiscNumber();
  value["loaded"] = m_bLoaded;
  value["year"] = m_dwReleaseDate.wYear;
  value["musicbrainztrackid"] = m_strMusicBrainzTrackID;
  value["musicbrainzartistid"] = m_musicBrainzArtistID;
  value["musicbrainzalbumid"] = m_strMusicBrainzAlbumID;
  value["musicbrainzalbumartistid"] = m_musicBrainzAlbumArtistID; 
  value["comment"] = m_strComment;
  value["contributors"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& role : m_musicRoles)
  {
    CVariant contributor;
    contributor["name"] = role.GetArtist();
    contributor["role"] = role.GetRoleDesc();
    contributor["roleid"] = (int)(role.GetRoleId());
    contributor["artistid"] = (int)(role.GetArtistId());
    value["contributors"].push_back(contributor);
  }
  value["displaycomposer"] = GetArtistStringForRole("composer");   //TCOM
  value["displayconductor"] = GetArtistStringForRole("conductor"); //TPE3
  value["displayorchestra"] = GetArtistStringForRole("orchestra");
  value["displaylyricist"] = GetArtistStringForRole("lyricist");   //TEXT
  value["mood"] = StringUtils::Split(m_strMood, g_advancedSettings.m_musicItemSeparator);
  value["recordlabel"] = m_strRecordLabel;
  value["rating"] = m_Rating;
  value["userrating"] = m_Userrating;
  value["votes"] = m_Votes;
  value["playcount"] = m_iTimesPlayed;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::Empty;
  value["dateadded"] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty;
  value["lyrics"] = m_strLyrics;
  value["albumid"] = m_iAlbumId;
  value["compilationartist"] = m_bCompilation;
  value["compilation"] = m_bCompilation;
  if (m_type.compare(MediaTypeAlbum) == 0)
    value["releasetype"] = CAlbum::ReleaseTypeToString(m_albumReleaseType);
  else if (m_type.compare(MediaTypeSong) == 0)
    value["albumreleasetype"] = CAlbum::ReleaseTypeToString(m_albumReleaseType);
}

void CMusicInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  switch (field)
  {
  case FieldTitle:
  {
    // make sure not to overwrite an existing path with an empty one
    std::string title = m_strTitle;
    if (!title.empty() || sortable.find(FieldTitle) == sortable.end())
      sortable[FieldTitle] = title;
    break;
  }
  case FieldArtist:      sortable[FieldArtist] = m_strArtistDesc; break;
  case FieldAlbum:       sortable[FieldAlbum] = m_strAlbum; break;
  case FieldAlbumArtist: sortable[FieldAlbumArtist] = m_strAlbumArtistDesc; break;
  case FieldGenre:       sortable[FieldGenre] = m_genre; break;
  case FieldTime:        sortable[FieldTime] = m_iDuration; break;
  case FieldTrackNumber: sortable[FieldTrackNumber] = m_iTrack; break;
  case FieldYear:        sortable[FieldYear] = m_dwReleaseDate.wYear; break;
  case FieldComment:     sortable[FieldComment] = m_strComment; break;
  case FieldMoods:       sortable[FieldMoods] = m_strMood; break;
  case FieldRating:      sortable[FieldRating] = m_Rating; break;
  case FieldUserRating:  sortable[FieldUserRating] = m_Userrating; break;
  case FieldVotes:       sortable[FieldVotes] = m_Votes; break;
  case FieldPlaycount:   sortable[FieldPlaycount] = m_iTimesPlayed; break;
  case FieldLastPlayed:  sortable[FieldLastPlayed] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::Empty; break;
  case FieldDateAdded:   sortable[FieldDateAdded] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty; break;
  case FieldListeners:   sortable[FieldListeners] = m_listeners; break;
  case FieldId:          sortable[FieldId] = (int64_t)m_iDbId; break;
  default: break;
  }
}

void CMusicInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strURL;
    ar << m_strTitle;
    ar << m_artist;
    ar << m_strArtistDesc;
    ar << m_strAlbum;
    ar << m_albumArtist;
    ar << m_strAlbumArtistDesc;
    ar << m_genre;
    ar << m_iDuration;
    ar << m_iTrack;
    ar << m_bLoaded;
    ar << m_dwReleaseDate;
    ar << m_strMusicBrainzTrackID;
    ar << m_musicBrainzArtistID;
    ar << m_strMusicBrainzAlbumID;
    ar << m_musicBrainzAlbumArtistID;
    ar << m_strMusicBrainzReleaseType;
    ar << m_lastPlayed;
    ar << m_dateAdded;
    ar << m_strComment;
    ar << (int)m_musicRoles.size();   
    for (VECMUSICROLES::const_iterator credit = m_musicRoles.begin(); credit != m_musicRoles.end(); ++credit)
    {
      ar << credit->GetRoleId();
      ar << credit->GetRoleDesc();
      ar << credit->GetArtist();
      ar << credit->GetArtistId();
    }
    ar << m_strMood;
    ar << m_strRecordLabel;
    ar << m_Rating;
    ar << m_Userrating;
    ar << m_Votes;
    ar << m_iTimesPlayed;
    ar << m_iAlbumId;
    ar << m_iDbId;
    ar << m_type;
    ar << m_strLyrics;
    ar << m_bCompilation;
    ar << m_listeners;
    ar << m_coverArt;
    ar << m_cuesheet;
    ar << static_cast<int>(m_albumReleaseType);
  }
  else
  {
    ar >> m_strURL;
    ar >> m_strTitle;
    ar >> m_artist;
    ar >> m_strArtistDesc;
    ar >> m_strAlbum;
    ar >> m_albumArtist;
    ar >> m_strAlbumArtistDesc;
    ar >> m_genre;
    ar >> m_iDuration;
    ar >> m_iTrack;
    ar >> m_bLoaded;
    ar >> m_dwReleaseDate;
    ar >> m_strMusicBrainzTrackID;
    ar >> m_musicBrainzArtistID;
    ar >> m_strMusicBrainzAlbumID;
    ar >> m_musicBrainzAlbumArtistID;
    ar >> m_strMusicBrainzReleaseType;
    ar >> m_lastPlayed;
    ar >> m_dateAdded;
    ar >> m_strComment;
    int iMusicRolesSize;
    ar >> iMusicRolesSize;
    m_musicRoles.reserve(iMusicRolesSize);
    for (int i = 0; i < iMusicRolesSize; ++i)
    {
      int idRole;
      long idArtist;
      std::string strArtist;
      std::string strRole;
      ar >> idRole;
      ar >> strRole;
      ar >> strArtist;
      ar >> idArtist;
      m_musicRoles.emplace_back(idRole, strRole, strArtist, idArtist);
    }
    ar >> m_strMood;
    ar >> m_strRecordLabel;
    ar >> m_Rating;
    ar >> m_Userrating;
    ar >> m_Votes;
    ar >> m_iTimesPlayed;
    ar >> m_iAlbumId;
    ar >> m_iDbId;
    ar >> m_type;
    ar >> m_strLyrics;
    ar >> m_bCompilation;
    ar >> m_listeners;
    ar >> m_coverArt;
    ar >> m_cuesheet;

    int albumReleaseType;
    ar >> albumReleaseType;
    m_albumReleaseType = static_cast<CAlbum::ReleaseType>(albumReleaseType);
  }
}

void CMusicInfoTag::Clear()
{
  m_strURL.clear();
  m_artist.clear();
  m_strAlbum.clear();
  m_albumArtist.clear();
  m_genre.clear();
  m_strTitle.clear();
  m_strMusicBrainzTrackID.clear();
  m_musicBrainzArtistID.clear();
  m_strMusicBrainzAlbumID.clear();
  m_musicBrainzAlbumArtistID.clear();
  m_strMusicBrainzReleaseType.clear();
  m_musicRoles.clear();
  m_iDuration = 0;
  m_iTrack = 0;
  m_bLoaded = false;
  m_lastPlayed.Reset();
  m_dateAdded.Reset();
  m_bCompilation = false;
  m_strComment.clear();
  m_strMood.clear();
  m_strRecordLabel.clear();
  m_cuesheet.clear();
  m_iDbId = -1;
  m_type.clear();
  m_iTimesPlayed = 0;
  memset(&m_dwReleaseDate, 0, sizeof(m_dwReleaseDate));
  m_iAlbumId = -1;
  m_coverArt.clear();
  m_replayGain = ReplayGain();
  m_albumReleaseType = CAlbum::Album;
  m_listeners = 0;
  m_Rating = 0;
  m_Userrating = 0;
  m_Votes = 0;
}

void CMusicInfoTag::AppendArtist(const std::string &artist)
{
  for (unsigned int index = 0; index < m_artist.size(); index++)
  {
    if (StringUtils::EqualsNoCase(artist, m_artist.at(index)))
      return;
  }

  m_artist.push_back(artist);
}

void CMusicInfoTag::AppendAlbumArtist(const std::string &albumArtist)
{
  for (unsigned int index = 0; index < m_albumArtist.size(); index++)
  {
    if (StringUtils::EqualsNoCase(albumArtist, m_albumArtist.at(index)))
      return;
  }

  m_albumArtist.push_back(albumArtist);
}

void CMusicInfoTag::AppendGenre(const std::string &genre)
{
  for (unsigned int index = 0; index < m_genre.size(); index++)
  {
    if (StringUtils::EqualsNoCase(genre, m_genre.at(index)))
      return;
  }

  m_genre.push_back(genre);
}

void CMusicInfoTag::AddArtistRole(const std::string& Role, const std::string& strArtist)
{
  if (!strArtist.empty() && !Role.empty())
    AddArtistRole(Role, StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator));
}

void CMusicInfoTag::AddArtistRole(const std::string& Role, const std::vector<std::string>& artists)
{
  for (unsigned int index = 0; index < artists.size(); index++)
  {
    CMusicRole ArtistCredit(Role, Trim(artists.at(index)));
    //Prevent duplicate entries
    VECMUSICROLES::iterator credit = find(m_musicRoles.begin(), m_musicRoles.end(), ArtistCredit);
    if (credit == m_musicRoles.end())
      m_musicRoles.push_back(ArtistCredit);
  }
}

void CMusicInfoTag::AppendArtistRole(const CMusicRole& ArtistRole)
{
  //Append contributor, no check for duplicates as from database
  m_musicRoles.push_back(ArtistRole);
}

const std::string CMusicInfoTag::GetArtistStringForRole(const std::string& strRole) const
{
  std::vector<std::string> artistvector;
  for (VECMUSICROLES::const_iterator credit = m_musicRoles.begin(); credit != m_musicRoles.end(); ++credit) 
  {
    if (StringUtils::EqualsNoCase(credit->GetRoleDesc(), strRole))
      artistvector.push_back(credit->GetArtist());
  }
  return StringUtils::Join(artistvector, g_advancedSettings.m_musicItemSeparator);
}

const std::string CMusicInfoTag::GetContributorsText() const
{
  std::string strLabel;
  for (VECMUSICROLES::const_iterator credit = m_musicRoles.begin(); credit != m_musicRoles.end(); ++credit)
  {
    strLabel += StringUtils::Format("%s\n", credit->GetArtist().c_str());
  }
  return StringUtils::TrimRight(strLabel, "\n");
}

const std::string CMusicInfoTag::GetContributorsAndRolesText() const
{
  std::string strLabel;
  for (VECMUSICROLES::const_iterator credit = m_musicRoles.begin(); credit != m_musicRoles.end(); ++credit)
  {
    strLabel += StringUtils::Format("%s - %s\n", credit->GetRoleDesc().c_str(), credit->GetArtist().c_str());
  }
  return StringUtils::TrimRight(strLabel, "\n");
}


const VECMUSICROLES &CMusicInfoTag::GetContributors()  const
{
  return m_musicRoles;
}

void CMusicInfoTag::SetContributors(const VECMUSICROLES& contributors)
{
  m_musicRoles = contributors;
}

std::string CMusicInfoTag::Trim(const std::string &value) const
{
  std::string trimmedValue(value);
  StringUtils::TrimLeft(trimmedValue, " ");
  StringUtils::TrimRight(trimmedValue, " \n\r");
  return trimmedValue;
}
