/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoActionProcessorHelper.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/dialogs/GUIDialogVideoVersion.h"

using namespace VIDEO::GUILIB;

std::shared_ptr<CFileItem> CVideoActionProcessorHelper::ChooseVideoVersion()
{
  if (!m_videoVersion && m_item->HasVideoVersions())
  {
    if (!m_item->GetProperty("force_choose_video_version").asBoolean(false))
    {
      // select the specified video version
      if (m_item->GetVideoInfoTag()->m_idVideoVersion > 0)
        m_videoVersion = m_item;

      if (!m_videoVersion)
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
            db.GetDefaultVideoVersion(m_item->GetVideoContentType(),
                                      m_item->GetVideoInfoTag()->m_iDbId, defaultVersion);
            if (!defaultVersion.HasVideoInfoTag() || defaultVersion.GetVideoInfoTag()->IsEmpty())
              CLog::LogF(LOGERROR, "Unable to get default video version from video database!");
            else
              m_videoVersion = std::make_shared<const CFileItem>(defaultVersion);
          }
        }
      }
    }

    if (!m_videoVersion && (m_item->GetProperty("force_choose_video_version").asBoolean(false) ||
                            !m_item->GetProperty("prohibit_choose_video_version").asBoolean(false)))
    {
      const auto result{CGUIDialogVideoVersion::ChooseVideoVersion(m_item)};
      if (result.cancelled)
        return {};
      else
        m_videoVersion = result.selected;
    }
  }

  if (m_videoVersion)
  {
    m_item = std::make_shared<CFileItem>(m_videoVersion->GetDynPath(), false);
    m_item->LoadDetails();
  }

  return m_item;
}
