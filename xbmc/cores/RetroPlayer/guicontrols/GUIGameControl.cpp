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
#include "GUIRenderSettings.h"
#include "cores/RetroPlayer/rendering/GUIGameRenderManager.h"
#include "cores/RetroPlayer/rendering/GUIRenderHandle.h"
#include "cores/RetroPlayer/rendering/RenderGeometry.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
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
  CGUIControl(parentID, controlID, posX, posY, width, height),
  m_renderSettings(new CGUIRenderSettings(*this))
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAME;

  m_renderSettings->SetGeometry(CRenderGeometry(CRect(CPoint(posX, posY), CSize(width, height))));

  RegisterControl();
}

CGUIGameControl::CGUIGameControl(const CGUIGameControl &other) :
  CGUIControl(other),
  m_scalingMethodInfo(other.m_scalingMethodInfo),
  m_viewModeInfo(other.m_viewModeInfo),
  m_bHasScalingMethod(other.m_bHasScalingMethod),
  m_bHasViewMode(other.m_bHasViewMode),
  m_renderSettings(new CGUIRenderSettings(*this))
{
  m_renderSettings->SetSettings(other.m_renderSettings->GetSettings());

  RegisterControl();
}

CGUIGameControl::~CGUIGameControl()
{
  UnregisterControl();
}

void CGUIGameControl::SetScalingMethod(const CGUIInfoLabel &scalingMethod)
{
  m_scalingMethodInfo = scalingMethod;
}

void CGUIGameControl::SetViewMode(const CGUIInfoLabel &viewMode)
{
  m_viewModeInfo = viewMode;
}

IGUIRenderSettings *CGUIGameControl::GetRenderSettings() const
{
  return m_renderSettings.get();
}

void CGUIGameControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed
  if (m_renderHandle->IsDirty())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIGameControl::Render()
{
  m_renderHandle->Render();

  CGUIControl::Render();
}

void CGUIGameControl::RenderEx()
{
  m_renderHandle->RenderEx();

  CGUIControl::RenderEx();
}

bool CGUIGameControl::CanFocus() const
{
  // Unfocusable
  return false;
}

void CGUIGameControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_renderSettings->SetGeometry(CRenderGeometry(CRect(CPoint(posX, posY), CSize(m_width, m_height))));
}

void CGUIGameControl::SetWidth(float width)
{
  CGUIControl::SetWidth(width);
  m_renderSettings->SetGeometry(CRenderGeometry(CRect(CPoint(m_posX, m_posY), CSize(width, m_height))));
}

void CGUIGameControl::SetHeight(float height)
{
  CGUIControl::SetHeight(height);
  m_renderSettings->SetGeometry(CRenderGeometry(CRect(CPoint(m_posX, m_posY), CSize(m_width, height))));
}

void CGUIGameControl::UpdateInfo(const CGUIListItem *item /* = nullptr */)
{
  Reset();

  if (item)
  {
    std::string strScalingMethod = m_scalingMethodInfo.GetItemLabel(item);
    if (StringUtils::IsNaturalNumber(strScalingMethod))
    {
      unsigned int scalingMethod;
      std::istringstream(std::move(strScalingMethod)) >> scalingMethod;
      m_renderSettings->SetScalingMethod(static_cast<ESCALINGMETHOD>(scalingMethod));
      m_bHasScalingMethod = true;
    }

    std::string strViewMode = m_viewModeInfo.GetItemLabel(item);
    if (StringUtils::IsNaturalNumber(strViewMode))
    {
      unsigned int viewMode;
      std::istringstream(std::move(strViewMode)) >> viewMode;
      m_renderSettings->SetViewMode(static_cast<ViewMode>(viewMode));
      m_bHasViewMode = true;
    }
  }
}

void CGUIGameControl::Reset()
{
  m_bHasScalingMethod = false;
  m_bHasViewMode = false;
  m_renderSettings->Reset();
}

void CGUIGameControl::RegisterControl()
{
  m_renderHandle = CServiceBroker::GetGameRenderManager().RegisterControl(*this);
}

void CGUIGameControl::UnregisterControl()
{
  m_renderHandle.reset();
}
