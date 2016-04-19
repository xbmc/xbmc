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

#include "Song.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"

using namespace MUSIC_INFO;

CSong::CSong(CFileItem& item)
{
  CMusicInfoTag& tag = *item.GetMusicInfoTag();
  SYSTEMTIME stTime;
  tag.GetReleaseDate(stTime);
  strTitle = tag.GetTitle();
  genre = tag.GetGenre();
  std::vector<std::string> artist = tag.GetArtist();
  std::vector<std::string> musicBrainzArtistHints = tag.GetMusicBrainzArtistHints();
  strArtistDesc = tag.GetArtistString();

  if (!tag.GetMusicBrainzArtistID().empty())
  { // Have musicbrainz artist info, so use it

    // Vector of possible separators in the order least likely to be part of artist name
    const std::vector<std::string> separators{ " feat. ", " ft. ", " Feat. "," Ft. ", ";", ":", "|", "#", "/", " with ", ",", "&" };

    // Establish tag consistency - do the number of musicbrainz ids and number of names in hints or artist match
    if (tag.GetMusicBrainzArtistID().size() != musicBrainzArtistHints.size() &&
        tag.GetMusicBrainzArtistID().size() != artist.size())
    {
      // Tags mis-match - report it and then try to fix
      CLog::Log(LOGDEBUG, "Mis-match in song file tags: %i mbid %i names %s %s", 
        (int)tag.GetMusicBrainzArtistID().size(), (int)artist.size(), strTitle.c_str(), strArtistDesc.c_str());
      /*
        Most likey we have no hints and a single artist name like "Artist1 feat. Artist2"
        or "Composer; Conductor, Orchestra, Soloist" or "Artist1/Artist2" where the
        expected single item separator (default = space-slash-space) as not been used.
        Ampersand (&), comma and slash (no spaces) are poor delimiters as could be in name
        e.g. "AC/DC", "Earth, Wind & Fire", but here treat them as such in attempt to find artist names.
        When there are hints but count not match mbid they could be poorly formatted using unexpected
        separators so attempt to split them. Or we could have more hints or artist names than
        musicbrainz id so ingore them but raise warning.
      */
      // Do hints exist yet mis-match
      if (musicBrainzArtistHints.size() > 0 &&
        musicBrainzArtistHints.size() != tag.GetMusicBrainzArtistID().size())
      {
        if (artist.size() == tag.GetMusicBrainzArtistID().size())
          // Artist name count matches, use that as hints
          musicBrainzArtistHints = artist;
        else if (musicBrainzArtistHints.size() < tag.GetMusicBrainzArtistID().size())
        { // Try splitting the hints until have matching number
          musicBrainzArtistHints = StringUtils::SplitMulti(musicBrainzArtistHints, separators, tag.GetMusicBrainzArtistID().size());
        }
        else
          // Extra hints, discard them.
          musicBrainzArtistHints.resize(tag.GetMusicBrainzArtistID().size());
      }
      // Do hints not exist or still mis-match, try artists
      if (musicBrainzArtistHints.size() != tag.GetMusicBrainzArtistID().size())
        musicBrainzArtistHints = artist;
      // Still mis-match, try splitting the hints (now artists) until have matching number
      if (musicBrainzArtistHints.size() < tag.GetMusicBrainzArtistID().size())
      {
        musicBrainzArtistHints = StringUtils::SplitMulti(musicBrainzArtistHints, separators, tag.GetMusicBrainzArtistID().size());
      }
    }
    else
    { // Either hints or artist names (or both) matches number of musicbrainz id
      // If hints mis-match, use artists
      if (musicBrainzArtistHints.size() != tag.GetMusicBrainzArtistID().size())
        musicBrainzArtistHints = tag.GetArtist();
    }

    for (size_t i = 0; i < tag.GetMusicBrainzArtistID().size(); i++)
    {
      std::string artistId = tag.GetMusicBrainzArtistID()[i];
      std::string artistName;
      /*
       We try and get the corresponding artist name from the hints list.
       Having already attempted to make the number of hints match, if they
       still don't then use musicbrainz id as the name and hope later on we
       can update that entry.
      */
      if (i < musicBrainzArtistHints.size())
        artistName = musicBrainzArtistHints[i];
      else
        artistName = artistId;
      artistCredits.emplace_back(StringUtils::Trim(artistName), artistId);
    }
  }
  else
  { // No musicbrainz artist ids, so fill in directly
    // Separate artist names further, if possible, and trim blank space.
    if (musicBrainzArtistHints.size() > tag.GetArtist().size())
      // Make use of hints (ARTISTS tag), when present, to separate artist names
      artist = musicBrainzArtistHints;
    else
      // Split artist names further using multiple possible delimiters, over single separator applied in Tag loader
      artist = StringUtils::SplitMulti(artist, g_advancedSettings.m_musicArtistSeparators);

    for (auto artistname: artist)
    {
      artistCredits.emplace_back(StringUtils::Trim(artistname));
    }
  }
  strAlbum = tag.GetAlbum();
  m_albumArtist = tag.GetAlbumArtist();
  // Separate album artist names further, if possible, and trim blank space.
  if (tag.GetMusicBrainzAlbumArtistHints().size() > m_albumArtist.size())
    // Make use of hints (ALBUMARTISTS tag), when present, to separate artist names
    m_albumArtist = tag.GetMusicBrainzAlbumArtistHints();
  else
    // Split album artist names further using multiple possible delimiters, over single separator applied in Tag loader
    m_albumArtist = StringUtils::SplitMulti(m_albumArtist, g_advancedSettings.m_musicArtistSeparators);
  for (auto artistname : m_albumArtist)
    StringUtils::Trim(artistname);

  strMusicBrainzTrackID = tag.GetMusicBrainzTrackID();
  m_musicRoles = tag.GetContributors();
  strComment = tag.GetComment();
  strCueSheet = tag.GetCueSheet();
  strMood = tag.GetMood();
  rating = tag.GetRating();
  userrating = tag.GetUserrating();
  votes = tag.GetVotes();
  iYear = stTime.wYear;
  iTrack = tag.GetTrackAndDiscNumber();
  iDuration = tag.GetDuration();
  bCompilation = tag.GetCompilation();
  embeddedArt = tag.GetCoverArtInfo();
  strFileName = tag.GetURL().empty() ? item.GetPath() : tag.GetURL();
  dateAdded = tag.GetDateAdded();
  strThumb = item.GetUserMusicThumb(true);
  iStartOffset = item.m_lStartOffset;
  iEndOffset = item.m_lEndOffset;
  idSong = -1;
  iTimesPlayed = 0;
  idAlbum = -1;
}

