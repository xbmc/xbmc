/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoManagerVersions.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
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
static constexpr unsigned int CONTROL_BUTTON_RENAME_VERSION = 24;
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
        if (AddVideoVersion())
        {
          // refresh data and controls
          Refresh();
          UpdateControls();
          m_hasUpdatedItems = true;
        }
      }
      else if (control == CONTROL_BUTTON_RENAME_VERSION)
      {
        Rename();
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

void CGUIDialogVideoManagerVersions::Clear()
{
  m_defaultVideoVersion = std::make_shared<CFileItem>();
  CGUIDialogVideoManager::Clear();
}

void CGUIDialogVideoManagerVersions::UpdateButtons()
{
  CGUIDialogVideoManager::UpdateButtons();

  // Always enabled
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

  // Conditional to empty list
  if (m_videoAssetsList->IsEmpty())
  {
    SET_CONTROL_FOCUS(CONTROL_BUTTON_ADD_VERSION, 0);
    CONTROL_DISABLE(CONTROL_BUTTON_RENAME_VERSION);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BUTTON_RENAME_VERSION);
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

bool CGUIDialogVideoManagerVersions::AddVideoVersion()
{
  if (!m_videoAsset || !m_videoAsset->HasVideoInfoTag())
  {
    CLog::LogF(LOGERROR, "invalid video asset");
    return false;
  }

  CVideoDatabase videoDb;
  if (!videoDb.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database!");
    return false;
  }

  CFileItemList items;
  if (!GetSimilarMovies(m_videoAsset, items, videoDb))
    return false;

  if (items.Size() == 0)
  {
    // No button = browse library
    // Yes button = browse files
    // Custom button = cancel

    const int dlgResult{CGUIDialogYesNo::ShowAndGetInput(
        CVariant{40030}, CVariant{40032}, CVariant{40029}, CVariant{40028}, CVariant{222},
        CGUIDialogYesNo::NO_TIMEOUT)};

    switch (dlgResult)
    {
      case CGUIDialogYesNo::DIALOG_RESULT_CANCEL:
      case CGUIDialogYesNo::DIALOG_RESULT_CUSTOM:
        // Dialog dismissed or Cancel button
        return false;

      case CGUIDialogYesNo::DIALOG_RESULT_NO:
      {
        // Browse library
        if (!GetAllOtherMovies(m_videoAsset, items, videoDb))
          return false;

        const auto tag{m_videoAsset->GetVideoInfoTag()};

        return ChooseVideoAndConvertToVideoVersion(items, m_videoAsset->GetVideoContentType(),
                                                   tag->m_type, tag->m_iDbId, videoDb,
                                                   MediaRole::Parent);
      }

      case CGUIDialogYesNo::DIALOG_RESULT_YES:
        // Browse files
        return AddVideoVersionFilePicker();
    }

    CLog::LogF(LOGERROR, "Unknown return value {} from CGUIDialogYesNo", dlgResult);
    return false;
  }

  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};

  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT instance!");
    return false;
  }

  // Load thumbs async
  CVideoThumbLoader loader;
  loader.Load(items);

  dialog->Reset();
  dialog->SetItems(items);
  dialog->SetHeading(40030);
  dialog->SetUseDetails(true);
  dialog->EnableButton(true, 40028); // Browse files
  dialog->EnableButton2(true, 40029); // Browse library
  dialog->Open();

  if (loader.IsLoading())
    loader.StopThread();

  if (dialog->IsConfirmed())
  {
    // A similar movie was selected
    return AddSimilarMovieAsVersion(dialog->GetSelectedFileItem());
  }
  else if (dialog->IsButtonPressed())
  {
    // User wants to browse the files
    return AddVideoVersionFilePicker();
  }
  else if (dialog->IsButton2Pressed())
  {
    // User wants to browse the library
    if (!GetAllOtherMovies(m_videoAsset, items, videoDb))
      return false;

    const auto tag{m_videoAsset->GetVideoInfoTag()};

    return ChooseVideoAndConvertToVideoVersion(items, m_videoAsset->GetVideoContentType(),
                                               tag->m_type, tag->m_iDbId, videoDb,
                                               MediaRole::Parent);
  }
  return false;
}

bool CGUIDialogVideoManagerVersions::ManageVideoVersions(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoManagerVersions* dialog{
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoManagerVersions>(
          WINDOW_DIALOG_MANAGE_VIDEO_VERSIONS)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_MANAGE_VIDEO_VERSIONS instance!");
    return false;
  }

  dialog->SetVideoAsset(item);
  dialog->Open();
  return dialog->HasUpdatedItems();
}

