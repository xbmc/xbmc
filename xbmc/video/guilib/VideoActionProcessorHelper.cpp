/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoActionProcessorHelper.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/dialogs/GUIDialogVideoVersion.h"

using namespace VIDEO::GUILIB;

CVideoActionProcessorHelper::~CVideoActionProcessorHelper()
{
  RestoreDefaultVideoVersion();
}

void CVideoActionProcessorHelper::SetDefaultVideoVersion()
{
  RestoreDefaultVideoVersion();

  //! @todo this hack must go away! Playback currently only works if we persist the
  //! movie version to play in the video database temporarily, until playback was started.
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Unable to open video database!");
    return;
  }

  const VideoDbContentType itemType{m_item->GetVideoContentType()};
  const int dbId{m_item->GetVideoInfoTag()->m_iDbId};

  CFileItem defaultVideoVersion;
  db.GetDefaultVideoVersion(itemType, dbId, defaultVideoVersion);
  m_defaultVideoVersionFileId = defaultVideoVersion.GetVideoInfoTag()->m_iDbId;
  m_defaultVideoVersionDynPath = defaultVideoVersion.GetDynPath();

  db.SetDefaultVideoVersion(itemType, dbId, m_videoVersion->GetVideoInfoTag()->m_iDbId);

  m_item->SetDynPath(m_videoVersion->GetDynPath());
  db.GetDetailsByTypeAndId(*m_item, itemType, dbId);

  // notify all windows to update the file item
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, GUI_MSG_FLAG_FORCE_UPDATE, m_item);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CVideoActionProcessorHelper::RestoreDefaultVideoVersion()
{
  //! @todo this hack must go away!
  if (m_restoreFolderFlag)
  {
    m_restoreFolderFlag = false;
    m_item->m_bIsFolder = true;
  }

  if (m_defaultVideoVersionFileId == -1)
    return;

  //! @todo this hack must go away! Playback currently only works if we persist the
  //! movie version to play in the video database temporarily, until playback was started.
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Unable to open video database!");
    return;
  }

  const VideoDbContentType itemType{m_item->GetVideoContentType()};
  const int dbId{m_item->GetVideoInfoTag()->m_iDbId};

  db.SetDefaultVideoVersion(itemType, dbId, m_defaultVideoVersionFileId);

  m_item->SetDynPath(m_defaultVideoVersionDynPath);
  db.GetDetailsByTypeAndId(*m_item, itemType, dbId);

  m_defaultVideoVersionFileId = -1;
  m_defaultVideoVersionDynPath.clear();

  // notify all windows to update the file item
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, GUI_MSG_FLAG_FORCE_UPDATE, m_item);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

std::shared_ptr<CFileItem> CVideoActionProcessorHelper::ChooseVideoVersion()
{
  if (!m_videoVersion && m_item->HasVideoVersions())
  {
    if (!m_item->GetProperty("force_choose_video_version").asBoolean(false))
    {
      // select the specified video version
      if (m_item->GetVideoInfoTag()->m_idVideoVersion > 0)
        m_videoVersion = m_item;

      const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};

      if (!m_videoVersion)
      {
        //! @todo get rid of this hack to patch away item's folder flag if it is video version
        //! folder
        if (m_item->GetVideoInfoTag()->m_idVideoVersion == VIDEO_VERSION_ID_ALL &&
            settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER))
        {
          m_item->m_bIsFolder = false;
          m_restoreFolderFlag = true;
        }
      }

      if (!m_videoVersion)
      {
        // select the default video version
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
    SetDefaultVideoVersion();

  return m_item;
}
