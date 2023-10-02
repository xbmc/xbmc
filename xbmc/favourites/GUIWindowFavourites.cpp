/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowFavourites.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "favourites/FavouritesURL.h"
#include "favourites/FavouritesUtils.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsUtils.h"
#include "utils/PlayerUtils.h"
#include "utils/StringUtils.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/guilib/VideoPlayActionProcessor.h"
#include "video/guilib/VideoSelectActionProcessor.h"

CGUIWindowFavourites::CGUIWindowFavourites()
  : CGUIMediaWindow(WINDOW_FAVOURITES, "MyFavourites.xml")
{
  m_loadType = KEEP_IN_MEMORY;
  CServiceBroker::GetFavouritesService().Events().Subscribe(
      this, &CGUIWindowFavourites::OnFavouritesEvent);
}

CGUIWindowFavourites::~CGUIWindowFavourites()
{
  CServiceBroker::GetFavouritesService().Events().Unsubscribe(this);
}

void CGUIWindowFavourites::OnFavouritesEvent(const CFavouritesService::FavouritesUpdated& event)
{
  CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, 0);
  CServiceBroker::GetAppMessenger()->SendGUIMessage(m);
}

namespace
{
bool ExecuteAction(const std::string& execute)
{
  if (!execute.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execute);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return true;
  }
  return false;
}

bool ExecuteAction(const CExecString& execute)
{
  return ExecuteAction(execute.GetExecString());
}

class CVideoSelectActionProcessor : public VIDEO::GUILIB::CVideoSelectActionProcessorBase
{
public:
  explicit CVideoSelectActionProcessor(CFileItem& item) : CVideoSelectActionProcessorBase(item) {}

protected:
  bool OnPlayPartSelected(unsigned int part) override
  {
    // part numbers are 1-based
    ExecuteAction({"PlayMedia", m_item, StringUtils::Format("playoffset={}", part - 1)});
    return true;
  }

  bool OnResumeSelected() override
  {
    ExecuteAction({"PlayMedia", m_item, "resume"});
    return true;
  }

  bool OnPlaySelected() override
  {
    ExecuteAction({"PlayMedia", m_item, "noresume"});
    return true;
  }

  bool OnQueueSelected() override
  {
    ExecuteAction({"QueueMedia", m_item, ""});
    return true;
  }

  bool OnInfoSelected() override
  {
    if (m_item.IsPVR())
    {
      CServiceBroker::GetPVRManager().Get<PVR::GUI::Utils>().OnInfo(m_item);
      return true;
    }
    else if (m_item.HasVideoInfoTag())
    {
      CGUIDialogVideoInfo::ShowFor(m_item);
      return true;
    }
    return false;
  }

  bool OnMoreSelected() override
  {
    CONTEXTMENU::ShowFor(std::make_shared<CFileItem>(m_item));
    return true;
  }
};

class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  explicit CVideoPlayActionProcessor(CFileItem& item) : CVideoPlayActionProcessorBase(item) {}

protected:
  bool OnResumeSelected() override
  {
    ExecuteAction({"PlayMedia", m_item, "resume"});
    return true;
  }

  bool OnPlaySelected() override
  {
    ExecuteAction({"PlayMedia", m_item, "noresume"});
    return true;
  }
};
} // namespace

bool CGUIWindowFavourites::OnSelect(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return false;

  const CFavouritesURL favURL{*(*m_vecItems)[item], GetID()};
  if (!favURL.IsValid())
    return false;

  CFileItem targetItem{favURL.GetTarget(), favURL.IsDir()};
  targetItem.LoadDetails();

  const bool isPlayMedia{favURL.GetAction() == CFavouritesURL::Action::PLAY_MEDIA};

  // video select action setting is for files only, except exec func is playmedia...
  if (targetItem.HasVideoInfoTag() && (!targetItem.m_bIsFolder || isPlayMedia))
  {
    CVideoSelectActionProcessor proc{targetItem};
    if (proc.Process())
      return true;
  }

  // exec the execute string for the original (!) item
  return ExecuteAction(favURL.GetExecString());
}

bool CGUIWindowFavourites::OnAction(const CAction& action)
{
  const int selectedItem = m_viewControl.GetSelectedItem();
  if (selectedItem < 0 || selectedItem >= m_vecItems->Size())
    return false;

  if (action.GetID() == ACTION_PLAYER_PLAY)
  {
    const CFavouritesURL favURL((*m_vecItems)[selectedItem]->GetPath());
    if (!favURL.IsValid())
      return false;

    CFileItem item{favURL.GetTarget(), favURL.IsDir()};
    item.LoadDetails();

    // video play action setting is for files and folders...
    if (item.HasVideoInfoTag() || (item.m_bIsFolder && VIDEO_UTILS::IsItemPlayable(item)))
    {
      CVideoPlayActionProcessor proc{item};
      if (proc.Process())
        return true;
    }

    if (CPlayerUtils::IsItemPlayable(item))
    {
      CFavouritesURL target{item, {}};
      if (target.GetAction() != CFavouritesURL::Action::PLAY_MEDIA)
      {
        // build a playmedia execute string for given target
        target = CFavouritesURL{CFavouritesURL::Action::PLAY_MEDIA,
                                {StringUtils::Paramify(item.GetPath())}};
      }
      return ExecuteAction(target.GetExecString());
    }
    return false;
  }
  else if (action.GetID() == ACTION_MOVE_ITEM_UP)
  {
    if (FAVOURITES_UTILS::ShouldEnableMoveItems())
      return MoveItem(selectedItem, -1);
  }
  else if (action.GetID() == ACTION_MOVE_ITEM_DOWN)
  {
    if (FAVOURITES_UTILS::ShouldEnableMoveItems())
      return MoveItem(selectedItem, +1);
  }
  else if (action.GetID() == ACTION_DELETE_ITEM)
  {
    return RemoveItem(selectedItem);
  }

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowFavourites::OnMessage(CGUIMessage& message)
{
  bool ret = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_REFRESH_LIST:
    {
      const int size = m_vecItems->Size();
      int selected = m_viewControl.GetSelectedItem();
      if (m_vecItems->Size() > 0 && selected == size - 1)
        --selected; // remove of last item, select the new last item after refresh

      Refresh(true);

      if (m_vecItems->Size() < size)
      {
        // item removed. select item after the removed item
        m_viewControl.SetSelectedItem(selected);
      }

      ret = true;
      break;
    }
  }

  return ret || CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowFavourites::Update(const std::string& strDirectory,
                                  bool updateFilterPath /* = true */)
{
  std::string directory = strDirectory;
  if (directory.empty())
    directory = "favourites://";

  return CGUIMediaWindow::Update(directory, updateFilterPath);
}

bool CGUIWindowFavourites::MoveItem(int item, int amount)
{
  if (item < 0 || item >= m_vecItems->Size() || m_vecItems->Size() < 2 || amount == 0)
    return false;

  if (FAVOURITES_UTILS::MoveItem(*m_vecItems, (*m_vecItems)[item], amount) &&
      CServiceBroker::GetFavouritesService().Save(*m_vecItems))
  {
    int selected = item + amount;
    if (selected >= m_vecItems->Size())
      selected = 0;
    else if (selected < 0)
      selected = m_vecItems->Size() - 1;

    m_viewControl.SetSelectedItem(selected);
    return true;
  }

  return false;
}

bool CGUIWindowFavourites::RemoveItem(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return false;

  if (FAVOURITES_UTILS::RemoveItem(*m_vecItems, (*m_vecItems)[item]) &&
      CServiceBroker::GetFavouritesService().Save(*m_vecItems))
    return true;

  return false;
}
