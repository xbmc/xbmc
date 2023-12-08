/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoVersion.h"

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
#include "guilib/GUIImage.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "playlists/PlayListTypes.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/guilib/VideoPlayActionProcessor.h"

#include <algorithm>
#include <string>

static constexpr unsigned int CONTROL_LABEL_MODE = 1;
static constexpr unsigned int CONTROL_LABEL_TITLE = 2;
static constexpr unsigned int CONTROL_IMAGE_THUMB = 3;

static constexpr unsigned int CONTROL_BUTTON_PLAY = 21;
static constexpr unsigned int CONTROL_BUTTON_ADD_VERSION = 22;
static constexpr unsigned int CONTROL_BUTTON_ADD_EXTRAS = 23;
static constexpr unsigned int CONTROL_BUTTON_RENAME = 24;
static constexpr unsigned int CONTROL_BUTTON_SET_DEFAULT = 25;
static constexpr unsigned int CONTROL_BUTTON_REMOVE = 26;
static constexpr unsigned int CONTROL_BUTTON_CHOOSE_ART = 27;

static constexpr unsigned int CONTROL_LIST_PRIMARY_VERSION = 50;
static constexpr unsigned int CONTROL_LIST_EXTRAS_VERSION = 51;

CGUIDialogVideoVersion::CGUIDialogVideoVersion(int id)
  : CGUIDialog(id, "DialogVideoVersion.xml"),
    m_videoItem(std::make_shared<CFileItem>()),
    m_primaryVideoVersionList(std::make_unique<CFileItemList>()),
    m_extrasVideoVersionList(std::make_unique<CFileItemList>()),
    m_defaultVideoVersion(std::make_unique<CFileItem>()),
    m_selectedVideoVersion(std::make_unique<CFileItem>())
{
  m_loadType = KEEP_IN_MEMORY;

  if (!m_database.Open())
    CLog::Log(LOGERROR, "{}: Failed to open database", __FUNCTION__);
}

CGUIDialogVideoVersion::~CGUIDialogVideoVersion()
{
}

bool CGUIDialogVideoVersion::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_cancelled = false;
      break;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {
      ClearVideoVersionList();
      break;
    }

    case GUI_MSG_CLICKED:
    {
      const int control = message.GetSenderId();
      if (control == CONTROL_LIST_PRIMARY_VERSION)
      {
        const int action = message.GetParam1();
        if (action == ACTION_SELECT_ITEM || action == ACTION_MOUSE_LEFT_CLICK)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), control);
          OnMessage(msg);

          const int item = msg.GetParam1();
          if (item < 0 || item >= m_primaryVideoVersionList->Size())
            break;

          *m_selectedVideoVersion = *m_primaryVideoVersionList->Get(item);

          if (m_selectedVideoVersion->GetVideoInfoTag()->m_iDbId ==
              m_defaultVideoVersion->GetVideoInfoTag()->m_iDbId)
          {
            CONTROL_DISABLE(CONTROL_BUTTON_REMOVE);
            CONTROL_DISABLE(CONTROL_BUTTON_SET_DEFAULT);
          }
          else
          {
            CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_REMOVE, m_mode == Mode::MANAGE);
            CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_RENAME, m_mode == Mode::MANAGE);
            CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_SET_DEFAULT, m_mode == Mode::MANAGE);
          }

          if (m_mode == Mode::MANAGE)
            SET_CONTROL_FOCUS(CONTROL_BUTTON_PLAY, 0);
          else
            CloseAll();
        }
      }
      else if (control == CONTROL_LIST_EXTRAS_VERSION)
      {
        const int action = message.GetParam1();
        if (action == ACTION_SELECT_ITEM || action == ACTION_MOUSE_LEFT_CLICK)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), control);
          OnMessage(msg);

          const int item = msg.GetParam1();
          if (item < 0 || item >= m_extrasVideoVersionList->Size())
            break;

          *m_selectedVideoVersion = *m_extrasVideoVersionList->Get(item).get();

          CONTROL_DISABLE(CONTROL_BUTTON_SET_DEFAULT);

          CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_REMOVE, m_mode == Mode::MANAGE);
          CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_RENAME, m_mode == Mode::MANAGE);

          if (m_mode == Mode::MANAGE)
            SET_CONTROL_FOCUS(CONTROL_BUTTON_PLAY, 0);
          else
            CloseAll();
        }
      }
      else if (control == CONTROL_BUTTON_PLAY)
      {
        Play();
      }
      else if (control == CONTROL_BUTTON_ADD_VERSION)
      {
        AddVersion();
      }
      else if (control == CONTROL_BUTTON_ADD_EXTRAS)
      {
        AddExtras();
      }
      else if (control == CONTROL_BUTTON_RENAME)
      {
        Rename();
      }
      else if (control == CONTROL_BUTTON_SET_DEFAULT)
      {
        SetDefault();
      }
      else if (control == CONTROL_BUTTON_REMOVE)
      {
        Remove();
      }
      else if (control == CONTROL_BUTTON_CHOOSE_ART)
      {
        ChooseArt();
      }

      break;
    }
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogVideoVersion::OnBack(int actionID)
{
  m_cancelled = true;
  return CGUIDialog::OnBack(actionID);
}

