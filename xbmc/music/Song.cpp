/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Song.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

CSong::CSong(CFileItem& item)
{
  CMusicInfoTag& tag = *item.GetMusicInfoTag();
  strTitle = tag.GetTitle();
  genre = tag.GetGenre();
  strArtistDesc = tag.GetArtistString();
  //Set sort string before processing artist credits
  strArtistSort = tag.GetArtistSort();
  m_strComposerSort = tag.GetComposerSort();

  // Determine artist credits from various tag arrays
  SetArtistCredits(tag.GetArtist(), tag.GetMusicBrainzArtistHints(), tag.GetMusicBrainzArtistID());

  strAlbum = tag.GetAlbum();
  m_albumArtist = tag.GetAlbumArtist();
  // Separate album artist names further, if possible, and trim blank space.
  if (tag.GetMusicBrainzAlbumArtistHints().size() > m_albumArtist.size())
    // Make use of hints (ALBUMARTISTS tag), when present, to separate artist names
    m_albumArtist = tag.GetMusicBrainzAlbumArtistHints();
  else
    // Split album artist names further using multiple possible delimiters, over single separator applied in Tag loader
    m_albumArtist = StringUtils::SplitMulti(m_albumArtist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicArtistSeparators);
  for (auto artistname : m_albumArtist)
    StringUtils::Trim(artistname);
  m_strAlbumArtistSort = tag.GetAlbumArtistSort();

  strMusicBrainzTrackID = tag.GetMusicBrainzTrackID();
  m_musicRoles = tag.GetContributors();
  strComment = tag.GetComment();
  strCueSheet = tag.GetCueSheet();
  strMood = tag.GetMood();
  rating = tag.GetRating();
  userrating = tag.GetUserrating();
  votes = tag.GetVotes();
  strOrigReleaseDate = tag.GetOriginalDate();
  strReleaseDate = tag.GetReleaseDate();
  strDiscSubtitle = tag.GetDiscSubtitle();
  iTrack = tag.GetTrackAndDiscNumber();
  iDuration = tag.GetDuration();
  strRecordLabel = tag.GetRecordLabel();
  strAlbumType = tag.GetMusicBrainzReleaseType();
  bCompilation = tag.GetCompilation();
  embeddedArt = tag.GetCoverArtInfo();
  strFileName = tag.GetURL().empty() ? item.GetPath() : tag.GetURL();
  dateAdded = tag.GetDateAdded();
  replayGain = tag.GetReplayGain();
  strThumb = item.GetUserMusicThumb(true);
  iStartOffset = static_cast<int>(item.GetStartOffset());
  iEndOffset = static_cast<int>(item.GetEndOffset());
  idSong = -1;
  iTimesPlayed = 0;
  idAlbum = -1;
  iBPM = tag.GetBPM();
  iSampleRate = tag.GetSampleRate();
  iBitRate = tag.GetBitRate();
  iChannels = tag.GetNoOfChannels();
  songVideoURL = tag.GetSongVideoURL();
}

CSong::CSong()
{
  Clear();
}

