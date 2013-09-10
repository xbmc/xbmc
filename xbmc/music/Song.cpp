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
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"

using namespace std;
using namespace MUSIC_INFO;

CSong::CSong(CFileItem& item)
{
  CMusicInfoTag& tag = *item.GetMusicInfoTag();
  SYSTEMTIME stTime;
  tag.GetReleaseDate(stTime);
  strTitle = tag.GetTitle();
  genre = tag.GetGenre();
  artist = tag.GetArtist();
  bool hasMusicBrainzArtist = !tag.GetMusicBrainzArtistID().empty();
  const vector<string>& artists = hasMusicBrainzArtist ? tag.GetMusicBrainzArtistID() : tag.GetArtist();
  for (vector<string>::const_iterator it = artists.begin(); it != artists.end(); ++it)
  {
    CStdString artistName = hasMusicBrainzArtist && !artist.empty() ? artist[0] : *it;
    CStdString artistId = hasMusicBrainzArtist ? *it : StringUtils::EmptyString;
    CStdString strJoinPhrase = (it == --artists.end() ? "" : g_advancedSettings.m_musicItemSeparator);
    CArtistCredit artistCredit(artistName, artistId, strJoinPhrase);
    artistCredits.push_back(artistCredit);
  }
  strAlbum = tag.GetAlbum();
  albumArtist = tag.GetAlbumArtist();
  strMusicBrainzTrackID = tag.GetMusicBrainzTrackID();
  strComment = tag.GetComment();
  rating = tag.GetRating();
  iYear = stTime.wYear;
  iTrack = tag.GetTrackAndDiskNumber();
  iDuration = tag.GetDuration();
  bCompilation = tag.GetCompilation();
  embeddedArt = tag.GetCoverArtInfo();
  strFileName = tag.GetURL().empty() ? item.GetPath() : tag.GetURL();
  strThumb = item.GetUserMusicThumb(true);
  iStartOffset = item.m_lStartOffset;
  iEndOffset = item.m_lEndOffset;
  idSong = -1;
  iTimesPlayed = 0;
  iKaraokeNumber = 0;
  iKaraokeDelay = 0;         //! Karaoke song lyrics-music delay in 1/10 seconds.
  idAlbum = -1;
}

CSong::CSong()
{
  Clear();
}

void CSong::Serialize(CVariant& value) const
{
  value["filename"] = strFileName;
  value["title"] = strTitle;
  value["artist"] = artist;
  value["album"] = strAlbum;
  value["albumartist"] = albumArtist;
  value["genre"] = genre;
  value["duration"] = iDuration;
  value["track"] = iTrack;
  value["year"] = iYear;
  value["musicbrainztrackid"] = strMusicBrainzTrackID;
  value["comment"] = strComment;
  value["rating"] = rating;
  value["timesplayed"] = iTimesPlayed;
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDateTime() : "";
  value["karaokenumber"] = (int64_t) iKaraokeNumber;
  value["albumid"] = idAlbum;
}

void CSong::Clear()
{
  strFileName.clear();
  strTitle.clear();
  artist.clear();
  strAlbum.clear();
  albumArtist.clear();
  genre.clear();
  strThumb.clear();
  strMusicBrainzTrackID.clear();
  strComment.clear();
  rating = '0';
  iTrack = 0;
  iDuration = 0;
  iYear = 0;
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
  iTimesPlayed = 0;
  lastPlayed.Reset();
  iKaraokeNumber = 0;
  strKaraokeLyrEncoding.clear();
  iKaraokeDelay = 0;
  idAlbum = -1;
  bCompilation = false;
  embeddedArt.clear();
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