void CGUIDialogVideoVersion::OnInitWindow()
{
  // set working mode
  SET_CONTROL_LABEL(CONTROL_LABEL_MODE, m_mode == Mode::MANAGE ? "manage" : "choose");

  // set window title
  std::string title = m_videoItem->GetVideoInfoTag()->GetTitle();

  const int year = m_videoItem->GetVideoInfoTag()->GetYear();
  if (year != 0)
    title = StringUtils::Format("{} ({})", title, year);

  SET_CONTROL_LABEL(
      CONTROL_LABEL_TITLE,
      StringUtils::Format(g_localizeStrings.Get(m_mode == Mode::MANAGE ? 40022 : 40023), title));

  // bind primary and extras version lists
  CGUIMessage msg1(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_PRIMARY_VERSION, 0, 0,
                   m_primaryVideoVersionList.get());
  OnMessage(msg1);

  CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_EXTRAS_VERSION, 0, 0,
                   m_extrasVideoVersionList.get());
  OnMessage(msg2);

  // disable buttons that need a selected video version
  CONTROL_DISABLE(CONTROL_BUTTON_REMOVE);
  CONTROL_DISABLE(CONTROL_BUTTON_SET_DEFAULT);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_ADD_VERSION, m_mode == Mode::MANAGE);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_ADD_EXTRAS, m_mode == Mode::MANAGE);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_CHOOSE_ART, m_mode == Mode::MANAGE);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_RENAME, m_mode == Mode::MANAGE);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BUTTON_PLAY, m_mode == Mode::MANAGE);

  CGUIDialog::OnInitWindow();
}

void CGUIDialogVideoVersion::ClearVideoVersionList()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_PRIMARY_VERSION);
  OnMessage(msg1);
  m_primaryVideoVersionList->Clear();

  CGUIMessage msg2(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_EXTRAS_VERSION);
  OnMessage(msg2);
  m_extrasVideoVersionList->Clear();
}

void CGUIDialogVideoVersion::RefreshVideoVersionList()
{
  // clear current version list
  ClearVideoVersionList();

  const int dbId = m_videoItem->GetVideoInfoTag()->m_iDbId;
  MediaType mediaType = m_videoItem->GetVideoInfoTag()->m_type;
  VideoDbContentType itemType = m_videoItem->GetVideoContentType();

  // get primary version list
  m_database.GetVideoVersions(itemType, dbId, *m_primaryVideoVersionList,
                              VideoVersionItemType::PRIMARY);
  m_primaryVideoVersionList->SetContent(CMediaTypes::ToPlural(mediaType));

  // refresh primary version list
  CGUIMessage msg1(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_PRIMARY_VERSION, 0, 0,
                   m_primaryVideoVersionList.get());
  OnMessage(msg1);

  // get extras version list
  m_database.GetVideoVersions(itemType, dbId, *m_extrasVideoVersionList,
                              VideoVersionItemType::EXTRAS);
  m_extrasVideoVersionList->SetContent(CMediaTypes::ToPlural(mediaType));

  // refresh extras version list
  CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_EXTRAS_VERSION, 0, 0,
                   m_extrasVideoVersionList.get());
  OnMessage(msg2);

  // update default video version
  m_database.GetDefaultVideoVersion(itemType, dbId, *m_selectedVideoVersion.get());
}

