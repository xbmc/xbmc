/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoManagerExtras.h"

#include "FileItem.h"
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

CGUIDialogVideoManagerExtras::CGUIDialogVideoManagerExtras()
  : CGUIDialogVideoManager(WINDOW_DIALOG_MANAGE_VIDEO_EXTRAS)
{
}

VideoAssetType CGUIDialogVideoManagerExtras::GetVideoAssetType()
{
  return VideoAssetType::EXTRAS;
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
        AddVideoExtra();
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

  // Enabled if list not empty
  if (m_videoAssetsList->IsEmpty())
  {
    SET_CONTROL_FOCUS(CONTROL_BUTTON_ADD_EXTRAS, 0);
  }
}

void CGUIDialogVideoManagerExtras::SetVideoAsset(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoManager::SetVideoAsset(item);

  if (!m_videoAssetsList->IsEmpty())
    SetSelectedVideoAsset(m_videoAssetsList->Get(0));
}

void CGUIDialogVideoManagerExtras::AddVideoExtra()
{
  const MediaType mediaType{m_videoAsset->GetVideoInfoTag()->m_type};

  // prompt to choose a video file
  VECSOURCES sources{*CMediaSourceSettings::GetInstance().GetSources("files")};

  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  CServiceBroker::GetMediaManager().GetNetworkLocations(sources);

  std::string path;
  if (CGUIDialogFileBrowser::ShowAndGetFile(
          sources, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
          g_localizeStrings.Get(40015), path))
  {
    std::string typeVideoVersion;
    std::string videoTitle;
    int idFile{-1};
    int idMedia{-1};
    MediaType itemMediaType;
    VideoAssetType videoAssetType{VideoAssetType::UNKNOWN};

    const int idVideoVersion{m_database.GetVideoVersionInfo(path, idFile, typeVideoVersion, idMedia,
                                                            itemMediaType, videoAssetType)};

    const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
    const VideoDbContentType itemType = m_videoAsset->GetVideoContentType();

    if (idVideoVersion != -1)
    {
      CFileItemList versions;
      m_database.GetVideoVersions(itemType, dbId, versions);
      if (std::any_of(versions.begin(), versions.end(),
                      [idFile](const std::shared_ptr<CFileItem>& version)
                      { return version->GetVideoInfoTag()->m_iDbId == idFile; }))
      {
        CGUIDialogOK::ShowAndGetInput(
            CVariant{40015}, StringUtils::Format(g_localizeStrings.Get(40026), typeVideoVersion));
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
              StringUtils::Format(g_localizeStrings.Get(40027), typeVideoVersion, videoTitle)))
      {
        return;
      }

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

    const std::string typeNewVideoVersion{
        CGUIDialogVideoManagerExtras::GenerateVideoExtra(URIUtils::GetFileName(path))};

    const int idNewVideoVersion{
        m_database.AddVideoVersionType(typeNewVideoVersion, VideoAssetTypeOwner::AUTO)};

    m_database.AddExtrasVideoVersion(itemType, dbId, idNewVideoVersion, item);

    // refresh data and controls
    Refresh();
    UpdateControls();
  }
}

void CGUIDialogVideoManagerExtras::ManageVideoExtra(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoManagerExtras* dialog{
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoManagerExtras>(
          WINDOW_DIALOG_MANAGE_VIDEO_EXTRAS)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_MANAGE_VIDEO_EXTRAS instance!");
    return;
  }

  dialog->SetVideoAsset(item);
  dialog->Open();
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
