/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoManagerVersions.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoThumbLoader.h"

#include <algorithm>
#include <string>

static constexpr unsigned int CONTROL_BUTTON_ADD_VERSION = 22;
static constexpr unsigned int CONTROL_BUTTON_SET_DEFAULT = 25;

CGUIDialogVideoManagerVersions::CGUIDialogVideoManagerVersions()
  : CGUIDialogVideoManager(WINDOW_DIALOG_MANAGE_VIDEO_VERSIONS),
    m_defaultVideoVersion(std::make_shared<CFileItem>())
{
}

VideoAssetType CGUIDialogVideoManagerVersions::GetVideoAssetType()
{
  return VideoAssetType::VERSION;
}

bool CGUIDialogVideoManagerVersions::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      const int control{message.GetSenderId()};
      if (control == CONTROL_BUTTON_ADD_VERSION)
      {
        AddVideoVersion();
      }
      else if (control == CONTROL_BUTTON_SET_DEFAULT)
      {
        SetDefault();
      }
      break;
    }
  }

  return CGUIDialogVideoManager::OnMessage(message);
}

void CGUIDialogVideoManagerVersions::UpdateButtons()
{
  CGUIDialogVideoManager::UpdateButtons();

  // Always anabled
  CONTROL_ENABLE(CONTROL_BUTTON_ADD_VERSION);

  // Enabled for non-default version only
  if (m_selectedVideoAsset->GetVideoInfoTag()->m_iDbId ==
      m_defaultVideoVersion->GetVideoInfoTag()->m_iDbId)
  {
    DisableRemove();
    CONTROL_DISABLE(CONTROL_BUTTON_SET_DEFAULT);
  }
  else
  {
    EnableRemove();
    CONTROL_ENABLE(CONTROL_BUTTON_SET_DEFAULT);
  }

  // Enabled if list not empty
  if (m_videoAssetsList->IsEmpty())
  {
    SET_CONTROL_FOCUS(CONTROL_BUTTON_ADD_VERSION, 0);
  }
}

void CGUIDialogVideoManagerVersions::UpdateDefaultVideoVersionSelection()
{
  // find new item in list and select it
  const int defaultDbId{m_defaultVideoVersion->GetVideoInfoTag()->m_iDbId};
  for (const auto& item : *m_videoAssetsList)
  {
    item->Select(item->GetVideoInfoTag()->m_iDbId == defaultDbId);
  }
}

void CGUIDialogVideoManagerVersions::Refresh()
{
  CGUIDialogVideoManager::Refresh();

  // update default video version
  const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
  const VideoDbContentType itemType{m_videoAsset->GetVideoContentType()};
  m_database.GetDefaultVideoVersion(itemType, dbId, *m_defaultVideoVersion);

  UpdateDefaultVideoVersionSelection();
}

void CGUIDialogVideoManagerVersions::SetVideoAsset(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoManager::SetVideoAsset(item);

  SetSelectedVideoAsset(m_defaultVideoVersion);
}

void CGUIDialogVideoManagerVersions::Remove()
{
  const MediaType mediaType{m_videoAsset->GetVideoInfoTag()->m_type};

  // default video version is not allowed
  if (m_database.IsDefaultVideoVersion(m_selectedVideoAsset->GetVideoInfoTag()->m_iDbId))
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{40018}, CVariant{40019});
    return;
  }

  CGUIDialogVideoManager::Remove();
}

void CGUIDialogVideoManagerVersions::SetDefault()
{
  // set the selected video version as default
  SetDefaultVideoVersion(*m_selectedVideoAsset);

  const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
  const VideoDbContentType itemType{m_videoAsset->GetVideoContentType()};

  // update our default video version
  m_database.GetDefaultVideoVersion(itemType, dbId, *m_defaultVideoVersion);

  UpdateControls();
  UpdateDefaultVideoVersionSelection();
}

