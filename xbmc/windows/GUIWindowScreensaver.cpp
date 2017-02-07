/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowScreensaver.h"

#include "Application.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"

using namespace ADDON;

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
  : CGUIWindow(WINDOW_SCREENSAVER, ""),
    IAddonInstanceHandler(ADDON_SCREENSAVER),
    m_addon(nullptr)
{
  memset(&m_struct, 0, sizeof(m_struct));
}

void CGUIWindowScreensaver::Process(unsigned int currentTime, CDirtyRegionList &regions)
{
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, regions);
  m_renderRegion.SetRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
}

void CGUIWindowScreensaver::Render()
{
  if (m_struct.toAddon.Render)
  {
    g_graphicsContext.CaptureStateBlock();
    m_struct.toAddon.Render(m_addonInstance);
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
      // notify screen saver that they should stop
      if (m_struct.toAddon.Stop)
        m_struct.toAddon.Stop(m_addonInstance);

      if (m_addon)
      {
        m_addon->DestroyInstance(m_addon->ID());
        CAddonMgr::GetInstance().ReleaseAddon(m_addon, this);
      }

      memset(&m_struct, 0, sizeof(m_struct));
      m_addonInstance = nullptr;

      g_graphicsContext.ApplyStateBlock();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);

      g_graphicsContext.CaptureStateBlock();

      m_addon = CAddonMgr::GetInstance().GetAddon(CServiceBroker::GetSettings().GetString(CSettings::SETTING_SCREENSAVER_MODE), this);
      if (!m_addon)
        return false;

      // Setup new screensaver instance
      m_name = m_addon->Name();
      m_presets = CSpecialProtocol::TranslatePath(m_addon->Path());
      m_profile = CSpecialProtocol::TranslatePath(m_addon->Profile());

    #ifdef HAS_DX
      m_struct.props.device = g_Windowing.Get3D11Context();
    #else
      m_struct.props.device = nullptr;
    #endif
      m_struct.props.x = 0;
      m_struct.props.y = 0;
      m_struct.props.width = g_graphicsContext.GetWidth();
      m_struct.props.height = g_graphicsContext.GetHeight();
      m_struct.props.pixelRatio = g_graphicsContext.GetResInfo().fPixelRatio;
      m_struct.props.name = m_name.c_str();
      m_struct.props.presets = m_presets.c_str();
      m_struct.props.profile = m_profile.c_str();
      m_struct.toKodi.kodiInstance = this;

      if (m_addon->CreateInstance(ADDON_INSTANCE_SCREENSAVER, m_addon->ID(), &m_struct, reinterpret_cast<KODI_HANDLE*>(&m_addonInstance)) != ADDON_STATUS_OK || !m_struct.toAddon.Start)
        return false;

      // notify screen saver that they should start
      return m_struct.toAddon.Start(m_addonInstance);
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
