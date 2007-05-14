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
#include "GUIWindowWebBrowser.h"
#include "GUIDialogContextMenu.h"
#include "guiWindowManager.h"
#include "util.h"
#include "application.h"
#include "utils/log.h"
#include "LinksBoksManager.h"
#include "utils/GUIInfoManager.h"

#ifdef WITH_LINKS_BROWSER

#define CONTROL_WEBBROWSER  10

CGUIWindowWebBrowser::CGUIWindowWebBrowser(void)
: CGUIWindow(WINDOW_WEB_BROWSER, "WebBrowser.xml")
{
  m_bKeepEngineRunning = FALSE;
  m_bShowPlayerInfo = FALSE;
  m_bShowWebPageInfo = FALSE;
}

CGUIWindowWebBrowser::~CGUIWindowWebBrowser(void)
{
}


bool CGUIWindowWebBrowser::OnAction(const CAction &action)
{
  CGUIWebBrowserControl *pControl = (CGUIWebBrowserControl *)GetControl(CONTROL_WEBBROWSER);
  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();

  switch(action.wID)
  {
  case ACTION_PREVIOUS_MENU:
    {
      // We're done browsing and going back to the main (or previous) screen
      // Tell the window and the control to terminate instead of freezing
      // The session is lost and all memory will be freed upon termination

      // Prompts user for confirmation first
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (pDialog)
      {
        pDialog->SetHeading(20400);
        pDialog->SetLine(0, 20418);
        pDialog->SetLine(1, 20419);
        pDialog->SetLine(2, "");
        pDialog->DoModal(m_gWindowManager.GetActiveWindow());
        if (!pDialog->IsConfirmed()) return true;
      }

      m_bKeepEngineRunning = FALSE;
      if (pControl) pControl->m_bKeepSession = FALSE;
      m_gWindowManager.PreviousWindow();
      return true;
    }
    break;
  case ACTION_WEBBROWSER_WEBPAGEINFO:
    {
      m_bShowWebPageInfo = !m_bShowWebPageInfo;
      g_infoManager.SetShowWebPageInfo(m_bShowWebPageInfo);
    }
    break;
    /* This one is the player song info */
  case ACTION_SHOW_INFO:
    {
      m_bShowPlayerInfo = !m_bShowPlayerInfo;
      g_infoManager.SetShowInfo(m_bShowPlayerInfo);
      return true;
    }
    break;
    /* Those are "hooks" to be able to scroll and go back/forward even if the
    webcontrol is not focused... */
  case ACTION_ANALOG_MOVE:
    {
      if (pControl) return pControl->OnAction(action);
    }
    break;
  }
  return CGUIWindow::OnAction(action);
}


bool CGUIWindowWebBrowser::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    m_strInitialURL = message.GetStringParam();
    m_bShowPlayerInfo = true;
    g_infoManager.SetShowInfo(true);
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    m_gWindowManager.m_bPointerNav = false;
    g_Mouse.SetInactive();
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowWebBrowser::AllocResources(bool forceLoad)
{
  if(g_browserManager.isFrozen())
  {
    // Engine is sleeping, unfreeze it
    g_browserManager.UnFreeze();
  }
  else
  {
    g_browserManager.Initialize();
  }

  CGUIWindow::AllocResources(forceLoad);

  CGUIWindow *pWindow;
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_OSD);
  if (pWindow) pWindow->AllocResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_BOOKMARKS);
  if (pWindow) pWindow->AllocResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_HISTORY);
  if (pWindow) pWindow->AllocResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_SETTINGS);
  if (pWindow) pWindow->AllocResources(true);

  // Set everything up to freeze the engine and the window instead of terminating
  // Takes some memory but allows us to get back our session when we get back
  m_bKeepEngineRunning = TRUE;
  CGUIWebBrowserControl *pControl = (CGUIWebBrowserControl *)GetControl(CONTROL_WEBBROWSER);
  if (pControl) pControl->m_bKeepSession = TRUE;

  // Now go to the "initial" URL if we were provided one...
  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();
  if (pLB && m_strInitialURL != "")
    pLB->GoToURL((unsigned char *)m_strInitialURL.c_str());
}

void CGUIWindowWebBrowser::FreeResources(bool forceLoad)
{
  if(m_bKeepEngineRunning)
  {
    // Don't terminate engine, put it to sleep instead
    g_browserManager.Freeze();

    // Make sure the correct value is still set in the control
    CGUIWebBrowserControl *pControl = (CGUIWebBrowserControl *)GetControl(CONTROL_WEBBROWSER);
    if (pControl) pControl->m_bKeepSession = TRUE;
  }
  else
  {
    g_browserManager.Terminate();

    // Make sure the correct value is still set in the control
    CGUIWebBrowserControl *pControl = (CGUIWebBrowserControl *)GetControl(CONTROL_WEBBROWSER);
    if (pControl) pControl->m_bKeepSession = FALSE;
  }

  CGUIWindow *pWindow;
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_OSD);
  if (pWindow) pWindow->FreeResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_BOOKMARKS);
  if (pWindow) pWindow->FreeResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_HISTORY);
  if (pWindow) pWindow->FreeResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_SETTINGS);
  if (pWindow) pWindow->FreeResources(true);

  CGUIWindow::FreeResources(forceLoad);
}

void CGUIWindowWebBrowser::Render()
{
  CGUIWindow::Render();
}

#endif /* WITH_LINKS_BROWSER */