CSong::CSong()
{
  Clear();
}

void CSong::MergeScrapedSong(const CSong& source, bool override)
{
  if ((override && !source.strTitle.empty()) || strTitle.empty())
    strTitle = source.strTitle;
  if ((override && source.iTrack != 0) || iTrack == 0)
    iTrack = source.iTrack;
  // artist = source.artist; // artist is read-only from the database
  if (override)
    artistCredits = source.artistCredits;
  else if (source.artistCredits.size() > artistCredits.size())
    artistCredits.insert(artistCredits.end(), source.artistCredits.begin()+artistCredits.size(), source.artistCredits.end());
}

void CSong::Serialize(CVariant& value) const
{
  value["filename"] = strFileName;
  value["title"] = strTitle;
  value["artist"] = GetArtist();
  value["album"] = strAlbum;
  value["albumartist"] = GetAlbumArtist();
  value["genre"] = genre;
  value["duration"] = iDuration;
  value["track"] = iTrack;
  value["year"] = iYear;
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
}

void CSong::Clear()
{
  strFileName.clear();
  strTitle.clear();
  strAlbum.clear();
  m_albumArtist.clear();
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
  iYear = 0;
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
  iTimesPlayed = 0;
  lastPlayed.Reset();
  dateAdded.Reset();
  idAlbum = -1;
  bCompilation = false;
  embeddedArt.clear();
}
const std::vector<std::string> CSong::GetArtist() const
{
  //Get artist names as vector from artist credits
  std::vector<std::string> songartists;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
  {
    songartists.push_back(artistCredit->GetArtist());
  }
  //When artist credits have not been populated attempt to build an artist vector from the descrpition string
  //This is a tempory fix, in the longer term other areas should query the song_artist table and populate
  //artist credits. Note that splitting the string may not give the same artists as held in the song_artist table
  if (songartists.empty() && !strArtistDesc.empty())
    songartists = StringUtils::Split(strArtistDesc, g_advancedSettings.m_musicItemSeparator);
  return songartists;
}

const std::vector<std::string> CSong::GetMusicBrainzArtistID() const
{
  //Get artist MusicBrainz IDs as vector from artist credits
  std::vector<std::string> muisicBrainzID;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
  {
    muisicBrainzID.push_back(artistCredit->GetMusicBrainzArtistID());
  }
  return muisicBrainzID;
}

const std::string CSong::GetArtistString() const
{
  //Artist description may be different from the artists in artistcredits (see ARTISTS tag processing)
  //but is takes precidence as a string because artistcredits is not always filled during processing
  if (!strArtistDesc.empty())
    return strArtistDesc;
  std::vector<std::string> artistvector;
  for (VECARTISTCREDITS::const_iterator i = artistCredits.begin(); i != artistCredits.end(); ++i)
    artistvector.push_back(i->GetArtist());
  std::string artistString;
  if (!artistvector.empty())
    artistString = StringUtils::Join(artistvector, g_advancedSettings.m_musicItemSeparator);
  return artistString;
}

const std::vector<int> CSong::GetArtistIDArray() const
{
  // Get song artist IDs for json rpc
  std::vector<int> artistids;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
    artistids.push_back(artistCredit->GetArtistId());
  return artistids;
}

void CSong::AppendArtistRole(const CMusicRole& musicRole)
{
  m_musicRoles.push_back(musicRole);
}

bool CSong::HasArt() const
{
  if (!strThumb.empty()) return true;
  if (!embeddedArt.empty()) return true;
  return false;
}

bool CSong::ArtMatches(const CSong &right) const
{
  return (right.strThumb == strThumb &&
          embeddedArt.matches(right.embeddedArt));
}
