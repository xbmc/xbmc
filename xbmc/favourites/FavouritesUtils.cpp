/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesUtils.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "favourites/FavouritesURL.h"
#include "favourites/GUIWindowFavourites.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "utils/ExecString.h"
#include "utils/Variant.h"
#include "view/GUIViewState.h"

#include <string>

namespace FAVOURITES_UTILS
{
bool ChooseAndSetNewName(CFileItem& item)
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

bool ChooseAndSetNewThumbnail(CFileItem& item)
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

bool MoveItem(CFileItemList& items, const std::shared_ptr<CFileItem>& item, int amount)
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
    items.Add(item);
    items.Remove(0);
  }
  else if (nextItem == 0)
  {
    items.AddFront(item, 0);
    items.Remove(itemPos + 1);
  }
  else
  {
    items.Swap(itemPos, nextItem);
  }
  return true;
}

bool RemoveItem(CFileItemList& items, const std::shared_ptr<CFileItem>& item)
{
  int iBefore = items.Size();
  items.Remove(item.get());
  return items.Size() == iBefore - 1;
}

bool ShouldEnableMoveItems()
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

namespace
{
bool ExecuteAction(const std::string& execString)
{
  if (!execString.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execString);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return true;
  }
  return false;
}
} // unnamed namespace

bool ExecuteAction(const CExecString& execString)
{
  return ExecuteAction(execString.GetExecString());
}

bool ExecuteAction(const CFavouritesURL& favURL)
{
  if (favURL.IsValid())
    return ExecuteAction(favURL.GetExecString());

  return false;
}

} // namespace FAVOURITES_UTILS
