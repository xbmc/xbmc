/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoVersionHelper.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
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
#include "video/VideoFileItemClassify.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoThumbLoader.h"

namespace KODI::VIDEO::GUILIB
{

namespace
{
class CVideoChooser
{
public:
  explicit CVideoChooser(const std::shared_ptr<const CFileItem>& item) : m_item(item) {}
  virtual ~CVideoChooser() = default;

  void EnableTypeSwitch(bool enable) { m_enableTypeSwitch = enable; }
  void SetInitialAssetType(VideoAssetType type) { m_initialAssetType = type; }

  std::shared_ptr<const CFileItem> ChooseVideo();

private:
  CVideoChooser() = delete;
  std::shared_ptr<const CFileItem> ChooseVideoVersion();
  std::shared_ptr<const CFileItem> ChooseVideoExtra();
  std::shared_ptr<const CFileItem> ChooseVideo(CGUIDialogSelect& dialog,
                                               int headingId,
                                               int buttonId,
                                               CFileItemList& itemsToDisplay,
                                               const CFileItemList& itemsToSwitchTo);

  const std::shared_ptr<const CFileItem> m_item;
  bool m_enableTypeSwitch{false};
  VideoAssetType m_initialAssetType{VideoAssetType::UNKNOWN};
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
  if (m_enableTypeSwitch && !m_item->HasVideoVersions() && !m_item->HasVideoExtras())
    return result;

  if (!m_enableTypeSwitch && m_initialAssetType == VideoAssetType::VERSION &&
      !m_item->HasVideoVersions())
    return result;

  if (!m_enableTypeSwitch && m_initialAssetType == VideoAssetType::EXTRA &&
      !m_item->HasVideoExtras())
    return result;

  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Unable to open video database!");
    return result;
  }

  if (m_initialAssetType == VideoAssetType::VERSION || m_enableTypeSwitch)
  {
    db.GetAssetsForVideo(m_item->GetVideoContentType(), m_item->GetVideoInfoTag()->m_iDbId,
                         VideoAssetType::VERSION, m_videoVersions);

    // find default version item in list and select it
    for (const auto& item : m_videoVersions)
    {
      item->Select(item->GetVideoInfoTag()->IsDefaultVideoVersion());
    }
  }

  if (m_initialAssetType == VideoAssetType::EXTRA || m_enableTypeSwitch)
    db.GetAssetsForVideo(m_item->GetVideoContentType(), m_item->GetVideoInfoTag()->m_iDbId,
                         VideoAssetType::EXTRA, m_videoExtras);

  VideoAssetType itemType{m_initialAssetType};
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

  return ChooseVideo(*dialog, 40208 /* Choose version */, 40211 /* Extras */, m_videoVersions,
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

  return ChooseVideo(*dialog, 40214 /* Choose extra */, 40210 /* Versions */, m_videoExtras,
                     m_videoVersions);
}

std::shared_ptr<const CFileItem> CVideoChooser::ChooseVideo(CGUIDialogSelect& dialog,
                                                            int headingId,
                                                            int buttonId,
                                                            CFileItemList& itemsToDisplay,
                                                            const CFileItemList& itemsToSwitchTo)
{
  CVideoThumbLoader thumbLoader;
  thumbLoader.Load(itemsToDisplay);
  for (auto& item : itemsToDisplay)
    item->SetLabel2(item->GetVideoInfoTag()->m_strFileNameAndPath);

  dialog.Reset();

  const std::string heading{
      StringUtils::Format(g_localizeStrings.Get(headingId), m_item->GetVideoInfoTag()->GetTitle())};
  dialog.SetHeading(heading);

  dialog.EnableButton(m_enableTypeSwitch && !itemsToSwitchTo.IsEmpty(), buttonId);
  dialog.SetUseDetails(true);
  dialog.SetMultiSelection(false);
  dialog.SetItems(itemsToDisplay);

  dialog.Open();

  if (thumbLoader.IsLoading())
    thumbLoader.StopThread();

  m_switchType = dialog.IsButtonPressed();
  if (dialog.IsConfirmed())
    return dialog.GetSelectedFileItem();

  return {};
}
} // unnamed namespace

std::shared_ptr<CFileItem> CVideoVersionHelper::ChooseVideoFromAssets(
    const std::shared_ptr<CFileItem>& item)
{
  std::shared_ptr<const CFileItem> video;

  VideoAssetType assetType{static_cast<int>(
      item->GetProperty("video_asset_type").asInteger(static_cast<int>(VideoAssetType::UNKNOWN)))};
  bool allAssetTypes{false};
  bool hasMultipleChoices{false};

  switch (assetType)
  {
    case VideoAssetType::UNKNOWN:
      // asset type not provided means all types are allowed and the user can switch between types
      allAssetTypes = true;
      if (item->HasVideoVersions() || item->HasVideoExtras())
        hasMultipleChoices = true;
      break;

    case VideoAssetType::VERSION:
      if (item->HasVideoVersions())
        hasMultipleChoices = true;
      break;

    case VideoAssetType::EXTRA:
      if (item->HasVideoExtras())
        hasMultipleChoices = true;
      break;

    default:
      CLog::LogF(LOGERROR, "unknown asset type ({})", static_cast<int>(assetType));
      return {};
  }

  if (hasMultipleChoices)
  {
    if (!item->GetProperty("needs_resolved_video_asset").asBoolean(false))
    {
      // auto select the default video version
      const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
      if (settings->GetBool(CSettings::SETTING_MYVIDEOS_SELECTDEFAULTVERSION))
      {
        if (item->GetVideoInfoTag()->IsDefaultVideoVersion())
        {
          video = std::make_shared<const CFileItem>(*item);
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
              video = std::make_shared<const CFileItem>(defaultVersion);
          }
        }
      }
    }

    if (!video && (item->GetProperty("needs_resolved_video_asset").asBoolean(false) ||
                   !item->GetProperty("has_resolved_video_asset").asBoolean(false)))
    {
      CVideoChooser chooser{item};

      if (allAssetTypes)
      {
        chooser.EnableTypeSwitch(true);
        chooser.SetInitialAssetType(VideoAssetType::VERSION);
      }
      else
      {
        chooser.EnableTypeSwitch(false);
        chooser.SetInitialAssetType(assetType);
      }

      const auto result{chooser.ChooseVideo()};
      if (result)
        video = result;
      else
        return {};
    }
  }

  if (video)
    return std::make_shared<CFileItem>(*video);

  return item;
}

} // namespace KODI::VIDEO::GUILIB
