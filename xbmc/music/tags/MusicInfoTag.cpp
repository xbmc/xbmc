/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTag.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "music/Artist.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Archive.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>

using namespace MUSIC_INFO;

CMusicInfoTag::CMusicInfoTag(void)
{
  Clear();
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
  if (m_strDiscSubtitle != tag.m_strDiscSubtitle)
    return true;
  if (m_iTrack != tag.m_iTrack)
    return true;
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
    return StringUtils::Join(m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  else
    return StringUtils::Empty;
}

const std::string& CMusicInfoTag::GetArtistSort() const
{
  return m_strArtistSort;
}

const std::string& CMusicInfoTag::GetComposerSort() const
{
  return m_strComposerSort;
}

const std::string& CMusicInfoTag::GetAlbum() const
{
  return m_strAlbum;
}

const std::string& CMusicInfoTag::GetDiscSubtitle() const
{
  return m_strDiscSubtitle;
}

const std::string& CMusicInfoTag::GetOriginalDate() const
{
  return m_strOriginalDate;
}

const std::string MUSIC_INFO::CMusicInfoTag::GetOriginalYear() const
{
  return StringUtils::Left(m_strOriginalDate, 4);
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
    return StringUtils::Join(m_albumArtist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  else
    return StringUtils::Empty;
}

const std::string& CMusicInfoTag::GetAlbumArtistSort() const
{
  return m_strAlbumArtistSort;
}

const std::vector<std::string>& CMusicInfoTag::GetGenre() const
{
  return m_genre;
}

int CMusicInfoTag::GetDatabaseId() const
{
  return m_iDbId;
}

const std::string &CMusicInfoTag::GetType() const
{
  return m_type;
}

int CMusicInfoTag::GetYear() const
{
  return atoi(GetYearString().c_str());
}

std::string CMusicInfoTag::GetYearString() const
{
  /* Get year as YYYY from release or original dates depending on setting
     This is how GUI and by year sorting swiches to using original year.
     For ripper and non-library items (library entries have both values):
     when release date missing try to fallback to original date
     when original date missing use release date
  */
  std::string value;
  value = GetReleaseYear();
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
    CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE) ||
    value.empty())
  {
    std::string origvalue = GetOriginalYear();
    if (!origvalue.empty())
      return origvalue;
  }
  return value;
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

bool CMusicInfoTag::GetBoxset() const
{
  return m_bBoxset;
}

int CMusicInfoTag::GetTotalDiscs() const
{
  return m_iDiscTotal;
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

int CMusicInfoTag::GetBPM() const
{
  return m_iBPM;
}

int CMusicInfoTag::GetBitRate() const
{
  return m_bitrate;
}

int CMusicInfoTag::GetSampleRate() const
{
  return m_samplerate;
}

int CMusicInfoTag::GetNoOfChannels() const
{
  return m_channels;
}

const std::string& CMusicInfoTag::GetReleaseDate() const
{
  return m_strReleaseDate;
}

const std::string MUSIC_INFO::CMusicInfoTag::GetReleaseYear() const
{
  return StringUtils::Left(m_strReleaseDate, 4);
}

// This is the Musicbrainz release status tag. See https://musicbrainz.org/doc/Release#Status

const std::string& CMusicInfoTag::GetAlbumReleaseStatus() const
{
  return m_strReleaseStatus;
}

const std::string& CMusicInfoTag::GetStationName() const
{
  return m_stationName;
}

const std::string& CMusicInfoTag::GetStationArt() const
{
  return m_stationArt;
}

const std::string& CMusicInfoTag::GetSongVideoURL() const
{
  return m_songVideoURL;
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
    SetArtist(StringUtils::Split(strArtist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
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
    SetArtistDesc(StringUtils::Join(artists, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  }
}

void CMusicInfoTag::SetArtistDesc(const std::string& strArtistDesc)
{
  m_strArtistDesc = strArtistDesc;
}

void CMusicInfoTag::SetArtistSort(const std::string& strArtistsort)
{
  m_strArtistSort = strArtistsort;
}

void CMusicInfoTag::SetComposerSort(const std::string& strComposerSort)
{
  m_strComposerSort = strComposerSort;
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
    SetAlbumArtist(StringUtils::Split(strAlbumArtist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
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
    SetAlbumArtistDesc(StringUtils::Join(albumArtists, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CMusicInfoTag::SetAlbumArtistDesc(const std::string& strAlbumArtistDesc)
{
  m_strAlbumArtistDesc = strAlbumArtistDesc;
}

void CMusicInfoTag::SetAlbumArtistSort(const std::string& strAlbumArtistSort)
{
  m_strAlbumArtistSort = strAlbumArtistSort;
}

void CMusicInfoTag::SetGenre(const std::string& strGenre, bool bTrim /* = false*/)
{
  if (!strGenre.empty())
    SetGenre(StringUtils::Split(strGenre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator), bTrim);
  else
    m_genre.clear();
}

void CMusicInfoTag::SetGenre(const std::vector<std::string>& genres, bool bTrim /* = false*/)
{
  m_genre = genres;
  if (bTrim)
    for (auto genre : m_genre)
      StringUtils::Trim(genre);
}

void CMusicInfoTag::SetYear(int year)
{
  // Parse integer year value into YYYY ISO8601 format (partial) date string
  // Add century for to 2 digit numbers, 41 -> 1941, 40 -> 2040
  if (year > 99)
    SetReleaseDate(StringUtils::Format("{:04}", year));
  else if (year > 40)
    SetReleaseDate(StringUtils::Format("{:04}", 19 + year));
  else  if (year > 0)
    SetReleaseDate(StringUtils::Format("{:04}", 20 + year));
  else
    m_strReleaseDate.clear();
}

void CMusicInfoTag::SetDatabaseId(int id, const std::string &type)
{
  m_iDbId = id;
  m_type = type;
}

void CMusicInfoTag::SetTrackNumber(int iTrack)
{
  m_iTrack = (m_iTrack & 0xffff0000) | (iTrack & 0xffff);
}

void CMusicInfoTag::SetDiscNumber(int iDiscNumber)
{
  m_iTrack = (m_iTrack & 0xffff) | (iDiscNumber << 16);
}

void CMusicInfoTag::SetDiscSubtitle(const std::string& strDiscSubtitle)
{
  m_strDiscSubtitle = strDiscSubtitle;
}

void CMusicInfoTag::SetTotalDiscs(int iDiscTotal)
{
  m_iDiscTotal = iDiscTotal;
}

void CMusicInfoTag::SetReleaseDate(const std::string& strReleaseDate)
{
  // Date in ISO8601 YYYY, YYYY-MM or YYYY-MM-DD
  m_strReleaseDate = strReleaseDate;
}

void CMusicInfoTag::SetOriginalDate(const std::string& strOriginalDate)
{
  // Date in ISO8601 YYYY, YYYY-MM or YYYY-MM-DD
  m_strOriginalDate = strOriginalDate;
}

void CMusicInfoTag::AddOriginalDate(const std::string& strDateYear)
{
  // Avoid overwriting YYYY-MM or YYYY-MM-DD (from DATE tag) with just YYYY (from YEAR tag)
  if (strDateYear.size() > m_strOriginalDate.size())
    m_strOriginalDate = strDateYear;
}

void CMusicInfoTag::AddReleaseDate(const std::string& strDateYear, bool isMonth /*= false*/)
{
  // Given MMDD  (from ID3 v2.3 TDAT tag) set MM-DD part of ISO8601 string
  if (isMonth && !strDateYear.empty())
  {
    std::string strYYYY = GetReleaseYear();
    if (strYYYY.empty())
      strYYYY = "0000"; // Fake year when TYER not read yet
    m_strReleaseDate = StringUtils::Format("{}-{}-{}", strYYYY, StringUtils::Left(strDateYear, 2),
                                           StringUtils::Right(strDateYear, 2));
  }
  // Given YYYY only (from YEAR tag) and already have YYYY-MM or YYYY-MM-DD (from DATE tag)
  else if (strDateYear.size() == 4 && (m_strReleaseDate.size() > 4))
  {
    // Have 0000-MM-DD where ID3 v2.3 TDAT tag read first, fill YYYY part from TYER
    if (GetReleaseYear() == "0000")
      StringUtils::Replace(m_strReleaseDate, "0000", strDateYear);
  }
  else
    m_strReleaseDate = strDateYear;  // Could be YYYY, YYYY-MM or YYYY-MM-DD
}

void CMusicInfoTag::SetTrackAndDiscNumber(int iTrackAndDisc)
{
  m_iTrack = iTrackAndDisc;
}

void CMusicInfoTag::SetDuration(int iSec)
{
  m_iDuration = iSec;
}

void CMusicInfoTag::SetBitRate(int bitrate)
{
  m_bitrate = bitrate;
}

void CMusicInfoTag::SetNoOfChannels(int channels)
{
  m_channels = channels;
}

void CMusicInfoTag::SetSampleRate(int samplerate)
{
  m_samplerate = samplerate;
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

void CMusicInfoTag::SetDateAdded(const std::string& strDateAdded)
{
  m_dateAdded.SetFromDBDateTime(strDateAdded);
}

void CMusicInfoTag::SetDateAdded(const CDateTime& dateAdded)
{
  m_dateAdded = dateAdded;
}

void MUSIC_INFO::CMusicInfoTag::SetDateUpdated(const std::string& strDateUpdated)
{
  m_dateUpdated.SetFromDBDateTime(strDateUpdated);
}

void MUSIC_INFO::CMusicInfoTag::SetDateUpdated(const CDateTime& dateUpdated)
{
  m_dateUpdated = dateUpdated;
}

void MUSIC_INFO::CMusicInfoTag::SetDateNew(const std::string& strDateNew)
{
  m_dateNew.SetFromDBDateTime(strDateNew);
}

void MUSIC_INFO::CMusicInfoTag::SetDateNew(const CDateTime& dateNew)
{
  m_dateNew = dateNew;
}

void CMusicInfoTag::SetCompilation(bool compilation)
{
  m_bCompilation = compilation;
}

void CMusicInfoTag::SetBoxset(bool boxset)
{
  m_bBoxset = boxset;
}

void CMusicInfoTag::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicInfoTag::Loaded() const
{
  return m_bLoaded;
}

void CMusicInfoTag::SetBPM(int bpm)
{
  m_iBPM = bpm;
}

void CMusicInfoTag::SetStationName(const std::string& strStationName)
{
  m_stationName = strStationName;
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

const std::string & MUSIC_INFO::CMusicInfoTag::GetMusicBrainzReleaseGroupID() const
{
  return m_strMusicBrainzReleaseGroupID;
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

void MUSIC_INFO::CMusicInfoTag::SetMusicBrainzReleaseGroupID(const std::string & strReleaseGroupID)
{
  m_strMusicBrainzReleaseGroupID = strReleaseGroupID;
}

void CMusicInfoTag::SetMusicBrainzReleaseType(const std::string& ReleaseType)
{
  m_strMusicBrainzReleaseType = ReleaseType;
}

void CMusicInfoTag::SetCoverArtInfo(size_t size, const std::string &mimeType)
{
  m_coverArt.Set(size, mimeType);
}

void CMusicInfoTag::SetReplayGain(const ReplayGain& aGain)
{
  m_replayGain = aGain;
}

void CMusicInfoTag::SetAlbumReleaseType(CAlbum::ReleaseType releaseType)
{
  m_albumReleaseType = releaseType;
}

void CMusicInfoTag::SetType(const MediaType& mediaType)
{
  m_type = mediaType;
}

// This is the Musicbrainz release status tag. See https://musicbrainz.org/doc/Release#Status

void CMusicInfoTag::SetAlbumReleaseStatus(const std::string& ReleaseStatus)
{
  m_strReleaseStatus = ReleaseStatus;
}

void CMusicInfoTag::SetStationArt(const std::string& strStationArt)
{
  m_stationArt = strStationArt;
}

void CMusicInfoTag::SetSongVideoURL(const std::string& songVideoURL)
{
  m_songVideoURL = songVideoURL;
}

void CMusicInfoTag::SetArtist(const CArtist& artist)
{
  SetArtist(artist.strArtist);
  SetArtistSort(artist.strSortName);
  SetAlbumArtist(artist.strArtist);
  SetAlbumArtistSort(artist.strSortName);
  SetMusicBrainzArtistID({ artist.strMusicBrainzArtistID });
  SetMusicBrainzAlbumArtistID({ artist.strMusicBrainzArtistID });
  SetGenre(artist.genre);
  SetMood(StringUtils::Join(artist.moods, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  SetDateAdded(artist.dateAdded);
  SetDateUpdated(artist.dateUpdated);
  SetDateNew(artist.dateNew);
  SetDatabaseId(artist.idArtist, MediaTypeArtist);

  SetLoaded();
}

void CMusicInfoTag::SetAlbum(const CAlbum& album)
{
  Clear();
  //Set all artist information from album artist credits and artist description
  SetArtistDesc(album.GetAlbumArtistString());
  SetArtist(album.GetAlbumArtist());
  SetArtistSort(album.GetAlbumArtistSort());
  SetMusicBrainzArtistID(album.GetMusicBrainzAlbumArtistID());
  SetAlbumArtistDesc(album.GetAlbumArtistString());
  SetAlbumArtist(album.GetAlbumArtist());
  SetAlbumArtistSort(album.GetAlbumArtistSort());
  SetMusicBrainzAlbumArtistID(album.GetMusicBrainzAlbumArtistID());
  SetAlbumId(album.idAlbum);
  SetAlbum(album.strAlbum);
  SetTitle(album.strAlbum);
  SetMusicBrainzAlbumID(album.strMusicBrainzAlbumID);
  SetMusicBrainzReleaseGroupID(album.strReleaseGroupMBID);
  SetMusicBrainzReleaseType(album.strType);
  SetAlbumReleaseStatus(album.strReleaseStatus);
  SetGenre(album.genre);
  SetMood(StringUtils::Join(album.moods, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  SetRecordLabel(album.strLabel);
  SetRating(album.fRating);
  SetUserrating(album.iUserrating);
  SetVotes(album.iVotes);
  SetCompilation(album.bCompilation);
  SetOriginalDate(album.strOrigReleaseDate);
  SetReleaseDate(album.strReleaseDate);
  SetBoxset(album.bBoxedSet);
  SetAlbumReleaseType(album.releaseType);
  SetDateAdded(album.dateAdded);
  SetDateUpdated(album.dateUpdated);
  SetDateNew(album.dateNew);
  SetPlayCount(album.iTimesPlayed);
  SetDatabaseId(album.idAlbum, MediaTypeAlbum);
  SetLastPlayed(album.lastPlayed);
  SetTotalDiscs(album.iTotalDiscs);
  SetDuration(album.iAlbumDuration);

  SetLoaded();
}

void CMusicInfoTag::SetSong(const CSong& song)
{
  Clear();
  SetTitle(song.strTitle);
  SetGenre(song.genre);
  /* Set all artist information from song artist credits and artist description.
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
  SetArtistSort(song.GetArtistSort());
  SetAlbum(song.strAlbum);
  SetAlbumArtist(song.GetAlbumArtist()); //Only have album artist in song as vector, no desc or MBID
  SetAlbumArtistSort(song.GetAlbumArtistSort());
  SetMusicBrainzTrackID(song.strMusicBrainzTrackID);
  SetContributors(song.GetContributors());
  SetComment(song.strComment);
  SetCueSheet(song.strCueSheet);
  SetPlayCount(song.iTimesPlayed);
  SetLastPlayed(song.lastPlayed);
  SetDateAdded(song.dateAdded);
  SetDateUpdated(song.dateUpdated);
  SetDateNew(song.dateNew);
  SetCoverArtInfo(song.embeddedArt.m_size, song.embeddedArt.m_mime);
  SetRating(song.rating);
  SetUserrating(song.userrating);
  SetVotes(song.votes);
  SetURL(song.strFileName);
  SetReleaseDate(song.strReleaseDate);
  SetOriginalDate(song.strOrigReleaseDate);
  SetTrackAndDiscNumber(song.iTrack);
  SetDiscSubtitle(song.strDiscSubtitle);
  SetDuration(song.iDuration);
  SetMood(song.strMood);
  SetCompilation(song.bCompilation);
  SetAlbumId(song.idAlbum);
  SetDatabaseId(song.idSong, MediaTypeSong);
  SetBPM(song.iBPM);
  SetBitRate(song.iBitRate);
  SetSampleRate(song.iSampleRate);
  SetNoOfChannels(song.iChannels);
  SetSongVideoURL(song.songVideoURL);

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
  // be accurately derived by simply splitting the artist desc. Hence m_artist is only populated when the individual artists are
  // queried, whereas GetArtistString() will always return the artist description.
  // To avoid empty artist array in JSON, when m_artist is empty then an attempt is made to split the artist desc into artists.
  // A longer term solution would be to ensure that when individual artists are to be returned then the song_artist and artist tables
  // are queried.
  if (m_artist.empty())
    value["artist"] = StringUtils::Split(GetArtistString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);

  value["displayartist"] = GetArtistString();
  value["displayalbumartist"] = GetAlbumArtistString();
  value["sortartist"] = GetArtistSort();
  value["album"] = m_strAlbum;
  value["albumartist"] = m_albumArtist;
  value["sortalbumartist"] = m_strAlbumArtistSort;
  value["genre"] = m_genre;
  value["duration"] = m_iDuration;
  value["track"] = GetTrackNumber();
  value["disc"] = GetDiscNumber();
  value["loaded"] = m_bLoaded;
  value["year"] = GetYear(); // Optionally from m_strOriginalDate
  value["musicbrainztrackid"] = m_strMusicBrainzTrackID;
  value["musicbrainzartistid"] = m_musicBrainzArtistID;
  value["musicbrainzalbumid"] = m_strMusicBrainzAlbumID;
  value["musicbrainzreleasegroupid"] = m_strMusicBrainzReleaseGroupID;
  value["musicbrainzalbumartistid"] = m_musicBrainzAlbumArtistID;
  value["comment"] = m_strComment;
  value["contributors"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& role : m_musicRoles)
  {
    CVariant contributor;
    contributor["name"] = role.GetArtist();
    contributor["role"] = role.GetRoleDesc();
    contributor["roleid"] = role.GetRoleId();
    contributor["artistid"] = (int)(role.GetArtistId());
    value["contributors"].push_back(contributor);
  }
  value["displaycomposer"] = GetArtistStringForRole("composer");   //TCOM
  value["displayconductor"] = GetArtistStringForRole("conductor"); //TPE3
  value["displayorchestra"] = GetArtistStringForRole("orchestra");
  value["displaylyricist"] = GetArtistStringForRole("lyricist");   //TEXT
  value["mood"] = StringUtils::Split(m_strMood, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  value["recordlabel"] = m_strRecordLabel;
  value["rating"] = m_Rating;
  value["userrating"] = m_Userrating;
  value["votes"] = m_Votes;
  value["playcount"] = m_iTimesPlayed;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDateTime() : StringUtils::Empty;
  value["dateadded"] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty;
  value["datenew"] = m_dateNew.IsValid() ? m_dateNew.GetAsDBDateTime() : StringUtils::Empty;
  value["datemodified"] =
      m_dateUpdated.IsValid() ? m_dateUpdated.GetAsDBDateTime() : StringUtils::Empty;
  value["lyrics"] = m_strLyrics;
  value["albumid"] = m_iAlbumId;
  value["compilationartist"] = m_bCompilation;
  value["compilation"] = m_bCompilation;
  if (m_type.compare(MediaTypeAlbum) == 0)
    value["releasetype"] = CAlbum::ReleaseTypeToString(m_albumReleaseType);
  else if (m_type.compare(MediaTypeSong) == 0)
    value["albumreleasetype"] = CAlbum::ReleaseTypeToString(m_albumReleaseType);
  value["isboxset"] = m_bBoxset;
  value["totaldiscs"] = m_iDiscTotal;
  value["disctitle"] = m_strDiscSubtitle;
  value["releasedate"] = m_strReleaseDate;
  value["originaldate"] = m_strOriginalDate;
  value["albumstatus"] = m_strReleaseStatus;
  value["bpm"] = m_iBPM;
  value["bitrate"] = m_bitrate;
  value["samplerate"] = m_samplerate;
  value["channels"] = m_channels;
  value["songvideourl"] = m_songVideoURL;
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
  case FieldArtistSort:  sortable[FieldArtistSort] = m_strArtistSort; break;
  case FieldAlbum:       sortable[FieldAlbum] = m_strAlbum; break;
  case FieldAlbumArtist: sortable[FieldAlbumArtist] = m_strAlbumArtistDesc; break;
  case FieldGenre:       sortable[FieldGenre] = m_genre; break;
  case FieldTime:        sortable[FieldTime] = m_iDuration; break;
  case FieldTrackNumber: sortable[FieldTrackNumber] = m_iTrack; break;
  case FieldTotalDiscs:
    sortable[FieldTotalDiscs] = m_iDiscTotal;
    break;
  case FieldYear:
    sortable[FieldYear] = GetYear();  // Optionally from m_strOriginalDate
    break;
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
  case FieldOrigDate:    sortable[FieldOrigDate] = m_strOriginalDate; break;
  case FieldBPM:         sortable[FieldBPM] = m_iBPM; break;
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
    ar << m_strArtistSort;
    ar << m_strArtistDesc;
    ar << m_strAlbum;
    ar << m_albumArtist;
    ar << m_strAlbumArtistDesc;
    ar << m_genre;
    ar << m_iDuration;
    ar << m_iTrack;
    ar << m_bLoaded;
    ar << m_strReleaseDate;
    ar << m_strOriginalDate;
    ar << m_strMusicBrainzTrackID;
    ar << m_musicBrainzArtistID;
    ar << m_strMusicBrainzAlbumID;
    ar << m_strMusicBrainzReleaseGroupID;
    ar << m_musicBrainzAlbumArtistID;
    ar << m_strDiscSubtitle;
    ar << m_bBoxset;
    ar << m_iDiscTotal;
    ar << m_strMusicBrainzReleaseType;
    ar << m_lastPlayed;
    ar << m_dateAdded;
    ar << m_strComment;
    ar << (int)m_musicRoles.size();
    for (const auto& credit : m_musicRoles)
    {
      ar << credit.GetRoleId();
      ar << credit.GetRoleDesc();
      ar << credit.GetArtist();
      ar << credit.GetArtistId();
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
    ar << m_strReleaseStatus;
    ar << m_strLyrics;
    ar << m_bCompilation;
    ar << m_listeners;
    ar << m_coverArt;
    ar << m_cuesheet;
    ar << static_cast<int>(m_albumReleaseType);
    ar << m_iBPM;
    ar << m_samplerate;
    ar << m_bitrate;
    ar << m_channels;
    ar << m_songVideoURL;
  }
  else
  {
    ar >> m_strURL;
    ar >> m_strTitle;
    ar >> m_artist;
    ar >> m_strArtistSort;
    ar >> m_strArtistDesc;
    ar >> m_strAlbum;
    ar >> m_albumArtist;
    ar >> m_strAlbumArtistDesc;
    ar >> m_genre;
    ar >> m_iDuration;
    ar >> m_iTrack;
    ar >> m_bLoaded;
    ar >> m_strReleaseDate;
    ar >> m_strOriginalDate;
    ar >> m_strMusicBrainzTrackID;
    ar >> m_musicBrainzArtistID;
    ar >> m_strMusicBrainzAlbumID;
    ar >> m_strMusicBrainzReleaseGroupID;
    ar >> m_musicBrainzAlbumArtistID;
    ar >> m_strDiscSubtitle;
    ar >> m_bBoxset;
    ar >> m_iDiscTotal;
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
      int idArtist;
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
    ar >> m_strReleaseStatus;
    ar >> m_strLyrics;
    ar >> m_bCompilation;
    ar >> m_listeners;
    ar >> m_coverArt;
    ar >> m_cuesheet;

    int albumReleaseType;
    ar >> albumReleaseType;
    m_albumReleaseType = static_cast<CAlbum::ReleaseType>(albumReleaseType);
    ar >> m_iBPM;
    ar >> m_samplerate;
    ar >> m_bitrate;
    ar >> m_channels;
    ar >> m_songVideoURL;
  }
}

void CMusicInfoTag::Clear()
{
  m_strURL.clear();
  m_artist.clear();
  m_strArtistSort.clear();
  m_strComposerSort.clear();
  m_strAlbum.clear();
  m_albumArtist.clear();
  m_genre.clear();
  m_strTitle.clear();
  m_strMusicBrainzTrackID.clear();
  m_musicBrainzArtistID.clear();
  m_strMusicBrainzAlbumID.clear();
  m_strMusicBrainzReleaseGroupID.clear();
  m_musicBrainzAlbumArtistID.clear();
  m_strMusicBrainzReleaseType.clear();
  m_musicRoles.clear();
  m_iDuration = 0;
  m_iTrack = 0;
  m_bLoaded = false;
  m_lastPlayed.Reset();
  m_dateAdded.Reset();
  m_dateNew.Reset();
  m_dateUpdated.Reset();
  m_bCompilation = false;
  m_bBoxset = false;
  m_strDiscSubtitle.clear();
  m_strComment.clear();
  m_strMood.clear();
  m_strRecordLabel.clear();
  m_cuesheet.clear();
  m_iDbId = -1;
  m_type.clear();
  m_strReleaseStatus.clear();
  m_iTimesPlayed = 0;
  m_strReleaseDate.clear();
  m_strOriginalDate.clear();
  m_iAlbumId = -1;
  m_coverArt.Clear();
  m_replayGain = ReplayGain();
  m_albumReleaseType = CAlbum::Album;
  m_listeners = 0;
  m_Rating = 0;
  m_Userrating = 0;
  m_Votes = 0;
  m_iDiscTotal = 0;
  m_iBPM = 0;
  m_samplerate = 0;
  m_bitrate = 0;
  m_channels = 0;
  m_stationName.clear();
  m_stationArt.clear();
  m_songVideoURL.clear();
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
    AddArtistRole(Role, StringUtils::Split(strArtist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CMusicInfoTag::AddArtistRole(const std::string& Role, const std::vector<std::string>& artists)
{
  for (unsigned int index = 0; index < artists.size(); index++)
  {
    CMusicRole ArtistCredit(Role, Trim(artists.at(index)));
    //Prevent duplicate entries
    auto credit = find(m_musicRoles.begin(), m_musicRoles.end(), ArtistCredit);
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
  for (const auto& credit : m_musicRoles)
  {
    if (StringUtils::EqualsNoCase(credit.GetRoleDesc(), strRole))
      artistvector.push_back(credit.GetArtist());
  }
  return StringUtils::Join(artistvector, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
}

const std::string CMusicInfoTag::GetContributorsText() const
{
  std::string strLabel;
  for (const auto& credit : m_musicRoles)
  {
    strLabel += StringUtils::Format("{}\n", credit.GetArtist());
  }
  return StringUtils::TrimRight(strLabel, "\n");
}

const std::string CMusicInfoTag::GetContributorsAndRolesText() const
{
  std::string strLabel;
  for (const auto& credit : m_musicRoles)
  {
    strLabel += StringUtils::Format("{} - {}\n", credit.GetRoleDesc(), credit.GetArtist());
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
