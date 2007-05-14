/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogWebBrowserBookmarks.h"
#include "GUIBaseContainer.h"
#include "GUIDialogContextMenu.h"

#ifdef WITH_LINKS_BROWSER

#define CONTROL_LIST           11
#define ADD_BOOKMARK_BUTTON    2
#define NEW_FOLDER_BUTTON      3

CGUIDialogWebBrowserBookmarks::CGUIDialogWebBrowserBookmarks(void)
    : CGUIDialog(WINDOW_DIALOG_WEB_BOOKMARKS, "WebBrowserBookmarks.xml")
{
  LoadOnDemand(false);    // we are loaded by the web browser window.
}

CGUIDialogWebBrowserBookmarks::~CGUIDialogWebBrowserBookmarks(void)
{
}

bool CGUIDialogWebBrowserBookmarks::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      if (message.GetParam1() == ACTION_PREVIOUS_MENU)
      {
      }
      else if (message.GetSenderId() == CONTROL_LIST && message.GetParam1() == ACTION_SELECT_ITEM)
      {
        CGUIBaseContainer *pList = (CGUIBaseContainer *)GetControl(CONTROL_LIST);
        if (pList)
        {
          CFileItem *pItem = m_vecCurrentLevelItems[pList->GetSelectedItem()];

          if (pItem->m_bIsFolder)
          {
            // clicked on a folder
            if (m_curRoot)
              m_vecCompletePath.push_back(m_curRoot);
            m_curRoot = pItem;
            m_iDepth = pItem->m_idepth;
            FillListControl();
          }
          else
          {
            // regular bookmark
		    if (g_browserManager.isRunning() && g_browserManager.GetBrowserWindow())
			  g_browserManager.GetBrowserWindow()->GoToURL((unsigned char *)pItem->m_strPath.c_str());
          }
        }
        return true;
      }
      else if (message.GetSenderId() == CONTROL_LIST && message.GetParam1() == ACTION_PARENT_DIR)
      {
        // back to parent
        if (m_iDepth >= 0)
        {
          if (m_vecCompletePath.size() > 0)
          {
            m_curRoot = m_vecCompletePath.back();
            m_vecCompletePath.pop_back();
            m_iDepth = m_curRoot->m_idepth;
          }
          else
          {
            m_curRoot = NULL;
            m_iDepth = -1;
          }

          FillListControl();
        }
      }
      else if (message.GetSenderId() == CONTROL_LIST && message.GetParam1() == ACTION_CONTEXT_MENU)
      {
        ContextMenu();
      }
      else if (message.GetSenderId() == ADD_BOOKMARK_BUTTON)
      {
        CFileItem *pItem = NULL;
        CGUIBaseContainer *pList = (CGUIBaseContainer *)GetControl(CONTROL_LIST);
        if (pList && pList->GetRows() > 0)
          pItem = m_vecCurrentLevelItems[pList->GetSelectedItem()];

        NewBookmark(pItem);
        FillListControl();
      }
      else if (message.GetSenderId() == NEW_FOLDER_BUTTON)
      {
        CFileItem *pItem = NULL;
        CGUIBaseContainer *pList = (CGUIBaseContainer *)GetControl(CONTROL_LIST);
        if (pList && pList->GetRows() > 0)
          pItem = m_vecCurrentLevelItems[pList->GetSelectedItem()];

        NewFolder(pItem);
        FillListControl();
      }
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
	    CGUIDialog::OnMessage(message);

      m_gWindowManager.m_bPointerNav = false;
      g_Mouse.SetInactive();
      PopulateBookmarks();
      m_iDepth = -1;
      m_curRoot = NULL;
      m_curBookmark = NULL;
      m_movingBookmark = NULL;
      FillListControl();
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      CLog::Log(LOGNOTICE, (SaveBookmarks()) ? "Web Bookmarks saved successfully!" : "ERROR! Web Bookmarks NOT saved!");
      
	  // wipe bookmarks file items
      if (m_vecBookmarks.size())
      {
        IVECFILEITEMS i;
        i = m_vecBookmarks.begin();
        while (i != m_vecBookmarks.end())
        {
          CFileItem* pItem = *i;
          delete pItem;
          i = m_vecBookmarks.erase(i);
        }
      }
      m_vecBookmarks.clear();
      m_curRoot = NULL;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogWebBrowserBookmarks::Render()
{
  CGUIDialog::Render();
}

void CGUIDialogWebBrowserBookmarks::PopulateBookmarks()
{
  m_vecBookmarks.clear();
  m_vecCompletePath.clear();

  ILinksBoksURLList *pList = g_browserManager.GetURLList(LINKSBOKS_URLLIST_BOOKMARKS);

  CFileItem *pItem;
  for(pList->GetRoot(); pList->GetNext();)
  {
    pItem = new CFileItem((CStdString)((char *)pList->GetTitle()));
    pItem->m_strPath = (CStdString)((char *)pList->GetURL());
    pItem->m_idepth = pList->GetDepth();
    pItem->m_bIsFolder = (pList->GetType() == 1);
    m_vecBookmarks.push_back(pItem);
  }

  //DebugPrintList();

  delete pList;
}

