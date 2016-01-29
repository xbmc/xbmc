/*
 *      Copyright (C) 2005-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowFavourites.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "filesystem/FavouritesDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIKeyboardFactory.h"
#include "storage/MediaManager.h"

CGUIWindowFavourites::CGUIWindowFavourites(void) :
    CGUIMediaWindow(WINDOW_FAVOURITES, "Favourites.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowFavourites::~CGUIWindowFavourites(void)
{
}

void CGUIWindowFavourites::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;

  CFileItemPtr pItem = m_vecItems->Get(itemNumber);
  if (pItem->IsParentFolder())
    return;

  buttons.Add(CONTEXT_BUTTON_DELETE, 15015);
  buttons.Add(CONTEXT_BUTTON_RENAME, 118);
  buttons.Add(CONTEXT_BUTTON_SET_THUMB, 20019);

  CContextMenuManager::GetInstance().AddVisibleItems(pItem, buttons);
}

bool CGUIWindowFavourites::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;

  switch (button)
  {
    case CONTEXT_BUTTON_DELETE:
      OnDeleteItem(itemNumber);
      return true;

    case CONTEXT_BUTTON_RENAME:
      OnRenameItem(itemNumber);
      return true;

    case CONTEXT_BUTTON_SET_THUMB:
      OnSetThumb(itemNumber);
      return true;

    default:
      break;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowFavourites::OnDeleteItem(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return;
  m_vecItems->Remove(item);

  XFILE::CFavouritesDirectory::Save(*m_vecItems);

  Refresh(true);
}

void CGUIWindowFavourites::OnRenameItem(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return;

  std::string label(m_vecItems->Get(item)->GetLabel());
  if (CGUIKeyboardFactory::ShowAndGetInput(label, CVariant{g_localizeStrings.Get(16008)}, false))
    m_vecItems->Get(item)->SetLabel(label);

  XFILE::CFavouritesDirectory::Save(*m_vecItems);
  Refresh(true);
}

bool CGUIWindowFavourites::OnSelect(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return false;

  CFileItemPtr pItem = (*m_vecItems)[item];
  std::string execute(pItem->GetPath());

  CGUIMessage message(GUI_MSG_EXECUTE, 0, GetID());
  message.SetStringParam(execute);
  g_windowManager.SendMessage(message);

  return true;
}

void CGUIWindowFavourites::OnSetThumb(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return;

  CFileItemPtr pItem = m_vecItems->Get(item);
  CFileItemList items;

  // Current
  if (pItem->HasArt("thumb"))
  {
    CFileItemPtr current(new CFileItem("thumb://Current", false));
    current->SetArt("thumb", pItem->GetArt("thumb"));
    current->SetLabel(g_localizeStrings.Get(20016));
    items.Add(current);
  }

  // None
  CFileItemPtr none(new CFileItem("thumb://None", false));
  none->SetIconImage(pItem->GetIconImage());
  none->SetLabel(g_localizeStrings.Get(20018));
  items.Add(none);

  std::string thumb;
  VECSOURCES sources;
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(1030), thumb))
    return;

  pItem->SetArt("thumb", thumb);

  XFILE::CFavouritesDirectory::Save(*m_vecItems);
  Refresh(true);
}

bool CGUIWindowFavourites::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  std::string directory = strDirectory;
  if (directory.empty())
    directory = "favourites://";
  
  return CGUIMediaWindow::Update(directory, updateFilterPath);
}
