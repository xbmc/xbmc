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
  if (action.GetID() == ACTION_PLAYER_PLAY)
  {
    const int selectedItem = m_viewControl.GetSelectedItem();
    if (selectedItem < 0 || selectedItem >= m_vecItems->Size())
      return false;

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

bool CGUIWindowFavourites::MoveItem(CFileItemList& items, const CFileItem& item, int amount)
{
  if (items.Size() < 2 || amount == 0)
    return false;

  int itemPos = -1;
  for (const auto& i : items)
  {
    itemPos++;

    if (i->GetPath() == item.GetPath())
      break;
  }

  if (itemPos < 0 || itemPos >= items.Size())
    return false;

  int nextItem = (itemPos + amount) % items.Size();
  if (nextItem < 0)
    nextItem += items.Size();

  items.Swap(itemPos, nextItem);
  return true;
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
