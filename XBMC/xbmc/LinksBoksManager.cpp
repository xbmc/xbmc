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

#ifdef WITH_LINKS_BROWSER

#include "LinksBoksManager.h"
#include "util.h"
#include "Application.h"
#include "FileSystem/DirectoryCache.h"
#include "cores\DllLoader\DllLoader.h"

CLinksBoksManager g_browserManager;

/* Function called when trying to open a known type. We get a stupid "commandline"
string to identify what to do, the file path, the fg parameter is not too important :)
If we return <0, the file isn't deleted after we return */
int WebBrowser_LaunchInXBMC(LinksBoksWindow *pLB, unsigned char *cmdline, unsigned char *filepath, int fg)
{
  CStdString strCommand = (CStdString)((char *)cmdline);

  // Empty the directory cache for our temp path... Else the new file could remain
  // undetected
  CStdString strPath;
  CUtil::GetDirectory((CStdString)((char *)filepath), strPath);
  strPath.ToLower();
  g_directoryCache.ClearDirectory(strPath);

  // We stop the current media first in order to maximize memory
  // We also tell Links to free its caches as much as possible
  //g_application.StopPlaying();
  g_browserManager.EmptyCaches();

  m_gWindowManager.m_bPointerNav = false;
  g_Mouse.SetInactive();

  CUtil::ExecBuiltIn(strCommand);
  return -1;
}

LinksBoksExtFont *WebBrowser_FontCallback(unsigned char *fontname, int fonttype)
{
  CStdString strFontName = (CStdString)((char *)fontname);
  return g_browserManager.GetFont(strFontName);
}

BOOL WebBrowser_MsgBoxHandler(void *dlg, unsigned char *title, int nblabels, unsigned char *labels[], int nbbuttons, unsigned char *buttons[])
{
  /* Don't handle dialogs with more than 2 buttons */
  if(nbbuttons > 2)
    return FALSE;
  
  CStdString strTitle = (CStdString)((char *)title);
  CStdString strLabels[3] = { "", "", "" };

  int n = 0;
  for(int i = 0; i < nblabels && n < 3; i++)
  {
    strLabels[n] = CStdString((char *)labels[i]);
  }

  m_gWindowManager.m_bPointerNav = false;
  g_Mouse.SetInactive();

  CGUIDialogBoxBase* pDialog = (CGUIDialogBoxBase *)m_gWindowManager.GetWindow((nbbuttons == 1) ? WINDOW_DIALOG_OK : WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(strTitle);
    pDialog->SetLine(0, strLabels[0]);
    pDialog->SetLine(1, strLabels[1]);
    pDialog->SetLine(2, strLabels[2]);
    pDialog->SetChoice(0, CStdString((char *)buttons[0]));
    if(nbbuttons > 1) pDialog->SetChoice(1, CStdString((char *)buttons[1]));
    pDialog->DoModal(m_gWindowManager.GetActiveWindow());
    if (!pDialog->IsConfirmed())
      g_browserManager.ValidateMsgBox(dlg, 1);
    else
      g_browserManager.ValidateMsgBox(dlg, 0);
  }

  return TRUE;
}

CLinksBoksManager::CLinksBoksManager(void)
{
  m_Initialized = false;
  m_IsLoaded = false;
  m_Sleeping = false;
  m_window = NULL;
}

CLinksBoksManager::~CLinksBoksManager(void)
{
  UnloadDLL();
}

bool CLinksBoksManager::RefreshSettings(bool bRedraw)
{
  if (!m_IsLoaded)
    return false;

  CStdString strHomePage = g_guiSettings.GetString("webbrowser.homepage");
  CStdString strDownloadDir = g_guiSettings.GetString("webbrowser.downloaddir");
  CStdString strBookmarksFile = g_guiSettings.GetString("webbrowser.bookmarks");

  if (strHomePage == "") strHomePage = "http://www.XboxMediaCenter.com";
  if (strDownloadDir == "") strDownloadDir = "Q://";

  int iTopMargin = g_guiSettings.GetInt("webbrowser.margintop");
  int iBottomMargin = g_guiSettings.GetInt("webbrowser.marginbottom");
  int iLeftMargin = g_guiSettings.GetInt("webbrowser.marginleft");
  int iRightMargin = g_guiSettings.GetInt("webbrowser.marginright");

  int iFontSize = g_guiSettings.GetInt("webbrowser.fontsize");
  int iScaleImages = g_guiSettings.GetInt("webbrowser.scaleimages");

  m_dll.LinksBoks_SetOptionString("homepage", (unsigned char *)strHomePage.c_str());
  m_dll.LinksBoks_SetOptionString("network_download_directory", (unsigned char *)strDownloadDir.c_str());
  m_dll.LinksBoks_SetOptionString("bookmarks_file", (unsigned char *)strBookmarksFile.c_str());

  m_dll.LinksBoks_SetOptionInt("html_font_size", iFontSize);
  m_dll.LinksBoks_SetOptionInt("menu_font_size", iFontSize);
  m_dll.LinksBoks_SetOptionInt("html_images_scale", iScaleImages);

  if(m_window && bRedraw)
  {
    m_viewport.margin_top = iTopMargin;
    m_viewport.margin_bottom = iBottomMargin;
    m_viewport.margin_left = iLeftMargin;
    m_viewport.margin_right = iRightMargin;
    
    m_window->ResizeWindow(m_viewport);
  }

  return true;
}