void CGUIDialogVideoManagerVersions::SetDefaultVideoVersion(const CFileItem& version)
{
  const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
  const VideoDbContentType itemType{m_videoAsset->GetVideoContentType()};

  // set the specified video version as default
  m_database.SetDefaultVideoVersion(itemType, dbId, version.GetVideoInfoTag()->m_iDbId);

  // update the video item
  m_videoAsset->SetPath(version.GetPath());
  m_videoAsset->SetDynPath(version.GetPath());

  // update video details since we changed the video file for the item
  m_database.GetDetailsByTypeAndId(*m_videoAsset, itemType, dbId);

  // notify all windows to update the file item
  CGUIMessage msg{GUI_MSG_NOTIFY_ALL,        0,           0, GUI_MSG_UPDATE_ITEM,
                  GUI_MSG_FLAG_FORCE_UPDATE, m_videoAsset};
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CGUIDialogVideoManagerVersions::AddVideoVersion()
{
  const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
  const MediaType mediaType{m_videoAsset->GetVideoInfoTag()->m_type};
  const VideoDbContentType itemType{m_videoAsset->GetVideoContentType()};

  // prompt to choose a video file
  VECSOURCES sources{*CMediaSourceSettings::GetInstance().GetSources("files")};

  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  CServiceBroker::GetMediaManager().GetNetworkLocations(sources);

  std::string path;
  if (CGUIDialogFileBrowser::ShowAndGetFile(
          sources, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
          g_localizeStrings.Get(40014), path))
  {
    std::string typeVideoVersion;
    std::string videoTitle;
    int idFile{-1};
    int idMedia{-1};
    MediaType itemMediaType;
    VideoAssetType videoAssetType{VideoAssetType::UNKNOWN};

    const int idVideoVersion{m_database.GetVideoVersionInfo(path, idFile, typeVideoVersion, idMedia,
                                                            itemMediaType, videoAssetType)};

    if (idVideoVersion != -1)
    {
      CFileItemList versions;
      m_database.GetVideoVersions(itemType, dbId, versions);
      if (std::any_of(versions.begin(), versions.end(),
                      [idFile](const std::shared_ptr<CFileItem>& version)
                      { return version->GetVideoInfoTag()->m_iDbId == idFile; }))
      {
        CGUIDialogOK::ShowAndGetInput(
            CVariant{40014}, StringUtils::Format(g_localizeStrings.Get(40016), typeVideoVersion));
        return;
      }

      if (itemMediaType == MediaTypeMovie)
      {
        videoTitle = m_database.GetMovieTitle(idMedia);
      }
      else
        return;

      if (!CGUIDialogYesNo::ShowAndGetInput(
              CVariant{40014},
              StringUtils::Format(g_localizeStrings.Get(40017), typeVideoVersion, videoTitle)))
      {
        return;
      }

      if (m_database.IsDefaultVideoVersion(idFile))
      {
        CFileItemList list;
        m_database.GetVideoVersions(itemType, idMedia, list);

        if (list.Size() > 1)
        {
          CGUIDialogOK::ShowAndGetInput(CVariant{40014}, CVariant{40019});
          return;
        }
        else
        {
          if (itemMediaType == MediaTypeMovie)
          {
            m_database.DeleteMovie(idMedia);
          }
          else
            return;
        }
      }
      else
        m_database.RemoveVideoVersion(idFile);
    }

    CFileItem item{path, false};

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
    {
      CDVDFileInfo::GetFileStreamDetails(&item);
      CLog::LogF(LOGDEBUG, "Extracted filestream details from video file {}",
                 CURL::GetRedacted(item.GetPath()));
    }

    const int idNewVideoVersion{ChooseVideoAsset(m_videoAsset, VideoAssetType::VERSION)};
    if (idNewVideoVersion != -1)
      m_database.AddPrimaryVideoVersion(itemType, dbId, idNewVideoVersion, item);

    // refresh data and controls
    Refresh();
    UpdateControls();
  }
}

std::tuple<int, std::string> CGUIDialogVideoManagerVersions::NewVideoVersion()
{
  std::string typeVideoVersion;

  // prompt for the new video version
  if (!CGUIKeyboardFactory::ShowAndGetInput(typeVideoVersion, CVariant{40004}, false))
    return std::make_tuple(-1, "");

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database!");
    return std::make_tuple(-1, "");
  }

  typeVideoVersion = StringUtils::Trim(typeVideoVersion);
  const int idVideoVersion{videodb.AddVideoVersionType(typeVideoVersion, VideoAssetTypeOwner::USER,
                                                       VideoAssetType::VERSION)};

  return std::make_tuple(idVideoVersion, typeVideoVersion);
}

void CGUIDialogVideoManagerVersions::ManageVideoVersions(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoManagerVersions* dialog{
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoManagerVersions>(
          WINDOW_DIALOG_MANAGE_VIDEO_VERSIONS)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_MANAGE_VIDEO_VERSIONS instance!");
    return;
  }

  dialog->SetVideoAsset(item);
  dialog->Open();
}

