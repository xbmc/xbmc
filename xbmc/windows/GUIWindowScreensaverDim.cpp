/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowScreensaverDim.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUITexture.h"
#include "utils/ColorUtils.h"
#include "windowing/GraphicContext.h"

CGUIWindowScreensaverDim::CGUIWindowScreensaverDim(void)
  : CGUIDialog(WINDOW_SCREENSAVER_DIM, "", DialogModalityType::MODELESS)
{
  m_needsScaling = false;
  m_animations.push_back(CAnimation::CreateFader(0, 100, 0, 1000, ANIM_TYPE_WINDOW_OPEN));
  m_animations.push_back(CAnimation::CreateFader(100, 0, 0, 1000, ANIM_TYPE_WINDOW_CLOSE));
  m_renderOrder = RENDER_ORDER_WINDOW_SCREENSAVER;
}

void CGUIWindowScreensaverDim::UpdateVisibility()
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  if (appPower->IsInScreenSaver())
  {
    if (m_visible)
      return;

    std::string usedId = appPower->ScreensaverIdInUse();
    if  (usedId == "screensaver.xbmc.builtin.dim" ||
         usedId == "screensaver.xbmc.builtin.black")
    {
      m_visible = true;
      ADDON::AddonPtr info;
      bool success = CServiceBroker::GetAddonMgr().GetAddon(
          usedId, info, ADDON::AddonType::SCREENSAVER, ADDON::OnlyEnabled::CHOICE_YES);
      if (success && info && !info->GetSetting("level").empty())
        m_newDimLevel = 100.0f - (float)atof(info->GetSetting("level").c_str());
      else
        m_newDimLevel = 100.0f;
      Open();
    }
  }
  else if (m_visible)
  {
    m_visible = false;
    Close();
  }
}

void CGUIWindowScreensaverDim::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_newDimLevel != m_dimLevel && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
    m_dimLevel = m_newDimLevel;
  CGUIDialog::Process(currentTime, dirtyregions);
  m_renderRegion.SetRect(0, 0, (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
}

void CGUIWindowScreensaverDim::Render()
{
  // draw a translucent black quad - fading is handled by the window animation
  UTILS::COLOR::Color color = (static_cast<UTILS::COLOR::Color>(m_dimLevel * 2.55f) & 0xff) << 24;
  color = CServiceBroker::GetWinSystem()->GetGfxContext().MergeAlpha(color);
  CRect rect(0, 0, (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
  CGUITexture::DrawQuad(rect, color);
  CGUIDialog::Render();
}
