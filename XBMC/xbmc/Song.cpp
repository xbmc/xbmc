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