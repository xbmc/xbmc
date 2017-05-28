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

#include "GUIWindowScreensaver.h"

#include "Application.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/ScreenSaver.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
  : CGUIWindow(WINDOW_SCREENSAVER, ""),
    m_addon(nullptr)
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
  if (m_addon)
  {
    g_graphicsContext.CaptureStateBlock();
    m_addon->Render();
    g_graphicsContext.ApplyStateBlock();
    return;
  }

  CGUIWindow::Render();
}

// called when the mouse is moved/clicked etc. etc.
EVENT_RESULT CGUIWindowScreensaver::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  g_windowManager.PreviousWindow();
  return EVENT_RESULT_HANDLED;
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_addon)
      {
        m_addon->Stop();
        delete m_addon;
        m_addon = nullptr;
      }

      g_graphicsContext.ApplyStateBlock();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);

      g_graphicsContext.CaptureStateBlock();

      ADDON::AddonPtr addonInfo;
      bool ret = ADDON::CAddonMgr::GetInstance().GetAddon(CServiceBroker::GetSettings().GetString(CSettings::SETTING_SCREENSAVER_MODE), addonInfo, ADDON::ADDON_SCREENSAVER);
      if (!ret || !addonInfo)
        return false;
      m_addon = new ADDON::CScreenSaver(std::dynamic_pointer_cast<ADDON::CAddonDll>(addonInfo));
      return m_addon->Start();
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