bool CLinksBoksManager::Initialize()
{
  if (!LoadDLL()) return false;
  m_dll.LinksBoks_InitCore((unsigned char *)"Q:\\userdata\\LinksBrowser\\", NULL, NULL);
  
  // Set some options, some from settings and some hardcoded
  RefreshSettings(false);

  // might want to calculate cache sizes according to available memory
	m_dll.LinksBoks_SetOptionInt("cache_fonts_size", 1000000);    // 1MB
	
  m_dll.LinksBoks_SetOptionBool("toolbar_button_visibility_back", FALSE);
	m_dll.LinksBoks_SetOptionBool("toolbar_button_visibility_history", FALSE);
	m_dll.LinksBoks_SetOptionBool("toolbar_button_visibility_forward", FALSE);
	m_dll.LinksBoks_SetOptionBool("toolbar_button_visibility_reload", FALSE);
	m_dll.LinksBoks_SetOptionBool("toolbar_button_visibility_stop", FALSE);
	m_dll.LinksBoks_SetOptionBool("toolbar_button_visibility_bookmarks", FALSE);
	m_dll.LinksBoks_SetOptionBool("video_dither_letters", FALSE);
	m_dll.LinksBoks_SetOptionBool("video_display_optimize", TRUE);
	m_dll.LinksBoks_SetOptionBool("tabs_show", FALSE);
	m_dll.LinksBoks_SetOptionBool("hide_menus", TRUE);
	m_dll.LinksBoks_SetOptionBool("keyboard_navigation", FALSE);

	// Register our function for external viewers, in our case media file.
	// We can do this at any time, even change it during execution.
  m_dll.LinksBoks_SetExecFunction(WebBrowser_LaunchInXBMC);

  // Register our function for providing fonts
  m_dll.LinksBoks_SetExtFontCallbackFunction(WebBrowser_FontCallback);

  // Same thing for the message box handler
  m_dll.LinksBoks_SetMessageBoxFunction(WebBrowser_MsgBoxHandler);

  m_Initialized = true;
  return true;
}

bool CLinksBoksManager::CreateBrowserWindow(int width, int height)
{
  if (!m_Initialized)
    return false;

  int iTopMargin = g_guiSettings.GetInt("webbrowser.margintop");
  int iBottomMargin = g_guiSettings.GetInt("webbrowser.marginbottom");
  int iLeftMargin = g_guiSettings.GetInt("webbrowser.marginleft");
  int iRightMargin = g_guiSettings.GetInt("webbrowser.marginright");

  m_viewport.margin_top = iTopMargin;
  m_viewport.margin_bottom = iBottomMargin;
  m_viewport.margin_left = iLeftMargin;
  m_viewport.margin_right = iRightMargin;

  m_viewport.width = width;
  m_viewport.height = height;
  m_window = m_dll.LinksBoks_CreateWindow(g_graphicsContext.Get3DDevice(), m_viewport);

  if (!m_window)
  {
    CLog::Log(LOGERROR, "LinksBoksManager::Error creating browser window!");
    Terminate();
    return false;
  }
  return true;
}

bool CLinksBoksManager::CloseBrowserWindow()
{
  if (!m_Initialized || m_Sleeping || !GetBrowserWindow())
	return false;

  m_window->Close();
  m_window = NULL;

  return true;
}

void CLinksBoksManager::Terminate()
{
  if(!m_Initialized)
    return;
  CloseBrowserWindow();
  m_dll.LinksBoks_Terminate(TRUE);
  m_window = NULL;
  m_Initialized = false;
  UnloadDLL();
}

