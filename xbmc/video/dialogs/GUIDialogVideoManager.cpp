/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoManager.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "playlists/PlayListTypes.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoThumbLoader.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/guilib/VideoPlayActionProcessor.h"

#include <algorithm>
#include <string>

static constexpr unsigned int CONTROL_LABEL_TITLE = 2;

static constexpr unsigned int CONTROL_BUTTON_PLAY = 21;
static constexpr unsigned int CONTROL_BUTTON_RENAME = 24;
static constexpr unsigned int CONTROL_BUTTON_REMOVE = 26;
static constexpr unsigned int CONTROL_BUTTON_CHOOSE_ART = 27;

static constexpr unsigned int CONTROL_LIST_ASSETS = 50;

CGUIDialogVideoManager::CGUIDialogVideoManager(int windowId)
  : CGUIDialog(windowId, "DialogVideoManager.xml"),
    m_videoAsset(std::make_shared<CFileItem>()),
    m_videoAssetsList(std::make_unique<CFileItemList>()),
    m_selectedVideoAsset(std::make_shared<CFileItem>())
{
  m_loadType = KEEP_IN_MEMORY;

  if (!m_database.Open())
    CLog::LogF(LOGERROR, "Failed to open video database!");
}

bool CGUIDialogVideoManager::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
      break;
    }

    case GUI_MSG_CLICKED:
    {
      const int control{message.GetSenderId()};
      if (control == CONTROL_LIST_ASSETS)
      {
        const int action{message.GetParam1()};
        if (action == ACTION_SELECT_ITEM || action == ACTION_MOUSE_LEFT_CLICK)
        {
          if (UpdateSelectedAsset())
            SET_CONTROL_FOCUS(CONTROL_BUTTON_PLAY, 0);
        }
      }
      else if (control == CONTROL_BUTTON_PLAY)
      {
        Play();
      }
      else if (control == CONTROL_BUTTON_RENAME)
      {
        Rename();
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

bool CGUIDialogVideoManager::OnAction(const CAction& action)
{
  const int actionId{action.GetID()};
  if (actionId == ACTION_MOVE_DOWN || actionId == ACTION_MOVE_UP || actionId == ACTION_PAGE_DOWN ||
      actionId == ACTION_PAGE_UP || actionId == ACTION_FIRST_PAGE || actionId == ACTION_LAST_PAGE)
  {
    if (GetFocusedControlID() == CONTROL_LIST_ASSETS)
    {
      CGUIDialog::OnAction(action);
      return UpdateSelectedAsset();
    }
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogVideoManager::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  SET_CONTROL_LABEL(CONTROL_LABEL_TITLE,
                    StringUtils::Format(g_localizeStrings.Get(GetHeadingId()),
                                        m_videoAsset->GetVideoInfoTag()->GetTitle()));

  CGUIMessage msg{GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_ASSETS, 0, 0, m_videoAssetsList.get()};
  OnMessage(msg);

  UpdateControls();
}

void CGUIDialogVideoManager::Clear()
{
  m_videoAssetsList->Clear();
}

void CGUIDialogVideoManager::UpdateButtons()
{
  // Enabled if list not empty
  if (!m_videoAssetsList->IsEmpty())
  {
    CONTROL_ENABLE(CONTROL_BUTTON_CHOOSE_ART);
    CONTROL_ENABLE(CONTROL_BUTTON_RENAME);
    CONTROL_ENABLE(CONTROL_BUTTON_REMOVE);
    CONTROL_ENABLE(CONTROL_BUTTON_PLAY);

    SET_CONTROL_FOCUS(CONTROL_LIST_ASSETS, 0);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BUTTON_CHOOSE_ART);
    CONTROL_DISABLE(CONTROL_BUTTON_RENAME);
    CONTROL_DISABLE(CONTROL_BUTTON_REMOVE);
    CONTROL_DISABLE(CONTROL_BUTTON_PLAY);
  }
}

void CGUIDialogVideoManager::UpdateAssetsList()
{
  // find new item in list and select it
  for (int i = 0; i < m_videoAssetsList->Size(); ++i)
  {
    if (m_videoAssetsList->Get(i)->GetVideoInfoTag()->m_iDbId ==
        m_selectedVideoAsset->GetVideoInfoTag()->m_iDbId)
    {
      CONTROL_SELECT_ITEM(CONTROL_LIST_ASSETS, i);
      break;
    }
  }
}

bool CGUIDialogVideoManager::UpdateSelectedAsset()
{
  CGUIMessage msg{GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_ASSETS};
  OnMessage(msg);

  const int item{msg.GetParam1()};
  if (item >= 0 && item < m_videoAssetsList->Size())
  {
    m_selectedVideoAsset = m_videoAssetsList->Get(item);
    UpdateControls();
    return true;
  }
  return false;
}

void CGUIDialogVideoManager::DisableRemove()
{
  CONTROL_DISABLE(CONTROL_BUTTON_REMOVE);
}

void CGUIDialogVideoManager::EnableRemove()
{
  CONTROL_ENABLE(CONTROL_BUTTON_REMOVE);
}

void CGUIDialogVideoManager::UpdateControls()
{
  UpdateButtons();
  UpdateAssetsList();
}

void CGUIDialogVideoManager::Refresh()
{
  Clear();

  const int dbId{m_videoAsset->GetVideoInfoTag()->m_iDbId};
  const MediaType mediaType{m_videoAsset->GetVideoInfoTag()->m_type};
  const VideoDbContentType itemType{m_videoAsset->GetVideoContentType()};

  //! @todo db refactor: should not be versions, but assets
  m_database.GetVideoVersions(itemType, dbId, *m_videoAssetsList, GetVideoAssetType());
  m_videoAssetsList->SetContent(CMediaTypes::ToPlural(mediaType));

  CVideoThumbLoader loader;

  for (auto& item : *m_videoAssetsList)
    loader.LoadItem(item.get());

  CGUIMessage msg{GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_ASSETS, 0, 0, m_videoAssetsList.get()};
  OnMessage(msg);
}

void CGUIDialogVideoManager::SetVideoAsset(const std::shared_ptr<CFileItem>& item)
{
  if (!item || !item->HasVideoInfoTag() || item->GetVideoInfoTag()->m_type != MediaTypeMovie)
  {
    CLog::LogF(LOGERROR, "Unexpected video item!");
    return;
  }

  m_videoAsset = item;

  Refresh();
}

void CGUIDialogVideoManager::CloseAll()
{
  // close our dialog
  Close(true);

  // close the video info dialog if exists
  CGUIDialogVideoInfo* dialog{
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoInfo>(
          WINDOW_DIALOG_VIDEO_INFO)};
  if (dialog)
    dialog->Close(true);
}

namespace
{
class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  explicit CVideoPlayActionProcessor(const std::shared_ptr<CFileItem>& item)
    : CVideoPlayActionProcessorBase(item)
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

void CGUIDialogVideoManager::Play()
{
  CloseAll();

  CVideoPlayActionProcessor proc{m_selectedVideoAsset};
  proc.ProcessDefaultAction();
}

void CGUIDialogVideoManager::Remove()
{
  // confirm to remove
  if (!CGUIDialogYesNo::ShowAndGetInput(
          CVariant(40018),
          StringUtils::Format(g_localizeStrings.Get(40020),
                              m_selectedVideoAsset->GetVideoInfoTag()->GetAssetInfo().GetTitle())))
  {
    return;
  }

  //! @todo db refactor: should not be version, but asset
  m_database.RemoveVideoVersion(m_selectedVideoAsset->GetVideoInfoTag()->m_iDbId);

  // refresh data and controls
  Refresh();
  UpdateControls();
}

void CGUIDialogVideoManager::Rename()
{
  const int idAsset{SelectVideoAsset(m_videoAsset)};
  if (idAsset != -1)
  {
    //! @todo db refactor: should not be version, but asset
    m_database.SetVideoVersion(m_selectedVideoAsset->GetVideoInfoTag()->m_iDbId, idAsset);
  }

  // refresh data and controls
  Refresh();
  UpdateControls();
}

void CGUIDialogVideoManager::ChooseArt()
{
  if (!CGUIDialogVideoInfo::ChooseAndManageVideoItemArtwork(m_selectedVideoAsset))
    return;

  // refresh data and controls
  Refresh();
}

void CGUIDialogVideoManager::SetSelectedVideoAsset(const std::shared_ptr<CFileItem>& asset)
{
  m_selectedVideoAsset = asset;

  UpdateControls();
}

int CGUIDialogVideoManager::SelectVideoAsset(const std::shared_ptr<CFileItem>& item)
{
  if (!item || !item->HasVideoInfoTag())
    return -1;

  const VideoDbContentType itemType{item->GetVideoContentType()};
  if (itemType != VideoDbContentType::MOVIES)
    return -1;

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database!");
    return -1;
  }

  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT instance!");
    return -1;
  }

  //! @todo db refactor: should not be version, but asset
  CFileItemList list;
  videodb.GetVideoVersionTypes(itemType, list);

  int assetId{-1};
  while (true)
  {
    std::string assetTitle;

    dialog->Reset();
    dialog->SetItems(list);
    dialog->SetHeading(40208);
    dialog->EnableButton(true, 40004);
    dialog->Open();

    if (dialog->IsButtonPressed())
    {
      // create a new asset
      if (CGUIKeyboardFactory::ShowAndGetInput(assetTitle, g_localizeStrings.Get(40004), false))
      {
        assetTitle = StringUtils::Trim(assetTitle);
        //! @todo db refactor: should not be version, but asset
        assetId = videodb.AddVideoVersionType(assetTitle, VideoAssetTypeOwner::USER);
      }
    }
    else if (dialog->IsConfirmed())
    {
      const std::shared_ptr<CFileItem> selectedItem{dialog->GetSelectedFileItem()};
      assetId = selectedItem->GetVideoInfoTag()->GetAssetInfo().GetId();
      assetTitle = selectedItem->GetVideoInfoTag()->GetAssetInfo().GetTitle();
    }
    else
      return -1;

    if (assetId < 0)
      return -1;

    const int dbId{item->GetVideoInfoTag()->m_iDbId};

    //! @todo db refactor: should not be versions, but assets
    CFileItemList assets;
    videodb.GetVideoVersions(itemType, dbId, assets);

    // the selected video asset already exists
    if (std::any_of(assets.cbegin(), assets.cend(),
                    [assetId](const std::shared_ptr<CFileItem>& asset)
                    { return asset->GetVideoInfoTag()->m_iDbId == assetId; }))
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{40005},
                                    StringUtils::Format(g_localizeStrings.Get(40007), assetTitle));
    }
    else
      break;
  }

  return assetId;
}
