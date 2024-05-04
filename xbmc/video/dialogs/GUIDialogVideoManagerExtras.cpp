/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoManagerExtras.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogOK.h"
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
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoManagerTypes.h"

#include <algorithm>
#include <string>

static constexpr unsigned int CONTROL_BUTTON_ADD_EXTRAS = 23;
static constexpr unsigned int CONTROL_BUTTON_RENAME_EXTRA = 28;

CGUIDialogVideoManagerExtras::CGUIDialogVideoManagerExtras()
  : CGUIDialogVideoManager(WINDOW_DIALOG_MANAGE_VIDEO_EXTRAS)
{
}

VideoAssetType CGUIDialogVideoManagerExtras::GetVideoAssetType()
{
  return VideoAssetType::EXTRA;
}

bool CGUIDialogVideoManagerExtras::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      const int control{message.GetSenderId()};
      if (control == CONTROL_BUTTON_ADD_EXTRAS)
      {
        if (AddVideoExtra())
        {
          // refresh data and controls
          Refresh();
          UpdateControls();
          // @todo more detailed status to trigger an library list refresh or item update only when
          // needed. For example, library movie was converted into an extra.
          m_hasUpdatedItems = true;
        }
      }
      else if (control == CONTROL_BUTTON_RENAME_EXTRA)
      {
        Rename();
      }
      break;
    }
  }

  return CGUIDialogVideoManager::OnMessage(message);
}

void CGUIDialogVideoManagerExtras::UpdateButtons()
{
  CGUIDialogVideoManager::UpdateButtons();

  // Always enabled
  CONTROL_ENABLE(CONTROL_BUTTON_ADD_EXTRAS);

  // Conditional to empty list
  if (m_videoAssetsList->IsEmpty())
  {
    SET_CONTROL_FOCUS(CONTROL_BUTTON_ADD_EXTRAS, 0);
    CONTROL_DISABLE(CONTROL_BUTTON_RENAME_EXTRA);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BUTTON_RENAME_EXTRA);
  }
}

void CGUIDialogVideoManagerExtras::SetVideoAsset(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoManager::SetVideoAsset(item);

  if (!m_videoAssetsList->IsEmpty())
    SetSelectedVideoAsset(m_videoAssetsList->Get(0));
}

bool CGUIDialogVideoManagerExtras::AddVideoExtra()
{
  // @todo: combine with versions add file logic, structured similarly and sharing most logic.

  const MediaType mediaType{m_videoAsset->GetVideoInfoTag()->m_type};

  // prompt to choose a video file
  VECSOURCES sources{*CMediaSourceSettings::GetInstance().GetSources("files")};

  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  CServiceBroker::GetMediaManager().GetNetworkLocations(sources);
  AppendItemFolderToFileBrowserSources(sources);

  std::string path;
  if (CGUIDialogFileBrowser::ShowAndGetFile(
          sources, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
          g_localizeStrings.Get(40015), path))
  {
    const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
    const VideoDbContentType itemType = m_videoAsset->GetVideoContentType();

    const VideoAssetInfo newAsset{m_database.GetVideoVersionInfo(path)};

    std::string typeNewVideoVersion{
        CGUIDialogVideoManagerExtras::GenerateVideoExtra(URIUtils::GetFileName(path))};

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
            CVariant{40015},
            StringUtils::Format(g_localizeStrings.Get(msgid), newAsset.m_assetTypeName));
        return false;
      }

      // The video is an asset of another movie

      // The video is a version, ask for confirmation
      if (newAsset.m_assetType == VideoAssetType::VERSION &&
          !CGUIDialogYesNo::ShowAndGetInput(CVariant{40015},
                                            StringUtils::Format(g_localizeStrings.Get(40036))))
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
                CVariant{40015}, StringUtils::Format(g_localizeStrings.Get(msgid),
                                                     newAsset.m_assetTypeName, videoTitle)))
        {
          return false;
        }
      }

      // Additional constraints for the converion of a movie version
      if (newAsset.m_assetType == VideoAssetType::VERSION &&
          m_database.IsDefaultVideoVersion(newAsset.m_idFile))
      {
        CFileItemList list;
        m_database.GetVideoVersions(itemType, newAsset.m_idMedia, list, newAsset.m_assetType);

        if (list.Size() > 1)
        {
          // cannot add the default version of a movie with multiple versions to another movie
          CGUIDialogOK::ShowAndGetInput(CVariant{40015}, CVariant{40034});
          return false;
        }

        const int idNewVideoVersion{m_database.AddVideoVersionType(
            typeNewVideoVersion, VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA)};

        if (idNewVideoVersion == -1)
          return false;

        return m_database.ConvertVideoToVersion(itemType, newAsset.m_idMedia, dbId,
                                                idNewVideoVersion, VideoAssetType::EXTRA);
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

    const int idNewVideoVersion{m_database.AddVideoVersionType(
        typeNewVideoVersion, VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA)};

    if (idNewVideoVersion == -1)
      return false;

    m_database.AddVideoAsset(itemType, dbId, idNewVideoVersion, VideoAssetType::EXTRA, item);

    return true;
  }
  return false;
}

bool CGUIDialogVideoManagerExtras::ManageVideoExtras(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoManagerExtras* dialog{
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoManagerExtras>(
          WINDOW_DIALOG_MANAGE_VIDEO_EXTRAS)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_MANAGE_VIDEO_EXTRAS instance!");
    return false;
  }

  dialog->SetVideoAsset(item);
  dialog->Open();
  return dialog->HasUpdatedItems();
}

std::string CGUIDialogVideoManagerExtras::GenerateVideoExtra(const std::string& extrasRoot,
                                                             const std::string& extrasPath)
{
  // generate a video extra version string from its file path

  // remove the root path from its path
  const std::string extrasVersion{extrasPath.substr(extrasRoot.size())};

  return GenerateVideoExtra(extrasVersion);
}

std::string CGUIDialogVideoManagerExtras::GenerateVideoExtra(const std::string& extrasPath)
{
  // generate a video extra version string from its file path

  std::string extrasVersion{extrasPath};

  // remove file extension
  URIUtils::RemoveExtension(extrasVersion);

  // remove special characters
  extrasVersion = StringUtils::ReplaceSpecialCharactersWithSpace(extrasVersion);

  // trim the string
  return StringUtils::Trim(extrasVersion);
}
