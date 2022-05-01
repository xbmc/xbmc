/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowScreensaver.h"

#include "Application.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/ScreenSaver.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

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
        m_addon.reset();
      }

      CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);

      CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();

      const std::string addon = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_SCREENSAVER_MODE);
      const ADDON::AddonInfoPtr addonBase =
          CServiceBroker::GetAddonMgr().GetAddonInfo(addon, ADDON::ADDON_SCREENSAVER);
      if (!addonBase)
        return false;
      m_addon = std::make_unique<KODI::ADDONS::CScreenSaver>(addonBase);
      return m_addon->Start();
    }

  case GUI_MSG_CHECK_LOCK:
    if (!g_passwordManager.IsProfileLockUnlocked())
    {
      g_application.SetScreenSaverLockFailed();
      return false;
    }
    g_application.SetScreenSaverUnlocked();
    return true;
  }

  return CGUIWindow::OnMessage(message);
}