int CGUIDialogVideoManagerVersions::ManageVideoVersionContextMenu(
    const std::shared_ptr<CFileItem>& version)
{
  CContextButtons buttons;

  buttons.Add(CONTEXT_BUTTON_RENAME, 118);
  buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 40023);
  buttons.Add(CONTEXT_BUTTON_DELETE, 15015);
  buttons.Add(CONTEXT_BUTTON_SET_ART, 13511);

  const int button{CGUIDialogContextMenu::ShowAndGetChoice(buttons)};
  if (button > 0)
  {
    CGUIDialogVideoManagerVersions* dialog{
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoManagerVersions>(
            WINDOW_DIALOG_MANAGE_VIDEO_VERSIONS)};
    if (!dialog)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_MANAGE_VIDEO_VERSIONS instance!");
      return -1;
    }

    CFileItem videoItem;
    if (!dialog->m_database.GetVideoItemByVideoVersion(version->GetVideoInfoTag()->m_iDbId,
                                                       videoItem))
      return -1;

    dialog->SetVideoAsset(std::make_shared<CFileItem>(videoItem));
    dialog->SetSelectedVideoAsset(version);

    switch (static_cast<CONTEXT_BUTTON>(button))
    {
      case CONTEXT_BUTTON_RENAME:
      {
        dialog->Rename();
        break;
      }
      case CONTEXT_BUTTON_SET_DEFAULT:
      {
        dialog->SetDefault();
        break;
      }
      case CONTEXT_BUTTON_DELETE:
      {
        dialog->Remove();
        break;
      }
      case CONTEXT_BUTTON_SET_ART:
      {
        dialog->ChooseArt();
        break;
      }
      default:
        break;
    }
  }

  return button;
}

bool CGUIDialogVideoManagerVersions::ChooseVideoAndConvertToVideoVersion(
    CFileItemList& items,
    VideoDbContentType itemType,
    const std::string& mediaType,
    int dbId,
    CVideoDatabase& videoDb)
{
  // choose a video
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT instance!");
    return {};
  }

  // Load thumbs async
  CVideoThumbLoader loader;
  loader.Load(items);

  dialog->Reset();
  dialog->SetItems(items);
  dialog->SetHeading(CVariant{40002});
  dialog->SetUseDetails(true);
  dialog->Open();

  if (loader.IsLoading())
    loader.StopThread();

  if (!dialog->IsConfirmed())
    return false;

  const std::shared_ptr<CFileItem> selectedItem{dialog->GetSelectedFileItem()};
  if (!selectedItem)
    return false;

  // choose a video version for the video
  const int idVideoVersion{ChooseVideoAsset(selectedItem, VideoAssetType::VERSION)};
  if (idVideoVersion < 0)
    return false;

  videoDb.ConvertVideoToVersion(itemType, dbId, selectedItem->GetVideoInfoTag()->m_iDbId,
                                idVideoVersion);
  return true;
}

bool CGUIDialogVideoManagerVersions::ProcessVideoVersion(VideoDbContentType itemType, int dbId)
{
  if (itemType != VideoDbContentType::MOVIES)
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database!");
    return false;
  }

  CFileItem item;
  if (!videodb.GetDetailsByTypeAndId(item, itemType, dbId))
    return false;

  CFileItemList list;
  videodb.GetSameVideoItems(item, list);

  if (list.Size() < 2)
    return false;

  const MediaType mediaType{item.GetVideoInfoTag()->m_type};

  std::string path;
  videodb.GetFilePathById(dbId, path, itemType);

  if (!CGUIDialogYesNo::ShowAndGetInput(
          CVariant{40008}, StringUtils::Format(g_localizeStrings.Get(40009),
                                               item.GetVideoInfoTag()->GetTitle(), path)))
  {
    return false;
  }

  for (int i = 0; i < list.Size(); ++i)
  {
    if (dbId == list[i]->GetVideoInfoTag()->m_iDbId)
    {
      list.Remove(i);
      break;
    }
  }

  // decorate the items
  for (const auto& item : list)
  {
    item->SetLabel2(item->GetVideoInfoTag()->m_strFileNameAndPath);
  }

  return ChooseVideoAndConvertToVideoVersion(list, itemType, mediaType, dbId, videodb);
}
