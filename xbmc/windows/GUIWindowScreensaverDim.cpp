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

#include "GUIWindowScreensaverDim.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "utils/Color.h"
#include "windowing/GraphicContext.h"
#include "guilib/GUITexture.h"

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
  if (g_application.IsInScreenSaver())
  {
    if (m_visible)
      return;

    std::string usedId = g_application.ScreensaverIdInUse();
    if  (usedId == "screensaver.xbmc.builtin.dim" ||
         usedId == "screensaver.xbmc.builtin.black")
    {
      m_visible = true;
      ADDON::AddonPtr info;
      CServiceBroker::GetAddonMgr().GetAddon(usedId, info, ADDON::ADDON_SCREENSAVER);
      if (info && !info->GetSetting("level").empty())
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
  UTILS::Color color = (static_cast<UTILS::Color>(m_dimLevel * 2.55f) & 0xff) << 24;
  color = CServiceBroker::GetWinSystem()->GetGfxContext().MergeAlpha(color);
  CRect rect(0, 0, (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
  CGUITexture::DrawQuad(rect, color);
  CGUIDialog::Render();
}
