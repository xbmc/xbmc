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
#include "GUIDialogWebBrowserHistory.h"
#include "GUIBaseContainer.h"
#include "util.h"
#include "DateTime.h" 

#ifdef WITH_LINKS_BROWSER

#define CONTROL_LIST           11

CGUIDialogWebBrowserHistory::CGUIDialogWebBrowserHistory(void)
    : CGUIDialog(WINDOW_DIALOG_WEB_HISTORY, "WebBrowserHistory.xml")
{
  LoadOnDemand(false);    // we are loaded by the web browser window.
}

CGUIDialogWebBrowserHistory::~CGUIDialogWebBrowserHistory(void)
{
}

bool CGUIDialogWebBrowserHistory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_LIST && message.GetParam1() == ACTION_SELECT_ITEM)
      {
        CGUIBaseContainer *pList = (CGUIBaseContainer *)GetControl(CONTROL_LIST);
        if (pList)
        {
          CFileItem *pItem = m_vecHistory[pList->GetSelectedItem()];

          if (pItem->m_strPath && g_browserManager.isRunning() && g_browserManager.GetBrowserWindow())
			  g_browserManager.GetBrowserWindow()->GoToURL((unsigned char *)pItem->m_strPath.c_str());
        }
        return true;
      }
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
	    CGUIDialog::OnMessage(message);
      m_gWindowManager.m_bPointerNav = false;
      g_Mouse.SetInactive();
      PopulateHistory();
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      m_vecHistory.Clear();
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogWebBrowserHistory::Render()
{
  CGUIDialog::Render();
}

void CGUIDialogWebBrowserHistory::PopulateHistory()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);

  ILinksBoksURLList *pList = g_browserManager.GetURLList(LINKSBOKS_URLLIST_HISTORY);

  CFileItem *pItem;
  for(pList->GetRoot(); pList->GetNext();)
  {
    LONGLONG ll;
    FILETIME ft;
    FILETIME lft;
    SYSTEMTIME st;
    CStdString strLastVisit;
    pItem = new CFileItem((CStdString)((char *)pList->GetTitle()));
    pItem->m_strPath = (CStdString)((char *)pList->GetURL());
    ll = Int32x32To64(pList->GetLastVisit(), 10000000) + 116444736000000000;
    ft.dwLowDateTime = (DWORD)(ll & 0xFFFFFFFF);
    ft.dwHighDateTime = (DWORD)(ll >> 32);
    FileTimeToLocalFileTime(&ft, &lft);
    FileTimeToSystemTime(&lft, &st);
    pItem->m_dateTime = st;
    CDateTime objST(pItem->m_dateTime);
    strLastVisit = objST.GetAsLocalizedDate();
    pItem->SetLabel2(strLastVisit);
    CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, pItem);
    OnMessage(msg2);
    m_vecHistory.Add(pItem);
  }

  delete pList;
}

#endif