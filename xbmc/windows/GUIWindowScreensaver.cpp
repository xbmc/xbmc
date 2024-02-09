/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowScreensaver.h"

#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/ScreenSaver.h"
#include "addons/addoninfo/AddonType.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUITexture.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

using namespace KODI;

CGUIWindowScreensaver::CGUIWindowScreensaver()
  : CGUIDialog(WINDOW_SCREENSAVER, "", DialogModalityType::MODELESS)
{
  m_renderOrder = RENDER_ORDER_WINDOW_SCREENSAVER;
}

void CGUIWindowScreensaver::Process(unsigned int currentTime, CDirtyRegionList& regions)
{
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, regions);
  const auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();
  m_renderRegion.SetRect(0, 0, static_cast<float>(context.GetWidth()),
                         static_cast<float>(context.GetHeight()));
}

void CGUIWindowScreensaver::Render()
{
  // FIXME/TODO: Screensaver addons should make the screen black instead
  // keeping this just for compatibility reasons since it's now a dialog.
  CGUITexture::DrawQuad(m_renderRegion, UTILS::COLOR::BLACK);

  if (m_addon)
  {
    auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();

    context.CaptureStateBlock();
    m_addon->Render();
    context.ApplyStateBlock();
    return;
  }

  CGUIDialog::Render();
}

void CGUIWindowScreensaver::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  m_visible = true;
}

void CGUIWindowScreensaver::UpdateVisibility()
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  if (!appPower->IsInScreenSaver() && m_visible)
  {
    m_visible = false;
    Close();
  }
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
          CServiceBroker::GetAddonMgr().GetAddonInfo(addon, ADDON::AddonType::SCREENSAVER);
      if (!addonBase)
        return false;
      m_addon = std::make_unique<KODI::ADDONS::CScreenSaver>(addonBase);
      return m_addon->Start();
    }

    case GUI_MSG_CHECK_LOCK:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPower = components.GetComponent<CApplicationPowerHandling>();
      if (!g_passwordManager.IsProfileLockUnlocked())
      {
        appPower->SetScreenSaverLockFailed();
        return false;
      }
      appPower->SetScreenSaverUnlocked();
      return true;
    }
  }

  return CGUIWindow::OnMessage(message);
}
