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
}

