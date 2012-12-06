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

#include "Song.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/Variant.h"

using namespace std;
using namespace MUSIC_INFO;

CSong::CSong(CMusicInfoTag& tag)
{
  SYSTEMTIME stTime;
  tag.GetReleaseDate(stTime);
  strTitle = tag.GetTitle();
  genre = tag.GetGenre();
  strFileName = tag.GetURL();
  artist = tag.GetArtist();
  strAlbum = tag.GetAlbum();
  albumArtist = tag.GetAlbumArtist();
  strMusicBrainzTrackID = tag.GetMusicBrainzTrackID();
  strMusicBrainzArtistID = tag.GetMusicBrainzArtistID();
  strMusicBrainzAlbumID = tag.GetMusicBrainzAlbumID();
  strMusicBrainzAlbumArtistID = tag.GetMusicBrainzAlbumArtistID();
  strMusicBrainzTRMID = tag.GetMusicBrainzTRMID();
  strComment = tag.GetComment();
  rating = tag.GetRating();
  iYear = stTime.wYear;
  iTrack = tag.GetTrackAndDiskNumber();
  iDuration = tag.GetDuration();
  bCompilation = tag.GetCompilation();
  embeddedArt = tag.GetCoverArtInfo();
  strThumb = "";
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
  iTimesPlayed = 0;
  iKaraokeNumber = 0;
  iKaraokeDelay = 0;         //! Karaoke song lyrics-music delay in 1/10 seconds.
  iAlbumId = -1;
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
  value["musicbrainzartistid"] = strMusicBrainzArtistID;
  value["musicbrainzalbumid"] = strMusicBrainzAlbumID;
  value["musicbrainzalbumartistid"] = strMusicBrainzAlbumArtistID;
  value["musicbrainztrmid"] = strMusicBrainzTRMID;
  value["comment"] = strComment;
  value["rating"] = rating;
  value["timesplayed"] = iTimesPlayed;
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDateTime() : "";
  value["karaokenumber"] = (int64_t) iKaraokeNumber;
  value["albumid"] = iAlbumId;
}

void CSong::Clear()
{
  strFileName.Empty();
  strTitle.Empty();
  artist.clear();
  strAlbum.Empty();
  albumArtist.clear();
  genre.clear();
  strThumb.Empty();
  strMusicBrainzTrackID.Empty();
  strMusicBrainzArtistID.Empty();
  strMusicBrainzAlbumID.Empty();
  strMusicBrainzAlbumArtistID.Empty();
  strMusicBrainzTRMID.Empty();
  strComment.Empty();
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
  strKaraokeLyrEncoding.Empty();
  iKaraokeDelay = 0;
  iAlbumId = -1;
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

CSongMap::CSongMap()
{
}

std::map<CStdString, CSong>::const_iterator CSongMap::Begin()
{
  return m_map.begin();
}

std::map<CStdString, CSong>::const_iterator CSongMap::End()
{
  return m_map.end();
}

void CSongMap::Add(const CStdString &file, const CSong &song)
{
  CStdString lower = file;
  lower.ToLower();
  m_map.insert(pair<CStdString, CSong>(lower, song));
}

CSong* CSongMap::Find(const CStdString &file)
{
  CStdString lower = file;
  lower.ToLower();
  map<CStdString, CSong>::iterator it = m_map.find(lower);
  if (it == m_map.end())
    return NULL;
  return &(*it).second;
}

void CSongMap::Clear()
{
  m_map.erase(m_map.begin(), m_map.end());
}

int CSongMap::Size()
{
  return (int)m_map.size();
}

