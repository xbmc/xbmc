/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowScreensaver.h"
#include "GUIPassword.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/ScreenSaver.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
  : CGUIWindow(WINDOW_SCREENSAVER, "")
{
}

void CGUIWindowScreensaver::Process(unsigned int currentTime, CDirtyRegionList &regions)
{
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, regions);
  m_renderRegion.SetRect(0, 0, (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
}

void CGUIWindowScreensaver::Render()
{
  if (m_addon)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();
    m_addon->Render();
    CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
    return;
  }

  CGUIWindow::Render();
}

// called when the mouse is moved/clicked etc. etc.
EVENT_RESULT CGUIWindowScreensaver::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
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

      CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);

      CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();

      const ADDON::BinaryAddonBasePtr addonBase = CServiceBroker::GetBinaryAddonManager().GetInstalledAddonInfo(CServiceBroker::GetSettings().GetString(CSettings::SETTING_SCREENSAVER_MODE), ADDON::ADDON_SCREENSAVER);
      if (!addonBase)
        return false;
      m_addon = new ADDON::CScreenSaver(addonBase);
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