void CSong::SetArtistCredits(const std::vector<std::string>& names, const std::vector<std::string>& hints,
  const std::vector<std::string>& mbids)
{
  artistCredits.clear();
  std::vector<std::string> artistHints = hints;
  //Split the artist sort string to try and get sort names for individual artists
  std::vector<std::string> artistSort = StringUtils::Split(strArtistSort, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);

  if (!mbids.empty())
  { // Have musicbrainz artist info, so use it

    // Vector of possible separators in the order least likely to be part of artist name
    const std::vector<std::string> separators{ " feat. ", " ft. ", " Feat. "," Ft. ", ";", ":", "|", "#", "/", " with ", ",", "&" };

    // Establish tag consistency - do the number of musicbrainz ids and number of names in hints or artist match
    if (mbids.size() != artistHints.size() && mbids.size() != names.size())
    {
      // Tags mismatch - report it and then try to fix
      CLog::Log(LOGDEBUG, "Mismatch in song file tags: {} mbid {} names {} {}", (int)mbids.size(),
                (int)names.size(), strTitle, strArtistDesc);
      /*
        Most likely we have no hints and a single artist name like "Artist1 feat. Artist2"
        or "Composer; Conductor, Orchestra, Soloist" or "Artist1/Artist2" where the
        expected single item separator (default = space-slash-space) as not been used.
        Ampersand (&), comma and slash (no spaces) are poor delimiters as could be in name
        e.g. "AC/DC", "Earth, Wind & Fire", but here treat them as such in attempt to find artist names.
        When there are hints but count not match mbid they could be poorly formatted using unexpected
        separators so attempt to split them. Or we could have more hints or artist names than
        musicbrainz id so ignore them but raise warning.
      */
      // Do hints exist yet mismatch
      if (artistHints.size() > 0 &&
        artistHints.size() != mbids.size())
      {
        if (names.size() == mbids.size())
          // Artist name count matches, use that as hints
          artistHints = names;
        else if (artistHints.size() < mbids.size())
        { // Try splitting the hints until have matching number
          artistHints = StringUtils::SplitMulti(artistHints, separators, mbids.size());
        }
        else
          // Extra hints, discard them.
          artistHints.resize(mbids.size());
      }
      // Do hints not exist or still mismatch, try artists
      if (artistHints.size() != mbids.size())
        artistHints = names;
      // Still mismatch, try splitting the hints (now artists) until have matching number
      if (artistHints.size() < mbids.size())
      {
        artistHints = StringUtils::SplitMulti(artistHints, separators, mbids.size());
      }
    }
    else
    { // Either hints or artist names (or both) matches number of musicbrainz id
      // If hints mismatch, use artists
      if (artistHints.size() != mbids.size())
        artistHints = names;
    }

    // Try to get number of artist sort names and musicbrainz ids to match. Split sort names
    // further using multiple possible delimiters, over single separator applied in Tag loader
    if (artistSort.size() != mbids.size())
      artistSort = StringUtils::SplitMulti(artistSort, { ";", ":", "|", "#" });

    for (size_t i = 0; i < mbids.size(); i++)
    {
      std::string artistId = mbids[i];
      std::string artistName;
      /*
       We try and get the corresponding artist name from the hints list.
       Having already attempted to make the number of hints match, if they
       still don't then use musicbrainz id as the name and hope later on we
       can update that entry.
      */
      if (i < artistHints.size())
        artistName = artistHints[i];
      else
        artistName = artistId;

      // Use artist sort name providing we have as many as we have mbid,
      // otherwise something is wrong with them so ignore and leave blank
      if (artistSort.size() == mbids.size())
        artistCredits.emplace_back(StringUtils::Trim(artistName), StringUtils::Trim(artistSort[i]), artistId);
      else
        artistCredits.emplace_back(StringUtils::Trim(artistName), "", artistId);
    }
  }
  else
  { // No musicbrainz artist ids, so fill in directly
    // Separate artist names further, if possible, and trim blank space.
    std::vector<std::string> artists = names;
    if (artistHints.size() > names.size())
      // Make use of hints (ARTISTS tag), when present, to separate artist names
      artists = artistHints;
    else
      // Split artist names further using multiple possible delimiters, over single separator applied in Tag loader
      artists = StringUtils::SplitMulti(artists, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicArtistSeparators);

    if (artistSort.size() != artists.size())
      // Split artist sort names further using multiple possible delimiters, over single separator applied in Tag loader
      artistSort = StringUtils::SplitMulti(artistSort, { ";", ":", "|", "#" });

    for (size_t i = 0; i < artists.size(); i++)
    {
      artistCredits.emplace_back(StringUtils::Trim(artists[i]));
      // Set artist sort name providing we have as many as we have artists,
      // otherwise something is wrong with them so ignore rather than guess.
      if (artistSort.size() == artists.size())
        artistCredits.back().SetSortName(StringUtils::Trim(artistSort[i]));
    }
  }

}

void CSong::MergeScrapedSong(const CSong& source, bool override)
{
  // Merge when MusicBrainz Track ID match (checked in CAlbum::MergeScrapedAlbum)
  if ((override && !source.strTitle.empty()) || strTitle.empty())
    strTitle = source.strTitle;
  if ((override && source.iTrack != 0) || iTrack == 0)
    iTrack = source.iTrack;
  if (override)
  {
    artistCredits = source.artistCredits; // Replace artists and store mbid returned by scraper
    strArtistDesc.clear();  // @todo: set artist display string e.g. "artist1 feat. artist2" when scraped
  }
}

