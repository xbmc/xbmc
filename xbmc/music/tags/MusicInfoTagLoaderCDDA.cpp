/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderCDDA.h"

#include "MusicInfoTag.h"
#include "ServiceBroker.h"
#include "network/cddb.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

#ifdef HAS_OPTICAL_DRIVE
using namespace MEDIA_DETECT;
using namespace CDDB;
#endif

//! @todo - remove after Ubuntu 16.04 (Xenial) is EOL
#if !defined(LIBCDIO_VERSION_NUM) || (LIBCDIO_VERSION_NUM <= 83)
#define CDTEXT_FIELD_TITLE CDTEXT_TITLE
#define CDTEXT_FIELD_PERFORMER CDTEXT_PERFORMER
#define CDTEXT_FIELD_GENRE CDTEXT_GENRE
#endif

CMusicInfoTagLoaderCDDA::CMusicInfoTagLoaderCDDA(void) = default;

CMusicInfoTagLoaderCDDA::~CMusicInfoTagLoaderCDDA() = default;

bool CMusicInfoTagLoaderCDDA::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
#ifdef HAS_OPTICAL_DRIVE
  try
  {
    tag.SetURL(strFileName);
    bool bResult = false;

    // Get information for the inserted disc
    CCdInfo* pCdInfo = CServiceBroker::GetMediaManager().GetCdInfo();
    if (pCdInfo == NULL)
      return bResult;

    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    // Prepare cddb
    Xcddb cddb;
    cddb.setCacheDir(profileManager->GetCDDBFolder());

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
        const std::string& strTitle = cddb.getTrackTitle(iTrack);
        if (!strTitle.empty())
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
          tag.SetReleaseDate(cddb.getYear());

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
      std::string strTitle = ti.cdtext[CDTEXT_FIELD_TITLE];
      if (!strTitle.empty())
      {
        // Tracknumber
        tag.SetTrackNumber(iTrack);

        // Title
        tag.SetTitle(strTitle);

        // Get info for track zero, as we may have and need CD-Text Album info
        xbmc_cdtext_t discCDText = pCdInfo->GetDiscCDTextInformation();

        // Artist: Use track artist or disc artist
        std::string strArtist = ti.cdtext[CDTEXT_FIELD_PERFORMER];
        if (strArtist.empty())
          strArtist = discCDText[CDTEXT_FIELD_PERFORMER];
        tag.SetArtist(strArtist);

        // Album
        std::string strAlbum;
        strAlbum = discCDText[CDTEXT_FIELD_TITLE];
        tag.SetAlbum(strAlbum);

        // Genre: use track or disc genre
        std::string strGenre = ti.cdtext[CDTEXT_FIELD_GENRE];
        if (strGenre.empty())
          strGenre = discCDText[CDTEXT_FIELD_GENRE];
        tag.SetGenre( strGenre );

        tag.SetLoaded(true);
        bResult = true;
      }
    }
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader CDDB: exception in file {}", strFileName);
  }

#endif

  tag.SetLoaded(false);

  return false;
}
