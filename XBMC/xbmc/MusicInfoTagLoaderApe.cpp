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
#include "MusicInfoTagLoaderApe.h"
#include "cores/paplayer/DllMACDll.h"

// MPC stuff
#include "Util.h"
// MPC stuff


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
    //bool bResult = false;
    tag.SetURL(strFileName);
    CAPEv2Tag myTag;
    if (myTag.ReadTag(strFileName.c_str(), true)) // true to check ID3 tag as well
    {
      tag.SetTitle(myTag.GetTitle());
      tag.SetAlbum(myTag.GetAlbum());
      tag.SetArtist(myTag.GetArtist());
      tag.SetAlbumArtist(myTag.GetAlbumArtist());
      tag.SetGenre(myTag.GetGenre());
      tag.SetTrackNumber(myTag.GetTrackNum());
      tag.SetPartOfSet(myTag.GetDiscNum());
      tag.SetComment(myTag.GetComment());
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
      // Find duration - we must read the info from the ape file for this
      tag.SetDuration(ReadDuration(strFileName));
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

int CMusicInfoTagLoaderApe::ReadDuration(const CStdString &strFileName)
{
  // load the ape dll if we need it
  DllMACDll dll;
  if (!dll.Load())
    return 0;

  return (int)(dll.GetDuration(strFileName.c_str()) / 1000);
}