void CGUIDialogVideoVersion::SetVideoItem(const std::shared_ptr<CFileItem>& item)
{
  if (item == nullptr || !item->HasVideoInfoTag() ||
      item->GetVideoInfoTag()->m_type != MediaTypeMovie)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoVersion: Unexpected video item");
    return;
  }

  m_videoItem = item;

  ClearVideoVersionList();

  const int dbId = item->GetVideoInfoTag()->m_iDbId;
  MediaType mediaType = item->GetVideoInfoTag()->m_type;
  VideoDbContentType itemType = item->GetVideoContentType();

  m_database.GetVideoVersions(itemType, dbId, *m_primaryVideoVersionList,
                              VideoVersionItemType::PRIMARY);
  m_primaryVideoVersionList->SetContent(CMediaTypes::ToPlural(mediaType));

  m_database.GetVideoVersions(itemType, dbId, *m_extrasVideoVersionList,
                              VideoVersionItemType::EXTRAS);
  m_extrasVideoVersionList->SetContent(CMediaTypes::ToPlural(mediaType));

  m_database.GetDefaultVideoVersion(itemType, dbId, *m_defaultVideoVersion);
  m_database.GetDefaultVideoVersion(itemType, dbId, *m_selectedVideoVersion);

  CVideoThumbLoader loader;

  for (auto& item : *m_primaryVideoVersionList)
    loader.LoadItem(item.get());

  for (auto& item : *m_extrasVideoVersionList)
    loader.LoadItem(item.get());
}

void CGUIDialogVideoVersion::CloseAll()
{
  // close our dialog
  Close(true);

  // close the video info dialog if exists
  CGUIDialogVideoInfo* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoInfo>(
          WINDOW_DIALOG_VIDEO_INFO);
  if (dialog)
    dialog->Close(true);
}

namespace
{
class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  explicit CVideoPlayActionProcessor(const std::shared_ptr<CFileItem>& item,
                                     const std::shared_ptr<CFileItem>& videoVersion)
    : CVideoPlayActionProcessorBase(item, videoVersion)
  {
  }

protected:
  bool OnResumeSelected() override
  {
    m_item->SetStartOffset(STARTOFFSET_RESUME);
    Play();
    return true;
  }

  bool OnPlaySelected() override
  {
    Play();
    return true;
  }

private:
  void Play()
  {
    m_item->SetProperty("playlist_type_hint", PLAYLIST::TYPE_VIDEO);
    const ContentUtils::PlayMode mode{m_item->GetProperty("CheckAutoPlayNextItem").asBoolean()
                                          ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
                                          : ContentUtils::PlayMode::PLAY_ONLY_THIS};
    VIDEO_UTILS::PlayItem(m_item, "", mode);
  }
};
} // unnamed namespace

void CGUIDialogVideoVersion::Play()
{
  CloseAll();

  CVideoPlayActionProcessor proc{m_videoItem, m_selectedVideoVersion};
  proc.ProcessDefaultAction();
}

void CGUIDialogVideoVersion::Remove()
{
  MediaType mediaType = m_videoItem->GetVideoInfoTag()->m_type;

  // default video version is not allowed
  if (m_database.IsDefaultVideoVersion(m_selectedVideoVersion->GetVideoInfoTag()->m_iDbId))
  {
    CGUIDialogOK::ShowAndGetInput(
        CVariant(40018),
        StringUtils::Format(g_localizeStrings.Get(40019),
                            m_selectedVideoVersion->GetVideoInfoTag()->m_typeVideoVersion,
                            CMediaTypes::GetLocalization(mediaType),
                            m_videoItem->GetVideoInfoTag()->GetTitle()));
    return;
  }

  // confirm to remove
  if (!CGUIDialogYesNo::ShowAndGetInput(
          CVariant(40018),
          StringUtils::Format(g_localizeStrings.Get(40020),
                              m_selectedVideoVersion->GetVideoInfoTag()->m_typeVideoVersion)))
  {
    return;
  }

  // remove video version
  m_database.RemoveVideoVersion(m_selectedVideoVersion->GetVideoInfoTag()->m_iDbId);

  // refresh the video version list
  RefreshVideoVersionList();

  // select the default video version
  const int dbId = m_videoItem->GetVideoInfoTag()->m_iDbId;
  VideoDbContentType itemType = m_videoItem->GetVideoContentType();

  m_database.GetDefaultVideoVersion(itemType, dbId, *m_selectedVideoVersion.get());

  CONTROL_DISABLE(CONTROL_BUTTON_REMOVE);
  CONTROL_DISABLE(CONTROL_BUTTON_SET_DEFAULT);
}