bool CGUIDialogVideoManagerVersions::ChooseVideoAndConvertToVideoVersion(
    CFileItemList& items,
    VideoDbContentType itemType,
    const std::string& mediaType,
    int dbId,
    CVideoDatabase& videoDb,
    MediaRole role)
{
  if (items.Size() == 0)
  {
    CGUIDialogOK::ShowAndGetInput(role == MediaRole::NewVersion ? 40002 : 40030, 40031);
    return false;
  }

  // choose a video
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT instance!");
    return false;
  }

  // Load thumbs async
  CVideoThumbLoader loader;
  loader.Load(items);

  dialog->Reset();
  dialog->SetItems(items);
  dialog->SetHeading(role == MediaRole::NewVersion ? 40002 : 40030);
  dialog->SetUseDetails(true);
  dialog->Open();

  if (loader.IsLoading())
    loader.StopThread();

  if (!dialog->IsConfirmed())
    return false;

  const std::shared_ptr<CFileItem> selectedItem{dialog->GetSelectedFileItem()};
  if (!selectedItem)
    return false;

  CFileItemList list;
  videoDb.GetVideoVersions(itemType, selectedItem->GetVideoInfoTag()->m_iDbId, list,
                           VideoAssetType::VERSION);

  // ask confirmation for the addition of a movie with multiple versions to another movie
  if (list.Size() > 1 && !CGUIDialogYesNo::ShowAndGetInput(CVariant{40014}, CVariant{40037}))
  {
    return false;
  }

  // choose a video version for the video
  const int idVideoVersion{ChooseVideoAsset(selectedItem, VideoAssetType::VERSION, "")};
  if (idVideoVersion < 0)
    return false;

  int sourceDbId, targetDbId;
  switch (role)
  {
    case MediaRole::NewVersion:
      sourceDbId = dbId;
      targetDbId = selectedItem->GetVideoInfoTag()->m_iDbId;
      break;
    case MediaRole::Parent:
      sourceDbId = selectedItem->GetVideoInfoTag()->m_iDbId;
      targetDbId = dbId;
      break;
    default:
      return false;
  }

  return videoDb.ConvertVideoToVersion(itemType, sourceDbId, targetDbId, idVideoVersion,
                                       VideoAssetType::VERSION);
}

bool CGUIDialogVideoManagerVersions::GetAllOtherMovies(const std::shared_ptr<CFileItem>& item,
                                                       CFileItemList& list,
                                                       CVideoDatabase& videoDb)
{
  if (!item || !item->HasVideoInfoTag())
    return false;

  // get video list
  const std::string videoTitlesDir{StringUtils::Format(
      "videodb://{}/titles", CMediaTypes::ToPlural(item->GetVideoInfoTag()->m_type))};

  list.Clear();

  if (item->GetVideoContentType() == VideoDbContentType::MOVIES)
    videoDb.GetMoviesNav(videoTitlesDir, list);
  else
    return false;

  if (list.Size() < 2)
    return false;

  list.Sort(SortByLabel, SortOrderAscending,
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
                ? SortAttributeIgnoreArticle
                : SortAttributeNone);

  if (!PostProcessList(list, item->GetVideoInfoTag()->m_iDbId))
    return false;

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

  if (!PostProcessList(list, dbId))
    return false;

  return ChooseVideoAndConvertToVideoVersion(list, itemType, mediaType, dbId, videodb,
                                             MediaRole::NewVersion);
}

