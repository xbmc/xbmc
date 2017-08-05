/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GUIGameControl.h"
#include "cores/RetroPlayer/rendering/GUIRenderSettings.h"
#include "games/GameServices.h"
#include "guilib/Geometry.h"
#include "guilib/GraphicContext.h"
#include "guilib/TransformMatrix.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "Application.h"
#include "ApplicationPlayer.h"
#include "ServiceBroker.h"

#include <sstream>

using namespace KODI;
using namespace RETRO;

CGUIGameControl::CGUIGameControl(int parentID, int controlID, float posX, float posY, float width, float height) :
  CGUIControl(parentID, controlID, posX, posY, width, height)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAME;
}

void CGUIGameControl::SetViewMode(const CGUIInfoLabel &viewMode)
{
  m_viewModeInfo = viewMode;
}

void CGUIGameControl::SetVideoFilter(const CGUIInfoLabel &videoFilter)
{
  m_videoFilterInfo = videoFilter;
}

void CGUIGameControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed. Just mark always for now.
  if (g_application.m_pPlayer->IsRenderingGuiLayer())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIGameControl::Render()
{
  if (g_application.m_pPlayer->IsRenderingVideo())
  {
    // Set fullscreen
    const bool bFullscreen = g_graphicsContext.IsFullScreenVideo();
    if (bFullscreen)
      g_graphicsContext.SetFullScreenVideo(false);

    // Enable GUI render settings
    EnableGUIRender();

    // Set coordinates
    g_graphicsContext.SetViewWindow(m_posX, m_posY, m_posX + m_width, m_posY + m_height);
    TransformMatrix mat;
    g_graphicsContext.SetTransform(mat, 1.0, 1.0);

    // Clear render area
    CRect old = g_graphicsContext.GetScissors();
    CRect region = GetRenderRegion();
    region.Intersect(old);
    g_graphicsContext.SetScissors(region);
    g_graphicsContext.Clear(0);
    g_graphicsContext.SetScissors(old);

    // Render
    color_t alpha = g_graphicsContext.MergeAlpha(0xFF000000) >> 24;
    g_application.m_pPlayer->Render(false, alpha);

    // Restore coordinates
    g_graphicsContext.RemoveTransform();

    // Disable GUI render settings
    DisableGUIRender();

    // Restore fullscreen
    if (bFullscreen)
      g_graphicsContext.SetFullScreenVideo(true);
  }

  CGUIControl::Render();
}

void CGUIGameControl::RenderEx()
{
  if (g_application.m_pPlayer->IsRenderingVideo())
  {
    EnableGUIRender();
    g_application.m_pPlayer->Render(false, 255, false);
    DisableGUIRender();
  }
  
  CGUIControl::RenderEx();
}

bool CGUIGameControl::CanFocus() const
{
  // Unfocusable
  return false;
}

void CGUIGameControl::UpdateInfo(const CGUIListItem *item /* = nullptr */)
{
  m_viewMode = -1;
  m_scalingMethod = -1;

  if (item)
  {
    std::string strViewMode = m_viewModeInfo.GetItemLabel(item);
    if (StringUtils::IsNaturalNumber(strViewMode))
      std::istringstream(std::move(strViewMode)) >> m_viewMode;

    std::string strVideoFilter = m_videoFilterInfo.GetItemLabel(item);
    if (StringUtils::IsNaturalNumber(strVideoFilter))
      std::istringstream(std::move(strVideoFilter)) >> m_scalingMethod;
  }
}

void CGUIGameControl::EnableGUIRender()
{
  CGUIRenderSettings &renderSettings = CServiceBroker::GetGameServices().RenderSettings();
  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  renderSettings.EnableGuiRenderSettings(true);

  // Set view mode
  if (m_viewMode >= 0)
    renderSettings.SetRenderViewMode(static_cast<ViewMode>(m_viewMode));
  else
    renderSettings.SetRenderViewMode(gameSettings.ViewMode());

  // Set scaling method
  if (m_scalingMethod >= 0)
    renderSettings.SetScalingMethod(static_cast<ESCALINGMETHOD>(m_scalingMethod));
  else
    renderSettings.SetScalingMethod(gameSettings.ScalingMethod());
}

void CGUIGameControl::DisableGUIRender()
{
  CGUIRenderSettings &renderSettings = CServiceBroker::GetGameServices().RenderSettings();
  renderSettings.EnableGuiRenderSettings(false);
}