void CGUIDialogVideoVersion::Rename()
{
  const int idVideoVersion = SelectVideoVersion(m_videoItem);
  if (idVideoVersion != -1)
  {
    m_database.SetVideoVersion(m_selectedVideoVersion->GetVideoInfoTag()->m_iDbId, idVideoVersion);
  }

  // refresh the video version list
  RefreshVideoVersionList();
}

void CGUIDialogVideoVersion::SetDefault()
{
  // set the selected video version as default
  SetDefaultVideoVersion(*m_selectedVideoVersion.get());

  const int dbId = m_videoItem->GetVideoInfoTag()->m_iDbId;
  VideoDbContentType itemType = m_videoItem->GetVideoContentType();

  // update our default video version
  m_database.GetDefaultVideoVersion(itemType, dbId, *m_defaultVideoVersion.get());

  CONTROL_DISABLE(CONTROL_BUTTON_SET_DEFAULT);
}

void CGUIDialogVideoVersion::SetDefaultVideoVersion(CFileItem& version)
{
  const int dbId = m_videoItem->GetVideoInfoTag()->m_iDbId;
  VideoDbContentType itemType = m_videoItem->GetVideoContentType();

  // set the specified video version as default
  m_database.SetDefaultVideoVersion(itemType, dbId, version.GetVideoInfoTag()->m_iDbId);

  // update the video item
  m_videoItem->SetPath(version.GetPath());
  m_videoItem->SetDynPath(version.GetPath());

  // update video details since we changed the video file for the item
  m_database.GetDetailsByTypeAndId(*m_videoItem, itemType, dbId);

  // notify all windows to update the file item
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, GUI_MSG_FLAG_FORCE_UPDATE,
                  m_videoItem);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CGUIDialogVideoVersion::ChooseArt()
{
  if (!CGUIDialogVideoInfo::ChooseAndManageVideoItemArtwork(
          std::make_shared<CFileItem>(*m_selectedVideoVersion)))
    return;

  // update the thumbnail
  CGUIControl* control = GetControl(CONTROL_IMAGE_THUMB);
  if (control)
  {
    CGUIImage* imageControl = static_cast<CGUIImage*>(control);
    imageControl->FreeResources();
    imageControl->SetFileName(m_selectedVideoVersion->GetArt("thumb"));
  }

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CGUIDialogVideoVersion::AddVersion()
{
  AddVideoVersion(true);
}

void CGUIDialogVideoVersion::AddExtras()
{
  AddVideoVersion(false);
}

void CGUIDialogVideoVersion::AddVideoVersion(bool primary)
{
  const int dbId = m_videoItem->GetVideoInfoTag()->m_iDbId;
  MediaType mediaType = m_videoItem->GetVideoInfoTag()->m_type;
  VideoDbContentType itemType = m_videoItem->GetVideoContentType();

  std::string title = primary ? StringUtils::Format(g_localizeStrings.Get(40014),
                                                    CMediaTypes::GetLocalization(mediaType))
                              : StringUtils::Format(g_localizeStrings.Get(40015),
                                                    CMediaTypes::GetLocalization(mediaType));

  // prompt to choose a video file
  VECSOURCES sources = *CMediaSourceSettings::GetInstance().GetSources("files");

  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  CServiceBroker::GetMediaManager().GetNetworkLocations(sources);

  std::string path;
  if (CGUIDialogFileBrowser::ShowAndGetFile(
          sources, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(), title, path))
  {
    std::string typeVideoVersion;
    std::string videoTitle;
    int idFile{-1};
    int idMedia{-1};
    MediaType itemMediaType;
    VideoVersionItemType versionItemType{VideoVersionItemType::UNKNOWN};

    const int idVideoVersion = m_database.GetVideoVersionInfo(
        path, idFile, typeVideoVersion, idMedia, itemMediaType, versionItemType);

    if (idVideoVersion != -1)
    {
      CFileItemList versions;
      m_database.GetVideoVersions(itemType, dbId, versions);
      if (std::any_of(versions.begin(), versions.end(),
                      [idFile](const std::shared_ptr<CFileItem>& version)
                      { return version->GetVideoInfoTag()->m_iDbId == idFile; }))
      {
        CGUIDialogOK::ShowAndGetInput(
            title, StringUtils::Format(g_localizeStrings.Get(40016), typeVideoVersion,
                                       CMediaTypes::GetLocalization(mediaType)));
        return;
      }

      if (itemMediaType == MediaTypeMovie)
      {
        videoTitle = m_database.GetMovieTitle(idMedia);
      }
      else
        return;

      if (!CGUIDialogYesNo::ShowAndGetInput(
              title, StringUtils::Format(g_localizeStrings.Get(40017), typeVideoVersion,
                                         CMediaTypes::GetLocalization(mediaType), videoTitle,
                                         CMediaTypes::GetLocalization(mediaType))))
      {
        return;
      }

      if (m_database.IsDefaultVideoVersion(idFile))
      {
        CFileItemList list;
        m_database.GetVideoVersions(itemType, idMedia, list);

        if (list.Size() > 1)
        {
          CGUIDialogOK::ShowAndGetInput(
              title, StringUtils::Format(g_localizeStrings.Get(40019), typeVideoVersion,
                                         CMediaTypes::GetLocalization(mediaType), videoTitle));
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

    CFileItem item(path, false);

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
    {
      CDVDFileInfo::GetFileStreamDetails(&item);
      CLog::Log(LOGDEBUG, "VideoVersion: Extracted filestream details from video file {}",
                CURL::GetRedacted(item.GetPath()));
    }

    if (primary)
    {
      const int idVideoVersion = SelectVideoVersion(m_videoItem);
      if (idVideoVersion != -1)
      {
        m_database.AddPrimaryVideoVersion(itemType, dbId, idVideoVersion, item);
      }
    }
    else
    {
      std::string typeVideoVersion =
          CGUIDialogVideoVersion::GenerateExtrasVideoVersion(URIUtils::GetFileName(path));

      const int idVideoVersion =
          m_database.AddVideoVersionType(typeVideoVersion, VideoVersionTypeOwner::AUTO);

      m_database.AddExtrasVideoVersion(itemType, dbId, idVideoVersion, item);
    }

    // refresh the video version list
    RefreshVideoVersionList();
  }
}

std::tuple<int, std::string> CGUIDialogVideoVersion::NewVideoVersion()
{
  std::string typeVideoVersion;

  // prompt for the new video version
  if (!CGUIKeyboardFactory::ShowAndGetInput(typeVideoVersion,
                                            CVariant{g_localizeStrings.Get(40004)}, false))
    return std::make_tuple(-1, "");

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::Log(LOGERROR, "{}: Failed to open database", __FUNCTION__);
    return std::make_tuple(-1, "");
  }

  typeVideoVersion = StringUtils::Trim(typeVideoVersion);
  const int idVideoVersion =
      videodb.AddVideoVersionType(typeVideoVersion, VideoVersionTypeOwner::USER);

  return std::make_tuple(idVideoVersion, typeVideoVersion);
}

void CGUIDialogVideoVersion::SetSelectedVideoVersion(const std::shared_ptr<CFileItem>& version)
{
  m_selectedVideoVersion = std::make_unique<CFileItem>(*version);
}

void CGUIDialogVideoVersion::ManageVideoVersion(const std::shared_ptr<CFileItem>& item)
{
  CGUIDialogVideoVersion* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoVersion>(
          WINDOW_DIALOG_VIDEO_VERSION);
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_VIDEO_VERSION instance!");
    return;
  }

  dialog->SetVideoItem(item);
  dialog->SetMode(Mode::MANAGE);
  dialog->Open();
}

CGUIDialogVideoVersion::VersionSelectResult CGUIDialogVideoVersion::ChooseVideoVersion(
    const std::shared_ptr<CFileItem>& item)
{
  if (!item->HasVideoInfoTag())
  {
    CLog::LogF(LOGWARNING, "Item is not a video. path={}", item->GetPath());
    return {true, {}};
  }

  if (!item->HasVideoVersions())
  {
    CLog::LogF(LOGWARNING, "Item has no video versions. path={}", item->GetPath());
    return {true, {}};
  }

  // prompt to select a video version
  CGUIDialogVideoVersion* dialog{
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoVersion>(
          WINDOW_DIALOG_VIDEO_VERSION_SELECT)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_VIDEO_VERSION_SELECT instance!");
    return {true, {}};
  }

  dialog->SetVideoItem(item);
  dialog->SetMode(Mode::CHOOSE);
  dialog->Open();

  // get the selected video version from dialog if not cancelled
  return {dialog->m_cancelled, dialog->m_selectedVideoVersion};
}

int CGUIDialogVideoVersion::ManageVideoVersionContextMenu(const std::shared_ptr<CFileItem>& version)
{
  CContextButtons buttons;

  buttons.Add(CONTEXT_BUTTON_RENAME, 118);
  buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 31614);
  buttons.Add(CONTEXT_BUTTON_DELETE, 15015);
  buttons.Add(CONTEXT_BUTTON_SET_ART, 13511);

  int button = CGUIDialogContextMenu::ShowAndGetChoice(buttons);
  if (button > 0)
  {
    CGUIDialogVideoVersion* dialog =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoVersion>(
            WINDOW_DIALOG_VIDEO_VERSION);
    if (!dialog)
      return -1;

    CFileItem videoItem;
    if (!dialog->m_database.GetVideoItemByVideoVersion(version->GetVideoInfoTag()->m_iDbId,
                                                       videoItem))
      return -1;

    dialog->SetVideoItem(std::make_shared<CFileItem>(videoItem));
    dialog->SetMode(Mode::MANAGE);
    dialog->SetSelectedVideoVersion(version);

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

int CGUIDialogVideoVersion::SelectVideoVersion(const std::shared_ptr<CFileItem>& item)
{
  if (!item || !item->HasVideoInfoTag())
    return -1;

  VideoDbContentType itemType = item->GetVideoContentType();
  if (itemType != VideoDbContentType::MOVIES)
    return -1;

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::Log(LOGERROR, "{}: Failed to open database", __FUNCTION__);
    return -1;
  }

  int idVideoVersion = -1;
  std::string typeVideoVersion;

  CFileItemList list;
  videodb.GetVideoVersionTypes(itemType, list);

  const std::string mediaType = item->GetVideoInfoTag()->m_type;
  const std::string title =
      StringUtils::Format(g_localizeStrings.Get(40003), CMediaTypes::GetLocalization(mediaType));

  while (true)
  {
    CGUIDialogSelect* dialog =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
            WINDOW_DIALOG_SELECT);
    if (!dialog)
      return -1;

    dialog->Reset();
    dialog->SetItems(list);
    dialog->SetHeading(title);
    dialog->EnableButton(true, 40004);
    dialog->Open();

    if (dialog->IsButtonPressed())
    {
      // create a new video version
      if (CGUIKeyboardFactory::ShowAndGetInput(typeVideoVersion, g_localizeStrings.Get(40004),
                                               false))
      {
        typeVideoVersion = StringUtils::Trim(typeVideoVersion);
        idVideoVersion = videodb.AddVideoVersionType(typeVideoVersion, VideoVersionTypeOwner::USER);
      }
    }
    else if (dialog->IsConfirmed())
    {
      std::shared_ptr<CFileItem> selectedItem = dialog->GetSelectedFileItem();
      idVideoVersion = selectedItem->GetVideoInfoTag()->m_idVideoVersion;
      typeVideoVersion = selectedItem->GetVideoInfoTag()->m_typeVideoVersion;
    }
    else
      return -1;

    if (idVideoVersion < 0)
      return -1;

    const int dbId = item->GetVideoInfoTag()->m_iDbId;

    CFileItemList versions;
    videodb.GetVideoVersions(itemType, dbId, versions);

    // the selected video version already exists
    if (std::any_of(versions.begin(), versions.end(),
                    [idVideoVersion](const std::shared_ptr<CFileItem>& version)
                    { return version->GetVideoInfoTag()->m_iDbId == idVideoVersion; }))
    {
      CGUIDialogOK::ShowAndGetInput(
          CVariant{40005}, StringUtils::Format(g_localizeStrings.Get(40007), typeVideoVersion));
    }
    else
      break;
  }

  return idVideoVersion;
}

bool CGUIDialogVideoVersion::ConvertVideoVersion(const std::shared_ptr<CFileItem>& item)
{
  if (item == nullptr || !item->HasVideoInfoTag())
    return false;

  VideoDbContentType itemType = item->GetVideoContentType();
  if (itemType != VideoDbContentType::MOVIES)
    return false;

  std::string mediaType = item->GetVideoInfoTag()->m_type;

  // invalid operation warning
  if (item->GetVideoInfoTag()->HasVideoVersions())
  {
    CGUIDialogOK::ShowAndGetInput(
        CVariant{40005},
        StringUtils::Format(g_localizeStrings.Get(40006), CMediaTypes::GetLocalization(mediaType)));
    return false;
  }

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::Log(LOGERROR, "{}: Failed to open database", __FUNCTION__);
    return false;
  }

  // get video list
  std::string videoTitlesDir =
      StringUtils::Format("videodb://{}/titles", CMediaTypes::ToPlural(mediaType));

  CFileItemList list;
  if (itemType == VideoDbContentType::MOVIES)
    videodb.GetMoviesNav(videoTitlesDir, list);
  else
    return false;

  if (list.Size() < 2)
    return false;

  list.Sort(SortByLabel, SortOrderAscending,
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
                ? SortAttributeIgnoreArticle
                : SortAttributeNone);

  const int dbId = item->GetVideoInfoTag()->m_iDbId;

  for (int i = 0; i < list.Size(); ++i)
  {
    if (list[i]->GetVideoInfoTag()->m_iDbId == dbId)
    {
      list.Remove(i);
      break;
    }
  }

  // decorate the items
  CVideoThumbLoader loader;
  for (auto& item : list)
  {
    loader.LoadItem(item.get());
    item->SetLabel2(item->GetVideoInfoTag()->m_strFileNameAndPath);
  }

  // choose the target video
  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (!dialog)
    return false;

  dialog->Reset();
  dialog->SetItems(list);
  dialog->SetHeading(
      StringUtils::Format(g_localizeStrings.Get(40002), CMediaTypes::GetLocalization(mediaType)));
  dialog->SetUseDetails(true);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  std::shared_ptr<CFileItem> selectedItem = dialog->GetSelectedFileItem();

  // choose a video version
  const int idVideoVersion = SelectVideoVersion(selectedItem);
  if (idVideoVersion < 0)
    return false;

  videodb.ConvertVideoToVersion(itemType, dbId, selectedItem->GetVideoInfoTag()->m_iDbId,
                                idVideoVersion);
  return true;
}