void CGUIDialogWebBrowserBookmarks::FillListControl()
{
  unsigned int i = -1;

  m_vecCurrentLevelItems.clear();
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);

  /* First, iterate to the root element unless we're at the top-level */
  if (m_curRoot)
    for (i = 0; i < m_vecBookmarks.size() && m_vecBookmarks[i] != m_curRoot; i++);

  i++;

  /* Now we must take into account only the items with the current depth,
  and break when it goes below */
  for (; i < m_vecBookmarks.size() && m_vecBookmarks[i]->m_idepth >= m_iDepth + 1; i++)
  {
    if (m_vecBookmarks[i]->m_idepth > m_iDepth + 1)
      continue;

    CFileItem *pItem = new CFileItem(m_vecBookmarks[i]->GetLabel());
    pItem->m_strPath = m_vecBookmarks[i]->m_strPath;
    pItem->m_bIsFolder = m_vecBookmarks[i]->m_bIsFolder;
    pItem->FillInDefaultIcon();
    pItem->Select(m_vecBookmarks[i] == m_movingBookmark);
    CGUIMessage msg2 = CGUIMessage(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, pItem);
    OnMessage(msg2);
    m_vecCurrentLevelItems.push_back(m_vecBookmarks[i]);
  }
}

void CGUIDialogWebBrowserBookmarks::NewBookmark(CFileItem *pItem)
{
  IVECFILEITEMS i = m_vecBookmarks.begin();
  CStdString strTitle, strURL;

  if (m_curRoot)
  {
    for (; i != m_vecBookmarks.end() && *i != m_curRoot; i++);
    i++;
  }

  if (pItem)
    for (; i != m_vecBookmarks.end() && *i != pItem; i++);

  if (g_browserManager.isRunning() && g_browserManager.GetBrowserWindow())
  {
    strTitle = g_browserManager.GetCurrentTitle();
    strURL = g_browserManager.GetCurrentURL();
  }

  if(!CGUIDialogKeyboard::ShowAndGetInput(strTitle, g_localizeStrings.Get(20408), false))
    return;

  if(!CGUIDialogKeyboard::ShowAndGetInput(strURL, g_localizeStrings.Get(20409), false))
    return;

  CFileItem *pNewItem = new CFileItem(strTitle);
  pNewItem->m_strPath = strURL;
  pNewItem->m_idepth = m_iDepth + 1;
  pNewItem->m_bIsFolder = false;

  m_vecBookmarks.insert(i, pNewItem);
}

void CGUIDialogWebBrowserBookmarks::NewFolder(CFileItem *pItem)
{
  IVECFILEITEMS i = m_vecBookmarks.begin();
  CStdString strTitle, strURL;

  if (m_curRoot)
  {
    for (; i != m_vecBookmarks.end() && *i != m_curRoot; i++);
    i++;
  }

  if (pItem)
    for (; i != m_vecBookmarks.end() && *i != pItem; i++);

  if(!CGUIDialogKeyboard::ShowAndGetInput(strTitle, g_localizeStrings.Get(20408), false))
    return;

  CFileItem *pNewItem = new CFileItem(strTitle);
  pNewItem->m_strPath = "";
  pNewItem->m_idepth = m_iDepth + 1;
  pNewItem->m_bIsFolder = true;

  m_vecBookmarks.insert(i, pNewItem);
}

void CGUIDialogWebBrowserBookmarks::EditBookmark(CFileItem *pItem)
{
  if (!pItem)
    return;

  CStdString strTitle, strURL;
  strTitle = g_browserManager.GetCurrentTitle();
  strURL = g_browserManager.GetCurrentURL();

  strTitle = pItem->GetLabel();
  if(!CGUIDialogKeyboard::ShowAndGetInput(strTitle, g_localizeStrings.Get(20408), false))
    return;

  if(!pItem->m_bIsFolder)
  {
    strURL = pItem->m_strPath;
    if(!CGUIDialogKeyboard::ShowAndGetInput(strURL, g_localizeStrings.Get(20409), false))
      return;
  }

  pItem->SetLabel(strTitle);
  if (!pItem->m_bIsFolder)
    pItem->m_strPath = strURL;
}

void CGUIDialogWebBrowserBookmarks::DeleteBookmark(CFileItem *pItem)
{
  IVECFILEITEMS i, j;
  CStdString strTitle, strURL;

  m_movingBookmark = NULL;  // safer!

  if (!pItem)
    return;

  // Get to the item
  for (i = m_vecBookmarks.begin(); i != m_vecBookmarks.end() && *i != pItem; i++);

  // Prompts user for confirmation
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(20402);
    pDialog->SetLine(0, 20415);
    pDialog->SetLine(1, (*i)->m_bIsFolder ? 20417 : 20416);
    pDialog->SetLine(2, "");
    pDialog->DoModal(m_gWindowManager.GetActiveWindow());
    if (!pDialog->IsConfirmed()) return;
  }

  if ((*i)->m_bIsFolder)
  {
    for (j = i; j != m_vecBookmarks.end() && ((*j)->m_idepth > (*i)->m_idepth || i == j); j++);

    m_vecBookmarks.erase(i, j);
  }
  else
  {
    m_vecBookmarks.erase(i);
  }

  //DebugPrintList();
}