bool CGUIDialogVideoManagerVersions::AddVideoVersionFilePicker()
{
  // @todo: combine with extras add file logic, structured similarly and sharing most logic.

  const MediaType mediaType{m_videoAsset->GetVideoInfoTag()->m_type};

  // prompt to choose a video file
  VECSOURCES sources{*CMediaSourceSettings::GetInstance().GetSources("files")};

  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  CServiceBroker::GetMediaManager().GetNetworkLocations(sources);
  AppendItemFolderToFileBrowserSources(sources);

  std::string path;
  if (CGUIDialogFileBrowser::ShowAndGetFile(
          sources, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
          g_localizeStrings.Get(40014), path))
  {
    const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
    const VideoDbContentType itemType{m_videoAsset->GetVideoContentType()};

    const VideoAssetInfo newAsset{m_database.GetVideoVersionInfo(path)};

    // @todo look only for a version identified by idFile instead of retrieving all versions
    if (newAsset.m_idFile != -1 && newAsset.m_assetTypeId != -1)
    {
      // The video already is an asset of the movie
      if (newAsset.m_idMedia == dbId &&
          newAsset.m_mediaType == m_videoAsset->GetVideoInfoTag()->m_type)
      {
        unsigned int msgid{};

        if (newAsset.m_assetType == VideoAssetType::VERSION)
          msgid = 40016; // video is a version of the movie
        else if (newAsset.m_assetType == VideoAssetType::EXTRA)
          msgid = 40026; // video is an extra of the movie
        else
        {
          CLog::LogF(LOGERROR, "unexpected asset type {}", static_cast<int>(newAsset.m_assetType));
          return false;
        }

        CGUIDialogOK::ShowAndGetInput(
            CVariant{40014},
            StringUtils::Format(g_localizeStrings.Get(msgid), newAsset.m_assetTypeName));
        return false;
      }

      // The video is an asset of another movie

      // The video is an extra, ask for confirmation
      if (newAsset.m_assetType == VideoAssetType::EXTRA &&
          !CGUIDialogYesNo::ShowAndGetInput(CVariant{40014},
                                            StringUtils::Format(g_localizeStrings.Get(40035))))
      {
        return false;
      }

      std::string videoTitle;
      if (newAsset.m_mediaType == MediaTypeMovie)
      {
        videoTitle = m_database.GetMovieTitle(newAsset.m_idMedia);
      }
      else
        return false;

      {
        unsigned int msgid{};

        if (newAsset.m_assetType == VideoAssetType::VERSION)
          msgid = 40017; // video is a version of another movie
        else if (newAsset.m_assetType == VideoAssetType::EXTRA)
          msgid = 40027; // video is an extra of another movie
        else
        {
          CLog::LogF(LOGERROR, "unexpected asset type {}", static_cast<int>(newAsset.m_assetType));
          return false;
        }

        if (!CGUIDialogYesNo::ShowAndGetInput(
                CVariant{40014}, StringUtils::Format(g_localizeStrings.Get(msgid),
                                                     newAsset.m_assetTypeName, videoTitle)))
        {
          return false;
        }
      }

      // Additional constraints for the conversion of a movie version
      if (newAsset.m_assetType == VideoAssetType::VERSION &&
          m_database.IsDefaultVideoVersion(newAsset.m_idFile))
      {
        CFileItemList list;
        m_database.GetVideoVersions(itemType, newAsset.m_idMedia, list, newAsset.m_assetType);

        if (list.Size() > 1)
        {
          // cannot add the default version of a movie with multiple versions to another movie
          CGUIDialogOK::ShowAndGetInput(CVariant{40014}, CVariant{40038});
          return false;
        }

        const int idNewVideoVersion{ChooseVideoAsset(m_videoAsset, VideoAssetType::VERSION, "")};
        if (idNewVideoVersion != -1)
        {
          return m_database.ConvertVideoToVersion(itemType, newAsset.m_idMedia, dbId,
                                                  idNewVideoVersion, VideoAssetType::VERSION);
        }
        else
        {
          return false;
        }
      }
    }

    CFileItem item{path, false};

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
    {
      CDVDFileInfo::GetFileStreamDetails(&item);
      CLog::LogF(LOGDEBUG, "Extracted filestream details from video file {}",
                 CURL::GetRedacted(item.GetPath()));
    }

    const int idNewVideoVersion{ChooseVideoAsset(m_videoAsset, VideoAssetType::VERSION, "")};
    if (idNewVideoVersion == -1)
      return false;

    m_database.AddVideoAsset(itemType, dbId, idNewVideoVersion, VideoAssetType::VERSION, item);

    return true;
  }
  return false;
}

bool CGUIDialogVideoManagerVersions::GetSimilarMovies(const std::shared_ptr<CFileItem>& item,
                                                      CFileItemList& list,
                                                      CVideoDatabase& videoDb)
{
  list.Clear();

  videoDb.GetSameVideoItems(*item, list);

  if (list.Size() < 2)
  {
    list.Clear();
    return true;
  }

  if (!PostProcessList(list, item->GetVideoInfoTag()->m_iDbId))
    return false;

  return true;
}

bool CGUIDialogVideoManagerVersions::AddSimilarMovieAsVersion(
    const std::shared_ptr<CFileItem>& itemMovie)
{
  // A movie with versions cannot be turned into a version
  if (itemMovie->GetVideoInfoTag()->HasVideoVersions())
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{40005}, CVariant{40006});
    return false;
  }

  // choose a video version type for the video
  const int idVideoVersion{ChooseVideoAsset(itemMovie, VideoAssetType::VERSION, "")};
  if (idVideoVersion < 0)
    return false;

  CVideoDatabase videoDb;
  if (!videoDb.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database!");
    return false;
  }

  const int sourceDbId{itemMovie->GetVideoInfoTag()->m_iDbId};
  const int targetDbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
  return videoDb.ConvertVideoToVersion(VideoDbContentType::MOVIES, sourceDbId, targetDbId,
                                       idVideoVersion, VideoAssetType::VERSION);
}

bool CGUIDialogVideoManagerVersions::PostProcessList(CFileItemList& list, int dbId)
{
  // Exclude the provided dbId and decorate the items

  int i = 0;
  while (i < list.Size())
  {
    const auto item{list[i]};
    const auto itemtag{item->GetVideoInfoTag()};

    if (itemtag->m_iDbId == dbId)
    {
      list.Remove(i);
      // i is not incremented for the next iteration because the removal shifted what would have
      // been the next item into the current position.
      continue;
    }

    item->SetLabel2(itemtag->m_strFileNameAndPath);
    ++i;
  }

  return true;
}
