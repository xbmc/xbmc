/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoVersionHelper.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/dialogs/GUIDialogVideoVersion.h"

using namespace VIDEO::GUILIB;

std::shared_ptr<CFileItem> CVideoVersionHelper::ChooseMovieFromVideoVersions(
    const std::shared_ptr<CFileItem>& item)
{
  std::shared_ptr<const CFileItem> videoVersion;
  if (item->HasVideoVersions())
  {
    if (!item->GetProperty("force_choose_video_version").asBoolean(false))
    {
      // select the specified video version
      if (item->GetVideoInfoTag()->m_idVideoVersion > 0)
        videoVersion = item;

      if (!videoVersion)
      {
        // select the default video version
        const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
        if (settings->GetBool(CSettings::SETTING_MYVIDEOS_SELECTDEFAULTVERSION))
        {
          CVideoDatabase db;
          if (!db.Open())
          {
            CLog::LogF(LOGERROR, "Unable to open video database!");
          }
          else
          {
            CFileItem defaultVersion;
            db.GetDefaultVideoVersion(item->GetVideoContentType(), item->GetVideoInfoTag()->m_iDbId,
                                      defaultVersion);
            if (!defaultVersion.HasVideoInfoTag() || defaultVersion.GetVideoInfoTag()->IsEmpty())
              CLog::LogF(LOGERROR, "Unable to get default video version from video database!");
            else
              videoVersion = std::make_shared<const CFileItem>(defaultVersion);
          }
        }
      }
    }

    if (!videoVersion && (item->GetProperty("force_choose_video_version").asBoolean(false) ||
                          !item->GetProperty("prohibit_choose_video_version").asBoolean(false)))
    {
      const auto result{CGUIDialogVideoVersion::ChooseVideoVersion(item)};
      if (result.cancelled)
        return {};
      else
        videoVersion = result.selected;
    }
  }

  if (videoVersion)
    return GetMovieForVideoVersion(*videoVersion);

  return item;
}

std::shared_ptr<CFileItem> CVideoVersionHelper::GetMovieForVideoVersion(
    const CFileItem& videoVersion)
{
  auto item{std::make_shared<CFileItem>(videoVersion.GetDynPath(), false)};
  item->LoadDetails();
  return item;
}
