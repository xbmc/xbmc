/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Song.h"


CSong::CSong(CMusicInfoTag& tag)
{
  SYSTEMTIME stTime;
  tag.GetReleaseDate(stTime);
  strTitle = tag.GetTitle();
  strGenre = tag.GetGenre();
  strFileName = tag.GetURL();
  strArtist = tag.GetArtist();
  strAlbum = tag.GetAlbum();
  strMusicBrainzTrackID = tag.GetMusicBrainzTrackID();
  strMusicBrainzArtistID = tag.GetMusicBrainzArtistID();
  strMusicBrainzAlbumID = tag.GetMusicBrainzAlbumID();
  strMusicBrainzAlbumArtistID = tag.GetMusicBrainzAlbumArtistID();
  strMusicBrainzTRMID = tag.GetMusicBrainzTRMID();
  iYear = stTime.wYear;
  iTrack = tag.GetTrackAndDiskNumber();
  iDuration = tag.GetDuration();
  strThumb = "";
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
}

CSong::CSong()
{
  Clear();
}

void CSong::Clear()
{
  strFileName = "";
  strTitle = "";
  strArtist = "";
  strAlbum = "";
  strGenre = "";
  strThumb = "";
  strMusicBrainzTrackID = "";
  strMusicBrainzArtistID = "";
  strMusicBrainzAlbumID = "";
  strMusicBrainzAlbumArtistID = "";
  strMusicBrainzTRMID = "";
  iTrack = 0;
  iDuration = 0;
  iYear = 0;
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
}

CSongMap::CSongMap()
{
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
  std::map<CStdString, CSong>::iterator it = m_map.find(lower);
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