void CGUIDialogWebBrowserBookmarks::MoveBookmark(CFileItem *pItem)
{
  IVECFILEITEMS i = m_vecBookmarks.begin();
  IVECFILEITEMS j, k;
  int depth_offset;


  if (!m_movingBookmark)
    return;

  // Get to the item to move
  for (j = m_vecBookmarks.begin(); j != m_vecBookmarks.end() && *j != m_movingBookmark; j++);

  depth_offset = m_iDepth + 1 - m_movingBookmark->m_idepth;

  if ((*j)->m_bIsFolder)
  {
    VECFILEITEMS tmpvec;
    IVECFILEITEMS l, m;

    // it's a folder, collect all subitems and change their depth
    for (k = j; k != m_vecBookmarks.end() && ((*k)->m_idepth > (*j)->m_idepth || j == k); k++)
    {
      if(*k == pItem || *k == m_curRoot)
      {
        CLog::Log(LOGWARNING, "Trying to move a bookmark to one of its own subfolders! Aborting!");
        return;
      }

      (*k)->m_idepth += depth_offset;
      tmpvec.push_back(*k);
    }

    m_vecBookmarks.erase(j, k);

    // Get to the destination item
    if (m_curRoot)
    {
      for (; i != m_vecBookmarks.end() && *i != m_curRoot; i++);
      i++;
    }

    if (pItem)
      for (; i != m_vecBookmarks.end() && *i != pItem; i++);

    m_vecBookmarks.insert(i, tmpvec.begin(), tmpvec.end());
  }
  else
  {
    (*j)->m_idepth += depth_offset;

    m_vecBookmarks.erase(j);

    // Get to the destination item
    if (m_curRoot)
    {
      for (; i != m_vecBookmarks.end() && *i != m_curRoot; i++);
      i++;
    }

    if (pItem)
      for (; i != m_vecBookmarks.end() && *i != pItem; i++);

    m_vecBookmarks.insert(i, m_movingBookmark);
  }

  m_movingBookmark = NULL;
  //DebugPrintList();
}

void CGUIDialogWebBrowserBookmarks::ContextMenu()
{

  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();
  CGUIBaseContainer *pList = (CGUIBaseContainer *)GetControl(CONTROL_LIST);
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  CFileItem *pItem = NULL;

  if (pList->GetRows() > 0)
  {
    pItem = m_vecCurrentLevelItems[pList->GetSelectedItem()];
  }

  pMenu->Initialize();
  pMenu->AddButton(20410);      // Edit
  pMenu->AddButton(20411);      // Delete
  pMenu->AddButton(20412);      // Begin move
  pMenu->AddButton(20413);      // Move here
  pMenu->AddButton(20414);      // Cancel move

  if (!pItem)
  {
    pMenu->EnableButton(1, false);
    pMenu->EnableButton(2, false);
    pMenu->EnableButton(3, false);
  }

  if (m_movingBookmark)
  {
    pMenu->EnableButton(3, false);
  }
  else
  {
    pMenu->EnableButton(4, false);
    pMenu->EnableButton(5, false);
  }

  pMenu->SetPosition(pList->GetXPosition() + 40, pList->GetYPosition() + 40);
  pMenu->DoModal(m_gWindowManager.GetActiveWindow());
  pMenu->Close();

  switch(pMenu->GetButton())
  {
  case 1:
    {
      EditBookmark(pItem);
    }
    break;
  case 2:
    {
      DeleteBookmark(pItem);
    }
    break;
  case 3:
    {
      m_movingBookmark = pItem;
    }
    break;
  case 4:
    {
      MoveBookmark(pItem);
    }
    break;
  case 5:
    {
      m_movingBookmark = NULL;
    }
    break;
  }

  if (pMenu->GetButton())
    FillListControl();

}

void CGUIDialogWebBrowserBookmarks::DebugPrintList()
{
  for(IVECFILEITEMS i = m_vecBookmarks.begin(); i < m_vecBookmarks.end(); i++)
  {
    if((*i)->m_bIsFolder)
      CLog::Log(LOGDEBUG, "d=%d [[%s]]", (*i)->m_idepth, (*i)->GetLabel().c_str());
    else
      CLog::Log(LOGDEBUG, "d=%d %s", (*i)->m_idepth, (*i)->GetLabel().c_str());
  }
}

bool CGUIDialogWebBrowserBookmarks::SaveBookmarks()
{
  ILinksBoksBookmarksWriter *pWriter = g_browserManager.GetBookmarksWriter();

  if(!pWriter || !pWriter->Begin())
    return false;

  for(IVECFILEITEMS i = m_vecBookmarks.begin(); i < m_vecBookmarks.end(); i++)
  {
    if(!pWriter->AppendBookmark((unsigned char *)(*i)->GetLabel().c_str(),
        (unsigned char *)(*i)->m_strPath.c_str(), (*i)->m_idepth, (*i)->m_bIsFolder))
      return false;
  }

  if(!pWriter->Save())
      return false;

  return true;
}


#endif