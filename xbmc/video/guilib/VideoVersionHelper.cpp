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
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoThumbLoader.h"

using namespace VIDEO::GUILIB;

namespace
{
class CVideoChooser
{
public:
  explicit CVideoChooser(const std::shared_ptr<const CFileItem>& item) : m_item(item) {}
  virtual ~CVideoChooser() = default;

  std::shared_ptr<const CFileItem> ChooseVideo();

private:
  CVideoChooser() = delete;
  std::shared_ptr<const CFileItem> ChooseVideoVersion();
  std::shared_ptr<const CFileItem> ChooseVideoExtra();
  std::shared_ptr<const CFileItem> ChooseVideo(CGUIDialogSelect& dialog,
                                               int headingId,
                                               int buttonId,
                                               VideoVersionItemType itemType);

  const std::shared_ptr<const CFileItem> m_item;
  bool m_switchType{false};
};

std::shared_ptr<const CFileItem> CVideoChooser::ChooseVideo()
{
  m_switchType = false;

  std::shared_ptr<const CFileItem> result;
  if (!m_item->HasVideoVersions())
    return result;

  VideoVersionItemType itemType{VideoVersionItemType::PRIMARY};
  while (true)
  {
    if (itemType == VideoVersionItemType::PRIMARY)
    {
      result = ChooseVideoVersion();
      itemType = VideoVersionItemType::EXTRAS;
    }
    else
    {
      result = ChooseVideoExtra();
      itemType = VideoVersionItemType::PRIMARY;
    }

    if (!m_switchType)
      break;

    // switch type button pressed. Re-open, this time with the "other" type to select.
  }
  return result;
}

std::shared_ptr<const CFileItem> CVideoChooser::ChooseVideoVersion()
{
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT_VIDEO_VERSION)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT_VIDEO_VERSION dialog instance!");
    return {};
  }

  return ChooseVideo(*dialog, 40210 /* Versions */, 40211 /* Extras */,
                     VideoVersionItemType::PRIMARY);
}

std::shared_ptr<const CFileItem> CVideoChooser::ChooseVideoExtra()
{
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT_VIDEO_EXTRA)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT_VIDEO_EXTRA dialog instance!");
    return {};
  }

  return ChooseVideo(*dialog, 40211 /* Extras */, 40210 /* Versions */,
                     VideoVersionItemType::EXTRAS);
}

std::shared_ptr<const CFileItem> CVideoChooser::ChooseVideo(CGUIDialogSelect& dialog,
                                                            int headingId,
                                                            int buttonId,
                                                            VideoVersionItemType itemType)
{
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Unable to open video database!");
    return {};
  }

  CFileItemList items;
  db.GetVideoVersions(m_item->GetVideoContentType(), m_item->GetVideoInfoTag()->m_iDbId, items,
                      itemType);

  CVideoThumbLoader thumbLoader;
  for (auto& item : items)
  {
    thumbLoader.LoadItem(item.get());
    item->SetLabel2(item->GetVideoInfoTag()->m_strFileNameAndPath);
  }

  dialog.Reset();

  const std::string heading{
      StringUtils::Format("{} : {}", g_localizeStrings.Get(headingId), m_item->GetLabel())};
  dialog.SetHeading(heading);

  dialog.EnableButton(true, buttonId);
  dialog.SetUseDetails(true);
  dialog.SetMultiSelection(false);
  dialog.SetItems(items);

  dialog.Open();

  m_switchType = dialog.IsButtonPressed();
  if (dialog.IsConfirmed())
    return dialog.GetSelectedFileItem();

  return {};
}
} // unnamed namespace

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
      const auto result{CVideoChooser(item).ChooseVideo()};
      if (result)
        videoVersion = result;
      else
        return {};
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
