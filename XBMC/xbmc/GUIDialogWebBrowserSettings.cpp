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
#include "GUIDialogWebBrowserSettings.h"
#include "util.h"
#include "application.h"

#ifdef WITH_LINKS_BROWSER

#include "LinksBoksManager.h"

CGUIDialogWebBrowserSettings::CGUIDialogWebBrowserSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_WEB_SETTINGS, "WebBrowserSettings.xml")
{
  m_strHomepage = g_guiSettings.GetString("webbrowser.homepage");
  m_iFontSize = g_guiSettings.GetInt("webbrowser.fontsize");
  m_iScaleImages = g_guiSettings.GetInt("webbrowser.scaleimages");
  m_iMarginTop = g_guiSettings.GetInt("webbrowser.margintop");
  m_iMarginBottom = g_guiSettings.GetInt("webbrowser.marginbottom");
  m_iMarginLeft = g_guiSettings.GetInt("webbrowser.marginleft");
  m_iMarginRight = g_guiSettings.GetInt("webbrowser.marginright");
}

CGUIDialogWebBrowserSettings::~CGUIDialogWebBrowserSettings(void)
{
}

#define WEB_SETTINGS_HOMEPAGE                 1
#define WEB_SETTINGS_HOMEPAGE_SETCURRENT      2
#define WEB_SETTINGS_HOMEPAGE_SETBLANK        3
/* separator at 4 */
#define WEB_SETTINGS_FONTSIZE                 5
#define WEB_SETTINGS_SCALEIMAGES              6
/* separator at 7 */
#define WEB_SETTINGS_MARGIN_TOP               8
#define WEB_SETTINGS_MARGIN_BOTTOM            9
#define WEB_SETTINGS_MARGIN_LEFT              10
#define WEB_SETTINGS_MARGIN_RIGHT             11
/* separator at 12 */
#define WEB_SETTINGS_ADVANCED_OPTIONS         13

void CGUIDialogWebBrowserSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();

  AddButton(WEB_SETTINGS_HOMEPAGE, 20420, true);
  AddButton(WEB_SETTINGS_HOMEPAGE_SETCURRENT, 20421, true);
  AddButton(WEB_SETTINGS_HOMEPAGE_SETBLANK, 20422, false);
  AddSeparator(4);
  AddSlider(WEB_SETTINGS_FONTSIZE, 20423, &m_iFontSize, 8, 30);
  AddSlider(WEB_SETTINGS_SCALEIMAGES, 20424, &m_iScaleImages, 50, 200);
  AddSeparator(7);
  AddSlider(WEB_SETTINGS_MARGIN_TOP, 20425, &m_iMarginTop, 0, 50);
  AddSlider(WEB_SETTINGS_MARGIN_BOTTOM, 20426, &m_iMarginBottom, 0, 50);
  AddSlider(WEB_SETTINGS_MARGIN_LEFT, 20427, &m_iMarginLeft, 0, 50);
  AddSlider(WEB_SETTINGS_MARGIN_RIGHT, 20428, &m_iMarginRight, 0, 50);
  AddSeparator(12);
  AddButton(WEB_SETTINGS_ADVANCED_OPTIONS, 20431, true);

  m_settings.at(0).name = g_localizeStrings.Get(20420) + ": " + g_guiSettings.GetString("webbrowser.homepage");
}

void CGUIDialogWebBrowserSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);

  CLog::Log(LOGDEBUG, "Web browser setting #%d changed!", num);

  bool bResized = FALSE;

  switch(setting.id)
  {
  case WEB_SETTINGS_HOMEPAGE:
    {
      CGUIDialogKeyboard::ShowAndGetInput(m_strHomepage, g_localizeStrings.Get(20401), false);
    }
    break;
  case WEB_SETTINGS_HOMEPAGE_SETBLANK:
    {
      m_strHomepage = "";
    }
    break;
  case WEB_SETTINGS_HOMEPAGE_SETCURRENT:
    {
      ILinksBoksWindow *pWindow;

      if(pWindow = g_browserManager.GetBrowserWindow())
      {
        if(g_browserManager.GetCurrentURL())
          m_strHomepage = g_browserManager.GetCurrentURL();
      }
    }
    break;
  case WEB_SETTINGS_MARGIN_TOP:
  case WEB_SETTINGS_MARGIN_BOTTOM:
  case WEB_SETTINGS_MARGIN_LEFT:
  case WEB_SETTINGS_MARGIN_RIGHT:
    {
      g_guiSettings.SetInt("webbrowser.margintop", m_iMarginTop);
      g_guiSettings.SetInt("webbrowser.marginbottom", m_iMarginBottom);
      g_guiSettings.SetInt("webbrowser.marginleft", m_iMarginLeft);
      g_guiSettings.SetInt("webbrowser.marginright", m_iMarginRight);
      bResized = TRUE;
    }
    break;
  case WEB_SETTINGS_FONTSIZE:
    {
      g_guiSettings.SetInt("webbrowser.fontsize", m_iFontSize);
    }
    break;
  case WEB_SETTINGS_SCALEIMAGES:
    {
      g_guiSettings.SetInt("webbrowser.scaleimages", m_iScaleImages);
    }
    break;
  case WEB_SETTINGS_ADVANCED_OPTIONS:
    {
      CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
      OnMessage(msg);
      m_gWindowManager.GetWindow(WINDOW_DIALOG_WEB_OSD)->OnMessage(msg);
      m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYWEATHER);

      return;
    }
    break;
  default:
    ; // nothing atm
  }

  g_guiSettings.SetString("webbrowser.homepage", m_strHomepage);
  m_settings.at(0).name = g_localizeStrings.Get(20420) + ": " + g_guiSettings.GetString("webbrowser.homepage");
  UpdateSetting(WEB_SETTINGS_HOMEPAGE);

  g_browserManager.RefreshSettings(true);
  g_settings.Save();

}



#endif