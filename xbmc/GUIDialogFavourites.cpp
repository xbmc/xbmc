/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogFavourites.h"
#include "GUIDialogContextMenu.h"
#include "Favourites.h"
#include "GUIWindowManager.h"
#include "GUIDialogKeyboard.h"
#include "FileItem.h"
#include "LocalizeStrings.h"

#define FAVOURITES_LIST 450

CGUIDialogFavourites::CGUIDialogFavourites(void)
    : CGUIDialog(WINDOW_DIALOG_FAVOURITES, "DialogFavourites.xml")
{
  m_favourites = new CFileItemList;
}

CGUIDialogFavourites::~CGUIDialogFavourites(void)
{
  delete m_favourites;
}

bool CGUIDialogFavourites::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == FAVOURITES_LIST)
    {
      int item = GetSelectedItem();
      int action = message.GetParam1();
      if (action == ACTION_SELECT_ITEM || action == ACTION_MOUSE_LEFT_CLICK)
        OnClick(item);
      else if (action == ACTION_MOVE_ITEM_UP)
        OnMoveItem(item, -1);
      else if (action == ACTION_MOVE_ITEM_DOWN)
        OnMoveItem(item, 1);
      else if (action == ACTION_CONTEXT_MENU || action == ACTION_MOUSE_RIGHT_CLICK)
        OnPopupMenu(item);
      else if (action == ACTION_DELETE_ITEM)
        OnDelete(item);
      else
        return false;
      return true;
    }
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    CGUIDialog::OnMessage(message);
    // clear our favourites
    CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), FAVOURITES_LIST);
    OnMessage(message);
    m_favourites->Clear();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogFavourites::OnInitWindow()
{
  CFavourites::Load(*m_favourites);
  UpdateList();
  CGUIWindow::OnInitWindow();
}

int CGUIDialogFavourites::GetSelectedItem()
{
  CGUIMessage message(GUI_MSG_ITEM_SELECTED, GetID(), FAVOURITES_LIST);
  OnMessage(message);
  return message.GetParam1();
}

void CGUIDialogFavourites::OnClick(int item)
{
  if (item < 0 || item >= m_favourites->Size())
    return;

  // grab our message, close the dialog, and send
  CFileItemPtr pItem = (*m_favourites)[item];
  CStdString execute(pItem->m_strPath);

  Close();

  CGUIMessage message(GUI_MSG_EXECUTE, 0, GetID());
  message.SetStringParam(execute);
  g_windowManager.SendMessage(message);
}

void CGUIDialogFavourites::OnPopupMenu(int item)
{
  if (item < 0 || item >= m_favourites->Size())
    return;

  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    // highlight the item
    (*m_favourites)[item]->Select(true);

    // initialize the positioning
    CPoint pos;
    const CGUIControl *pList = GetControl(FAVOURITES_LIST);
    if (pList)
      pos = pList->GetRenderPosition() + CPoint(pList->GetWidth() * 0.5f, pList->GetHeight() * 0.5f);
    pMenu->Initialize();

    int btn_MoveUp = m_favourites->Size() > 1 ? pMenu->AddButton(13332) : 0;
    int btn_MoveDown = m_favourites->Size() > 1 ? pMenu->AddButton(13333) : 0;
    int btn_Remove = pMenu->AddButton(15015);
    int btn_Rename = pMenu->AddButton(118);

    pMenu->OffsetPosition(pos.x, pos.y);
    pMenu->DoModal(GetID());
    int button = pMenu->GetButton();

    // unhighlight the item
    (*m_favourites)[item]->Select(false);

    if (button == btn_MoveUp)
      OnMoveItem(item, -1);
    else if (button == btn_MoveDown)
      OnMoveItem(item, 1);
    else if (button == btn_Remove)
      OnDelete(item);
    else if (button == btn_Rename)
      OnRename(item);
  }
}

void CGUIDialogFavourites::OnMoveItem(int item, int amount)
{
  if (item < 0 || item >= m_favourites->Size() || m_favourites->Size() <= 1 || 0 == amount) return;

  int nextItem = (item + amount) % m_favourites->Size();
  if (nextItem < 0) nextItem += m_favourites->Size();

  m_favourites->Swap(item, nextItem);
  CFavourites::Save(*m_favourites);

  CGUIMessage message(GUI_MSG_ITEM_SELECT, GetID(), FAVOURITES_LIST, nextItem);
  OnMessage(message);

  UpdateList();
}

void CGUIDialogFavourites::OnDelete(int item)
{
  if (item < 0 || item >= m_favourites->Size())
    return;
  m_favourites->Remove(item);
  CFavourites::Save(*m_favourites);

  CGUIMessage message(GUI_MSG_ITEM_SELECT, GetID(), FAVOURITES_LIST, item < m_favourites->Size() ? item : item - 1);
  OnMessage(message);

  UpdateList();
}

void CGUIDialogFavourites::OnRename(int item)
{
  if (item < 0 || item >= m_favourites->Size())
    return;

  CStdString label((*m_favourites)[item]->GetLabel());
  if (CGUIDialogKeyboard::ShowAndGetInput(label, g_localizeStrings.Get(16008), false))
    (*m_favourites)[item]->SetLabel(label);

  CFavourites::Save(*m_favourites);

  UpdateList();
}

void CGUIDialogFavourites::UpdateList()
{
  int currentItem = GetSelectedItem();
  CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), FAVOURITES_LIST, currentItem >= 0 ? currentItem : 0, 0, m_favourites);
  OnMessage(message);
}

CFileItemPtr CGUIDialogFavourites::GetCurrentListItem(int offset)
{
  int currentItem = GetSelectedItem();
  if (currentItem < 0 || !m_favourites->Size()) return CFileItemPtr();

  int item = (currentItem + offset) % m_favourites->Size();
  if (item < 0) item += m_favourites->Size();
  return (*m_favourites)[item];
}