bool CGUIDialogVideoVersion::ProcessVideoVersion(VideoDbContentType itemType, int dbId)
{
  if (itemType != VideoDbContentType::MOVIES)
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::Log(LOGERROR, "{}: Failed to open database", __FUNCTION__);
    return false;
  }

  CFileItem item;
  if (!videodb.GetDetailsByTypeAndId(item, itemType, dbId))
    return false;

  CFileItemList list;
  videodb.GetSameVideoItems(item, list);

  if (list.Size() < 2)
    return false;

  MediaType mediaType = item.GetVideoInfoTag()->m_type;

  std::string path;
  videodb.GetFilePathById(dbId, path, itemType);

  if (!CGUIDialogYesNo::ShowAndGetInput(
          StringUtils::Format(g_localizeStrings.Get(40008),
                              CMediaTypes::GetLocalization(mediaType)),
          StringUtils::Format(g_localizeStrings.Get(40009), CMediaTypes::GetLocalization(mediaType),
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
  CVideoThumbLoader loader;
  for (auto& item : list)
  {
    loader.LoadItem(item.get());
    item->SetLabel2(item->GetVideoInfoTag()->m_strFileNameAndPath);
  }

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (!dialog)
    return false;

  dialog->Reset();
  dialog->SetItems(list);
  dialog->SetHeading(
      StringUtils::Format(g_localizeStrings.Get(40002), CMediaTypes::GetLocalization(mediaType)));
  dialog->SetUseDetails(true);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  std::shared_ptr<CFileItem> selectedItem = dialog->GetSelectedFileItem();

  // choose a video version
  const int idVideoVersion = SelectVideoVersion(selectedItem);
  if (idVideoVersion < 0)
    return false;

  videodb.ConvertVideoToVersion(itemType, dbId, selectedItem->GetVideoInfoTag()->m_iDbId,
                                idVideoVersion);
  return true;
}

std::string CGUIDialogVideoVersion::GenerateExtrasVideoVersion(const std::string& extrasRoot,
                                                               const std::string& extrasPath)
{
  // generate a video extra version string from its file path

  // remove the root path from its path
  std::string extrasVersion = extrasPath.substr(extrasRoot.size());

  return GenerateExtrasVideoVersion(extrasVersion);
}

std::string CGUIDialogVideoVersion::GenerateExtrasVideoVersion(const std::string& extrasPath)
{
  // generate a video extra version string from its file path

  std::string extrasVersion = extrasPath;

  // remove file extension
  URIUtils::RemoveExtension(extrasVersion);

  // remove special characters
  extrasVersion = StringUtils::ReplaceSpecialCharactersWithSpace(extrasVersion);

  // trim the string
  return StringUtils::Trim(extrasVersion);
}
