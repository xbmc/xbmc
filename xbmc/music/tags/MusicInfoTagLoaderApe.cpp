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

#include "MusicInfoTagLoaderApe.h"
#include "DllLibapetag.h"
#include "MusicInfoTag.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderApe::CMusicInfoTagLoaderApe(void)
{}

CMusicInfoTagLoaderApe::~CMusicInfoTagLoaderApe()
{}

bool CMusicInfoTagLoaderApe::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // retrieve the APE Tag info from strFileName
    // and put it in tag
    tag.SetURL(strFileName);
    CAPEv2Tag myTag;
    if (myTag.ReadTag((char*)strFileName.c_str())) // true to check ID3 tag as well
    {
      tag.SetTitle(myTag.GetTitle());
      tag.SetAlbum(myTag.GetAlbum());
      tag.SetArtist(myTag.GetArtist());
      tag.SetAlbumArtist(myTag.GetAlbumArtist());
      tag.SetGenre(myTag.GetGenre());
      tag.SetTrackNumber(myTag.GetTrackNum());
      tag.SetPartOfSet(myTag.GetDiscNum());
      tag.SetComment(myTag.GetComment());
      tag.SetLyrics(myTag.GetLyrics());
      tag.SetMusicBrainzAlbumArtistID(myTag.GetMusicBrainzAlbumArtistID());
      tag.SetMusicBrainzAlbumID(myTag.GetMusicBrainzAlbumID());
      tag.SetMusicBrainzArtistID(myTag.GetMusicBrainzArtistID());
      tag.SetMusicBrainzTrackID(myTag.GetMusicBrainzTrackID());
      tag.SetMusicBrainzTRMID(myTag.GetMusicBrainzTRMID());
      SYSTEMTIME dateTime;
      ZeroMemory(&dateTime, sizeof(SYSTEMTIME));
      dateTime.wYear = atoi(myTag.GetYear());
      tag.SetRating(myTag.GetRating());
      tag.SetReleaseDate(dateTime);
      tag.SetLoaded();
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader ape: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