ILinksBoksWindow *CLinksBoksManager::GetBrowserWindow()
{
  return m_window;
}

void CLinksBoksManager::FrameMove()
{
  if(!m_Initialized)
    return;

  m_dll.LinksBoks_FrameMove();
}

void CLinksBoksManager::EmptyCaches()
{
  if (!m_Initialized || m_Sleeping || !GetBrowserWindow())
    return;

  m_dll.LinksBoks_EmptyCaches();
}

bool CLinksBoksManager::Freeze()
{
  if (!m_Initialized || m_Sleeping || !GetBrowserWindow())
	return false;

  ILinksBoksWindow *pWindow = GetBrowserWindow();

  if (pWindow->Freeze())
    return false;
  m_dll.LinksBoks_FreezeCore();

  m_Sleeping = true;
  return true;
}

bool CLinksBoksManager::UnFreeze()
{
  if (!m_Initialized || !m_Sleeping || !GetBrowserWindow())
	return false;

  ILinksBoksWindow *pWindow = GetBrowserWindow();

  m_dll.LinksBoks_UnfreezeCore();
  if (pWindow->Unfreeze())
	  return false;

  m_Sleeping = false;
  return true;
}

bool CLinksBoksManager::LoadDLL()
{
  if (m_IsLoaded)
    return true;

  m_dll.Load();
  
  if(!m_dll.IsLoaded())
  {
    CLog::Log(LOGERROR, "LinksBoksManager: Unable to load dll!");
    return false;
  }

  m_IsLoaded = true;
  return true;
}

void CLinksBoksManager::UnloadDLL()
{
  m_IsLoaded = false;
  m_dll.Unload();
}

void CLinksBoksManager::RegisterFont(LinksBoksExtFont *xfont)
{
  m_vecFonts.push_back(xfont);
}

void CLinksBoksManager::UnregisterFont(LinksBoksExtFont *xfont)
{
  vector<LinksBoksExtFont *>::iterator i;

  for(i = m_vecFonts.begin(); i != m_vecFonts.end(); i++)
  {
    if (*i == xfont)
    {
      m_vecFonts.erase(i);
      break;
    }
  }
}

LinksBoksExtFont *CLinksBoksManager::GetFont(const CStdString& strFontName)
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    LinksBoksExtFont* font = m_vecFonts[i];
    if (!strcmp((const char *)font->name, strFontName))
      return font;
  }
  return NULL;
}

void CLinksBoksManager::ValidateMsgBox(void *dlg, int choice)
{
  if(m_Initialized)
    m_dll.LinksBoks_ValidateMessageBox(dlg, choice);
}


void CLinksBoksManager::SetExpertMode(BOOL mode)
{
  if(m_Initialized) {
	  m_dll.LinksBoks_SetOptionBool("hide_menus", mode);
	  m_window->RedrawWindow();
  }
}

BOOL CLinksBoksManager::GetExpertMode()
{
  if(m_Initialized)
	  return m_dll.LinksBoks_GetOptionBool("hide_menus");
  return FALSE;
}

CStdString CLinksBoksManager::GetCurrentTitle()
{
  if (isRunning() && m_window)
  {
	char title[256];
	memset(title, 0, 256);
	if (m_window->GetCurrentTitle((unsigned char *)title, 256))
	{
	  return (CStdString)title;
	}
  }
  return "";
}

CStdString CLinksBoksManager::GetCurrentURL()
{
  if (isRunning() && m_window)
  {
	char url[256];
	memset(url, 0, 256);
	if (m_window->GetCurrentURL((unsigned char *)url, 256))
	{
	  return (CStdString)url;
	}
  }
  return "";
}

CStdString CLinksBoksManager::GetCurrentStatus()
{
  if (isRunning() && m_window)
  {
	char status[256];
	memset(status, 0, 256);
	if (m_window->GetCurrentStatus((unsigned char *)status, 256))
	{
	  return (CStdString)status;
	}
  }
  return "";
}

int CLinksBoksManager::GetCurrentState()
{
  if (isRunning() && m_window)
  {
    int ret = m_window->GetCurrentState();
    return ret;
  }

  return -1;
}

ILinksBoksURLList *CLinksBoksManager::GetURLList(int type)
{
  if (isRunning())
  {
    return m_dll.LinksBoks_GetURLList(type);
  }

  return NULL;
}

ILinksBoksBookmarksWriter *CLinksBoksManager::GetBookmarksWriter()
{
  if (isRunning())
  {
    return m_dll.LinksBoks_GetBookmarksWriter();
  }

  return NULL;
}

#endif