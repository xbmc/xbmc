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
  strGenre = tag.GetGenre();
  strFileName = tag.GetURL();
  strArtist = tag.GetArtist();
  strAlbum = tag.GetAlbum();
  strAlbumArtist = tag.GetAlbumArtist();
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
  strThumb = "";
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
  iTimesPlayed = 0;
  iKaraokeNumber = 0;
  iKaraokeDelay = 0;         //! Karaoke song lyrics-music delay in 1/10 seconds.
  iArtistId = -1;
  iAlbumId = -1;
}

CSong::CSong()
{
  Clear();
}

void CSong::Serialize(CVariant& value)
{
  value["filename"] = strFileName;
  value["title"] = strTitle;
  value["artist"] = strArtist;
  value["album"] = strAlbum;
  value["albumartist"] = strAlbumArtist;
  value["genre"] = strGenre;
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
  value["karaokenumber"] = (int64_t) iKaraokeNumber;
  value["artistid"] = iArtistId;
  value["albumid"] = iAlbumId;
}

void CSong::Clear()
{
  strFileName.Empty();
  strTitle.Empty();
  strArtist.Empty();
  strAlbum.Empty();
  strAlbumArtist.Empty();
  strGenre.Empty();
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
  lastPlayed = "";
  iKaraokeNumber = 0;
  strKaraokeLyrEncoding.Empty();
  iKaraokeDelay = 0;
  iArtistId = -1;
  iAlbumId = -1;
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