void CSong::Serialize(CVariant& value) const
{
  value["filename"] = strFileName;
  value["title"] = strTitle;
  value["artist"] = GetArtist();
  value["artistsort"] = GetArtistSort();  // a string for the song not vector of values for each artist
  value["album"] = strAlbum;
  value["albumartist"] = GetAlbumArtist();
  value["genre"] = genre;
  value["duration"] = iDuration;
  value["track"] = iTrack;
  value["year"] = atoi(strReleaseDate.c_str());;
  value["musicbrainztrackid"] = strMusicBrainzTrackID;
  value["comment"] = strComment;
  value["mood"] = strMood;
  value["rating"] = rating;
  value["userrating"] = userrating;
  value["votes"] = votes;
  value["timesplayed"] = iTimesPlayed;
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDateTime() : "";
  value["dateadded"] = dateAdded.IsValid() ? dateAdded.GetAsDBDateTime() : "";
  value["albumid"] = idAlbum;
  value["albumreleasedate"] = strReleaseDate;
  value["bpm"] = iBPM;
  value["bitrate"] = iBitRate;
  value["samplerate"] = iSampleRate;
  value["channels"] = iChannels;
  value["songvideourl"] = songVideoURL;
}

void CSong::Clear()
{
  strFileName.clear();
  strTitle.clear();
  strAlbum.clear();
  strArtistSort.clear();
  strArtistDesc.clear();
  m_albumArtist.clear();
  m_strAlbumArtistSort.clear();
  genre.clear();
  strThumb.clear();
  strMusicBrainzTrackID.clear();
  m_musicRoles.clear();
  strComment.clear();
  strMood.clear();
  rating = 0;
  userrating = 0;
  votes = 0;
  iTrack = 0;
  iDuration = 0;
  strOrigReleaseDate.clear();
  strReleaseDate.clear();
  strDiscSubtitle.clear();
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
  iTimesPlayed = 0;
  lastPlayed.Reset();
  dateAdded.Reset();
  dateUpdated.Reset();
  dateNew.Reset();
  idAlbum = -1;
  bCompilation = false;
  embeddedArt.Clear();
  iBPM = 0;
  iBitRate = 0;
  iSampleRate = 0;
  iChannels =  0;
  songVideoURL.clear();

  replayGain = ReplayGain();
}
const std::vector<std::string> CSong::GetArtist() const
{
  //Get artist names as vector from artist credits
  std::vector<std::string> songartists;
  for (const auto& artistCredit : artistCredits)
  {
    songartists.push_back(artistCredit.GetArtist());
  }
  //When artist credits have not been populated attempt to build an artist vector from the description string
  //This is a temporary fix, in the longer term other areas should query the song_artist table and populate
  //artist credits. Note that splitting the string may not give the same artists as held in the song_artist table
  if (songartists.empty() && !strArtistDesc.empty())
    songartists = StringUtils::Split(strArtistDesc, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  return songartists;
}

const std::string CSong::GetArtistSort() const
{
  //The stored artist sort name string takes precedence but a
  //value could be created from individual sort names held in artistcredits
  if (!strArtistSort.empty())
    return strArtistSort;
  std::vector<std::string> artistvector;
  for (const auto& artistcredit : artistCredits)
    if (!artistcredit.GetSortName().empty())
      artistvector.emplace_back(artistcredit.GetSortName());
  std::string artistString;
  if (!artistvector.empty())
    artistString = StringUtils::Join(artistvector, "; ");
  return artistString;
}

const std::vector<std::string> CSong::GetMusicBrainzArtistID() const
{
  //Get artist MusicBrainz IDs as vector from artist credits
  std::vector<std::string> musicBrainzID;
  for (const auto& artistCredit : artistCredits)
  {
    musicBrainzID.push_back(artistCredit.GetMusicBrainzArtistID());
  }
  return musicBrainzID;
}

const std::string CSong::GetArtistString() const
{
  //Artist description may be different from the artists in artistcredits (see ARTISTS tag processing)
  //but is takes precedence as a string because artistcredits is not always filled during processing
  if (!strArtistDesc.empty())
    return strArtistDesc;
  std::vector<std::string> artistvector;
  for (const auto& i : artistCredits)
    artistvector.push_back(i.GetArtist());
  std::string artistString;
  if (!artistvector.empty())
    artistString = StringUtils::Join(artistvector, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  return artistString;
}

const std::vector<int> CSong::GetArtistIDArray() const
{
  // Get song artist IDs for json rpc
  std::vector<int> artistids;
  for (const auto& artistCredit : artistCredits)
    artistids.push_back(artistCredit.GetArtistId());
  return artistids;
}

void CSong::AppendArtistRole(const CMusicRole& musicRole)
{
  m_musicRoles.push_back(musicRole);
}

bool CSong::HasArt() const
{
  if (!strThumb.empty()) return true;
  if (!embeddedArt.Empty()) return true;
  return false;
}

bool CSong::ArtMatches(const CSong &right) const
{
  return (right.strThumb == strThumb &&
          embeddedArt.Matches(right.embeddedArt));
}

const std::string CSong::GetDiscSubtitle() const
{
  return strDiscSubtitle;
}
