/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerExtras.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "InfoScanner.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/Scraper.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "filesystem/Directory.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScannerArt.h"
#include "video/VideoManagerTypes.h"
#include "video/dialogs/GUIDialogVideoManagerExtras.h"

#include <memory>

using namespace XFILE;

namespace KODI::VIDEO
{
CVideoInfoScannerExtras::CVideoInfoScannerExtras(CVideoDatabase& database,
                                                 const CVideoInfoScannerArt& art)
  : m_database(database),
    m_art(art)
{
}

bool CVideoInfoScannerExtras::AddVideoExtras(const CFileItemList& items, const std::string& path)
{
  int dbId = -1;

  // get the movie which was added previously
  for (const auto& item : items)
  {
    if (!item->IsFolder())
    {
      dbId = m_database.GetMovieId(item->GetPath());
      if (dbId != -1)
      {
        break;
      }
    }
  }

  if (dbId == -1)
  {
    CLog::Log(LOGERROR, "VideoInfoScanner: Failed to find the library item for video extras {}",
              CURL::GetRedacted(path));
    return false;
  }

  AddVideoExtras(dbId, path);
  return true;
}

void CVideoInfoScannerExtras::AddVideoExtras(int dbId, const std::string& path)
{
  // No need to check for .nomedia in the current directory, the caller already checked and this
  // function would not have been called if it existed.

  // Add video extras to library
  CDirectory::EnumerateDirectory(
      path,
      [this, dbId, path](const std::shared_ptr<CFileItem>& item)
      {
        const std::string extraTypeName =
            CGUIDialogVideoManagerExtras::GenerateVideoExtra(path, item->GetPath());

        const int idVideoAssetType = m_database.AddVideoVersionType(
            extraTypeName, VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA);

        // the video may have been added to the library as a movie earlier (different settings)
        const int idMovie{m_database.GetMovieId(item->GetPath())};

        if (idMovie <= 0)
        {
          if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                  CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
          {
            CDVDFileInfo::GetFileStreamDetails(item.get());
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Extracted filestream details from video file {}",
                      CURL::GetRedacted(item->GetPath()));
          }

          m_art.GetArtwork(item.get(), ADDON::ContentType::MOVIES, true, true, "");

          if (m_database.AddVideoAsset(VideoDbContentType::MOVIES, dbId, idVideoAssetType,
                                       VideoAssetType::EXTRA, *item.get()))
          {
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Added video extra {}",
                      CURL::GetRedacted(item->GetPath()));
          }
          else
          {
            CLog::Log(LOGERROR, "VideoInfoScanner: Failed to add video extra {}",
                      CURL::GetRedacted(item->GetPath()));
          }
        }
        else
        {
          m_database.ConvertVideoToVersion(VideoDbContentType::MOVIES, idMovie, dbId,
                                           idVideoAssetType, VideoAssetType::EXTRA);
        }
      },
      [](const std::shared_ptr<CFileItem>& dirItem)
      { return !CInfoScanner::HasNoMedia(dirItem->GetPath()); }, true,
      CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(), DIR_FLAG_DEFAULTS);
}
} // namespace KODI::VIDEO
