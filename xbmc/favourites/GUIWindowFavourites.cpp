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
#include "Util.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "view/GUIViewState.h"

CGUIWindowFavourites::CGUIWindowFavourites()
  : CGUIMediaWindow(WINDOW_FAVOURITES, "MyFavourites.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

bool CGUIWindowFavourites::OnSelect(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return false;

  CGUIMessage message(GUI_MSG_EXECUTE, 0, GetID());
  message.SetStringParam(CUtil::GetExecPath(*(*m_vecItems)[item], std::to_string(GetID())));
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);

  return true;
}

bool CGUIWindowFavourites::OnPopupMenu(int item)
{
  return CGUIMediaWindow::OnPopupMenu(item) && Refresh(true);
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
