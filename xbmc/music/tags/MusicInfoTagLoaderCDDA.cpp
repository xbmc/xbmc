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

#include "system.h" // for HAS_DVD_DRIVE
#include "MusicInfoTagLoaderCDDA.h"
#include "network/cddb.h"
#include "MusicInfoTag.h"
#include "profiles/ProfilesManager.h"
#include "storage/MediaManager.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
using namespace CDDB;
#endif

#if defined (LIBCDIO_VERSION_NUM) && (LIBCDIO_VERSION_NUM > 83)
#define CDTEXT_TITLE CDTEXT_FIELD_TITLE
#define CDTEXT_PERFORMER CDTEXT_FIELD_PERFORMER
#define CDTEXT_GENRE CDTEXT_FIELD_GENRE
#endif

CMusicInfoTagLoaderCDDA::CMusicInfoTagLoaderCDDA(void)
{
}

CMusicInfoTagLoaderCDDA::~CMusicInfoTagLoaderCDDA()
{
}

bool CMusicInfoTagLoaderCDDA::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
#ifdef HAS_DVD_DRIVE
  try
  {
    tag.SetURL(strFileName);
    bool bResult = false;

    // Get information for the inserted disc
    CCdInfo* pCdInfo = g_mediaManager.GetCdInfo();
    if (pCdInfo == NULL)
      return bResult;

    // Prepare cddb
    Xcddb cddb;
    cddb.setCacheDir(CProfilesManager::Get().GetCDDBFolder());

    int iTrack = atoi(strFileName.substr(13, strFileName.size() - 13 - 5).c_str());

    // duration is always available
    tag.SetDuration( ( pCdInfo->GetTrackInformation(iTrack).nMins * 60 )
                     + pCdInfo->GetTrackInformation(iTrack).nSecs );

    // Only load cached cddb info in this tag loader, the internet database query is made in CCDDADirectory
    if (pCdInfo->HasCDDBInfo() && cddb.isCDCached(pCdInfo))
    {
      // get cddb information
      if (cddb.queryCDinfo(pCdInfo))
      {
        // Fill the fileitems music tag with cddb information, if available
        std::string strTitle = cddb.getTrackTitle(iTrack);
        if (strTitle.size() > 0)
        {
          // Tracknumber
          tag.SetTrackNumber(iTrack);

          // Title
          tag.SetTitle(strTitle);

          // Artist: Use track artist or disc artist
          std::string strArtist = cddb.getTrackArtist(iTrack);
          if (strArtist.empty())
            cddb.getDiskArtist(strArtist);
          tag.SetArtist(strArtist);

          // Album
          std::string strAlbum;
          cddb.getDiskTitle( strAlbum );
          tag.SetAlbum(strAlbum);

          // Album Artist
          std::string strAlbumArtist;
          cddb.getDiskArtist(strAlbumArtist);
          tag.SetAlbumArtist(strAlbumArtist);

          // Year
          SYSTEMTIME dateTime;
          dateTime.wYear = atoi(cddb.getYear().c_str());
          tag.SetReleaseDate( dateTime );

          // Genre
          tag.SetGenre( cddb.getGenre() );

          tag.SetLoaded(true);
          bResult = true;
        }
      }
    }
    else
    {
      // No cddb info, maybe we have CD-Text
      trackinfo ti = pCdInfo->GetTrackInformation(iTrack);

      // Fill the fileitems music tag with CD-Text information, if available
      std::string strTitle = ti.cdtext[CDTEXT_TITLE];
      if (strTitle.size() > 0)
      {
        // Tracknumber
        tag.SetTrackNumber(iTrack);

        // Title
        tag.SetTitle(strTitle);

        // Get info for track zero, as we may have and need CD-Text Album info
        xbmc_cdtext_t discCDText = pCdInfo->GetDiscCDTextInformation();

        // Artist: Use track artist or disc artist
        std::string strArtist = ti.cdtext[CDTEXT_PERFORMER];
        if (strArtist.empty())
          strArtist = discCDText[CDTEXT_PERFORMER];
        tag.SetArtist(strArtist);

        // Album
        std::string strAlbum;
        strAlbum = discCDText[CDTEXT_TITLE];
        tag.SetAlbum(strAlbum);

        // Genre: use track or disc genre
        std::string strGenre = ti.cdtext[CDTEXT_GENRE];
        if (strGenre.empty())
          strGenre = discCDText[CDTEXT_GENRE];
        tag.SetGenre( strGenre );

        tag.SetLoaded(true);
        bResult = true;
      }
    }
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader CDDB: exception in file %s", strFileName.c_str());
  }

#endif

  tag.SetLoaded(false);

  return false;
}
