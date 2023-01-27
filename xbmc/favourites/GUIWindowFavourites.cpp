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
#include "dialogs/GUIDialogFileBrowser.h"
#include "favourites/FavouritesURL.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "storage/MediaManager.h"
#include "utils/PlayerUtils.h"
#include "utils/StringUtils.h"
#include "view/GUIViewState.h"

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
    if (ShouldEnableMoveItems())
      return MoveItem(selectedItem, -1);
  }
  else if (action.GetID() == ACTION_MOVE_ITEM_DOWN)
  {
    if (ShouldEnableMoveItems())
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

bool CGUIWindowFavourites::ChooseAndSetNewName(CFileItem& item)
{
  std::string label = item.GetLabel();
  if (CGUIKeyboardFactory::ShowAndGetInput(label, CVariant{g_localizeStrings.Get(16008)},
                                           false)) // Enter new title
  {
    item.SetLabel(label);
    return true;
  }
  return false;
}

bool CGUIWindowFavourites::ChooseAndSetNewThumbnail(CFileItem& item)
{
  CFileItemList prefilledItems;
  if (item.HasArt("thumb"))
  {
    const auto current = std::make_shared<CFileItem>("thumb://Current", false);
    current->SetArt("thumb", item.GetArt("thumb"));
    current->SetLabel(g_localizeStrings.Get(20016)); // Current thumb
    prefilledItems.Add(current);
  }

  const auto none = std::make_shared<CFileItem>("thumb://None", false);
  none->SetArt("icon", item.GetArt("icon"));
  none->SetLabel(g_localizeStrings.Get(20018)); // No thumb
  prefilledItems.Add(none);

  std::string thumb;
  VECSOURCES sources;
  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  if (CGUIDialogFileBrowser::ShowAndGetImage(prefilledItems, sources, g_localizeStrings.Get(1030),
                                             thumb)) // Browse for image
  {
    item.SetArt("thumb", thumb);
    return true;
  }
  return false;
}

bool CGUIWindowFavourites::MoveItem(int item, int amount)
{
  if (item < 0 || item >= m_vecItems->Size() || m_vecItems->Size() < 2 || amount == 0)
    return false;

  if (MoveItem(*m_vecItems, (*m_vecItems)[item], amount) &&
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

bool CGUIWindowFavourites::MoveItem(CFileItemList& items,
                                    const std::shared_ptr<CFileItem>& item,
                                    int amount)
{
  if (items.Size() < 2 || amount == 0)
    return false;

  int itemPos = -1;
  for (const auto& i : items)
  {
    itemPos++;

    if (i->GetPath() == item->GetPath())
      break;
  }

  if (itemPos < 0 || itemPos >= items.Size())
    return false;

  int nextItem = (itemPos + amount) % items.Size();
  if (nextItem < 0)
  {
    const auto itemToAdd(item);
    items.Remove(itemPos);
    items.Add(itemToAdd);
  }
  else if (nextItem == 0)
  {
    const auto itemToAdd(item);
    items.Remove(itemPos);
    items.AddFront(itemToAdd, 0);
  }
  else
  {
    items.Swap(itemPos, nextItem);
  }
  return true;
}

bool CGUIWindowFavourites::RemoveItem(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return false;

  if (RemoveItem(*m_vecItems, (*m_vecItems)[item]) &&
      CServiceBroker::GetFavouritesService().Save(*m_vecItems))
    return true;

  return false;
}

bool CGUIWindowFavourites::RemoveItem(CFileItemList& items, const std::shared_ptr<CFileItem>& item)
{
  int iBefore = items.Size();
  items.Remove(item.get());
  return items.Size() == iBefore - 1;
}

bool CGUIWindowFavourites::ShouldEnableMoveItems()
{
  auto& mgr = CServiceBroker::GetGUI()->GetWindowManager();
  CGUIWindowFavourites* window = mgr.GetWindow<CGUIWindowFavourites>(WINDOW_FAVOURITES);
  if (window && window->IsActive())
  {
    const CGUIViewState* state = window->GetViewState();
    if (state && state->GetSortMethod().sortBy != SortByUserPreference)
      return false; // in favs window, allow move only if current sort method is by user preference
  }
  return true;
}
