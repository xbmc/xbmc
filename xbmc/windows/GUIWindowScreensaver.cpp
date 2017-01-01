/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "GUIWindowScreensaver.h"
#include "addons/AddonManager.h"
#include "Application.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace ADDON;

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
    : CGUIWindow(WINDOW_SCREENSAVER, "")
{
}

CGUIWindowScreensaver::~CGUIWindowScreensaver(void)
{
}

void CGUIWindowScreensaver::Process(unsigned int currentTime, CDirtyRegionList &regions)
{
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, regions);
  m_renderRegion.SetRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
}

void CGUIWindowScreensaver::Render()
{
  CSingleLock lock (m_critSection);

  if (m_screenSaver)
  {
    //some screensavers seem to be depending on xbmc clearing the screen
    //       g_Windowing.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00010001, 1.0f, 0L );
    g_graphicsContext.CaptureStateBlock();
    m_screenSaver->Render();
    g_graphicsContext.ApplyStateBlock();
    return;
  }

  CGUIWindow::Render();
}

bool CGUIWindowScreensaver::OnAction(const CAction &action)
{
  // We're just a screen saver, nothing to do here
  return false;
}

// called when the mouse is moved/clicked etc. etc.
EVENT_RESULT CGUIWindowScreensaver::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  g_windowManager.PreviousWindow();
  return EVENT_RESULT_HANDLED;
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CSingleLock lock (m_critSection);
      if (m_screenSaver)
      {
        m_screenSaver->DestroyScreenSaver();
        g_graphicsContext.ApplyStateBlock();
        m_screenSaver.reset();
      }
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      CSingleLock lock (m_critSection);

      assert(!m_screenSaver);
      m_screenSaver.reset();

      // Setup new screensaver instance
      AddonPtr addon;
      if (CAddonMgr::GetInstance().GetAddon(CServiceBroker::GetSettings().GetString(CSettings::SETTING_SCREENSAVER_MODE), addon, ADDON_SCREENSAVER))
        m_screenSaver.reset(new CScreenSaver(std::dynamic_pointer_cast<CAddonDll>(addon)));
      if (!m_screenSaver)
        return false;

      g_graphicsContext.CaptureStateBlock();
      return m_screenSaver->CreateScreenSaver();
    }
  case GUI_MSG_CHECK_LOCK:
    if (!g_passwordManager.IsProfileLockUnlocked())
    {
      g_application.m_iScreenSaveLock = -1;
      return false;
    }
    g_application.m_iScreenSaveLock = 1;
    return true;
  }
  return CGUIWindow::OnMessage(message);
}
