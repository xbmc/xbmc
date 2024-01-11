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
#include "video/VideoManagerTypes.h"
#include "video/VideoThumbLoader.h"

using namespace VIDEO::GUILIB;

namespace
{
class CVideoChooser
{
public:
  explicit CVideoChooser(const std::shared_ptr<const CFileItem>& item) : m_item(item) {}
  virtual ~CVideoChooser() = default;

  void EnableExtras(bool enable) { m_enableExtras = enable; }

  std::shared_ptr<const CFileItem> ChooseVideo();

private:
  CVideoChooser() = delete;
  std::shared_ptr<const CFileItem> ChooseVideoVersion();
  std::shared_ptr<const CFileItem> ChooseVideoExtra();
  std::shared_ptr<const CFileItem> ChooseVideo(CGUIDialogSelect& dialog,
                                               int headingId,
                                               int buttonId,
                                               const CFileItemList& itemsToDisplay,
                                               const CFileItemList& itemsToSwitchTo);

  const std::shared_ptr<const CFileItem> m_item;
  bool m_enableExtras{false};
  bool m_switchType{false};
  CFileItemList m_videoVersions;
  CFileItemList m_videoExtras;
};

std::shared_ptr<const CFileItem> CVideoChooser::ChooseVideo()
{
  m_switchType = false;
  m_videoVersions.Clear();
  m_videoExtras.Clear();

  std::shared_ptr<const CFileItem> result;
  if (!m_item->HasVideoVersions())
    return result;

  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Unable to open video database!");
    return result;
  }

  db.GetAssetsForVideo(m_item->GetVideoContentType(), m_item->GetVideoInfoTag()->m_iDbId,
                       VideoAssetType::VERSION, m_videoVersions);

  if (m_enableExtras)
    db.GetAssetsForVideo(m_item->GetVideoContentType(), m_item->GetVideoInfoTag()->m_iDbId,
                         VideoAssetType::EXTRA, m_videoExtras);
  else
    m_videoExtras.Clear();

  // find default version item in list and select it
  for (const auto& item : m_videoVersions)
  {
    item->Select(item->GetVideoInfoTag()->IsDefaultVideoVersion());
  }

  VideoAssetType itemType{VideoAssetType::VERSION};
  while (true)
  {
    if (itemType == VideoAssetType::VERSION)
    {
      result = ChooseVideoVersion();
      itemType = VideoAssetType::EXTRA;
    }
    else
    {
      result = ChooseVideoExtra();
      itemType = VideoAssetType::VERSION;
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

  return ChooseVideo(*dialog, 40210 /* Versions */, 40211 /* Extras */, m_videoVersions,
                     m_videoExtras);
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

  return ChooseVideo(*dialog, 402011 /* Extras */, 40210 /* Versions */, m_videoExtras,
                     m_videoVersions);
}

std::shared_ptr<const CFileItem> CVideoChooser::ChooseVideo(CGUIDialogSelect& dialog,
                                                            int headingId,
                                                            int buttonId,
                                                            const CFileItemList& itemsToDisplay,
                                                            const CFileItemList& itemsToSwitchTo)
{
  CVideoThumbLoader thumbLoader;
  for (auto& item : itemsToDisplay)
  {
    thumbLoader.LoadItem(item.get());
    item->SetLabel2(item->GetVideoInfoTag()->m_strFileNameAndPath);
  }

  dialog.Reset();

  const std::string heading{
      StringUtils::Format(g_localizeStrings.Get(headingId), m_item->GetVideoInfoTag()->GetTitle())};
  dialog.SetHeading(heading);

  dialog.EnableButton(!itemsToSwitchTo.IsEmpty(), buttonId);
  dialog.SetUseDetails(true);
  dialog.SetMultiSelection(false);
  dialog.SetItems(itemsToDisplay);

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
    if (!item->GetProperty("needs_resolved_video_version").asBoolean(false))
    {
      // auto select the default video version
      const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
      if (settings->GetBool(CSettings::SETTING_MYVIDEOS_SELECTDEFAULTVERSION))
      {
        if (item->GetVideoInfoTag()->IsDefaultVideoVersion())
        {
          videoVersion = std::make_shared<const CFileItem>(*item);
        }
        else
        {
          CVideoDatabase db;
          if (!db.Open())
          {
            CLog::LogF(LOGERROR, "Unable to open video database!");
          }
          else
          {
            CFileItem defaultVersion;
            if (!db.GetDefaultVersionForVideo(item->GetVideoContentType(),
                                              item->GetVideoInfoTag()->m_iDbId, defaultVersion))
              CLog::LogF(LOGERROR, "Unable to get default version from video database!");
            else
              videoVersion = std::make_shared<const CFileItem>(defaultVersion);
          }
        }
      }
    }

    if (!videoVersion && (item->GetProperty("needs_resolved_video_version").asBoolean(false) ||
                          !item->GetProperty("has_resolved_video_version").asBoolean(false)))
    {
      CVideoChooser chooser{item};
      chooser.EnableExtras(false);
      const auto result{chooser.ChooseVideo()};
      if (result)
        videoVersion = result;
      else
        return {};
    }
  }

  if (videoVersion)
    return std::make_shared<CFileItem>(*videoVersion);

  return item;
}
