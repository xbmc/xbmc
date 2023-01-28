/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowFavourites.h"

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
#include "utils/PlayerUtils.h"
#include "utils/StringUtils.h"

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
} // namespace

bool CGUIWindowFavourites::OnSelect(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return false;

  return ExecuteAction(CFavouritesURL(*(*m_vecItems)[item], GetID()).GetExecString());
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

    // If action is playmedia, just play it
    if (favURL.GetAction() == CFavouritesURL::Action::PLAY_MEDIA)
      return ExecuteAction(favURL.GetExecString());

    // Resolve and check the target
    const auto item = std::make_shared<CFileItem>(favURL.GetTarget(), favURL.IsDir());
    if (CPlayerUtils::IsItemPlayable(*item))
    {
      CFavouritesURL target(*item, {});
      if (target.GetAction() == CFavouritesURL::Action::PLAY_MEDIA)
      {
        return ExecuteAction(target.GetExecString());
      }
      else
      {
        // build and execute a playmedia execute string
        target = CFavouritesURL(CFavouritesURL::Action::PLAY_MEDIA,
                                {StringUtils::Paramify(item->GetPath())});
        return ExecuteAction(target.GetExecString());
      }
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
