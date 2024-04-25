/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIVideoControl.h"

#include "GUIComponent.h"
#include "GUITexture.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "utils/ColorUtils.h"
#include "windowing/GraphicContext.h"

using namespace KODI;

CGUIVideoControl::CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VIDEO;
}

CGUIVideoControl::~CGUIVideoControl(void) = default;

void CGUIVideoControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed. Just mark always for now.
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsRenderingGuiLayer())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIVideoControl::Render()
{
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
    return;
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsRenderingVideo())
  {
    if (!appPlayer->IsPausedPlayback())
    {
      auto& appComponents = CServiceBroker::GetAppComponents();
      const auto appPower = appComponents.GetComponent<CApplicationPowerHandling>();
      appPower->ResetScreenSaver();
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().SetViewWindow(m_posX, m_posY, m_posX + m_width, m_posY + m_height);
    TransformMatrix mat;
    CServiceBroker::GetWinSystem()->GetGfxContext().SetTransform(mat, 1.0, 1.0);

    UTILS::COLOR::Color alpha =
        CServiceBroker::GetWinSystem()->GetGfxContext().MergeAlpha(0xFF000000) >> 24;
    if (appPlayer->IsRenderingVideoLayer())
    {
      CRect old = CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors();
      CRect region = GetRenderRegion();
      region.Intersect(old);
      CServiceBroker::GetWinSystem()->GetGfxContext().SetScissors(region);

      // with dual pass rendering, we need to "clear" with a quad, as we need to conserve the already rendered layers
      if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
          RENDER_ORDER_BACK_TO_FRONT)
        CGUITexture::DrawQuad(region, 0x00000000, nullptr, nullptr, -1.0f, false);
      else if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
               RENDER_ORDER_ALL_BACK_TO_FRONT)
        CServiceBroker::GetWinSystem()->GetGfxContext().Clear(0);
      CServiceBroker::GetWinSystem()->GetGfxContext().SetScissors(old);
    }
    else
      appPlayer->Render(false, alpha);

    CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
  }
  CGUIControl::Render();
}

void CGUIVideoControl::RenderEx()
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsRenderingVideo())
    appPlayer->Render(false, 255, false);

  CGUIControl::RenderEx();
}

EVENT_RESULT CGUIVideoControl::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingVideo())
    return EVENT_RESULT_UNHANDLED;
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  { // switch to fullscreen
    CGUIMessage message(GUI_MSG_FULLSCREEN, GetID(), GetParentID());
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

bool CGUIVideoControl::CanFocus() const
{ // unfocusable
  return false;
}

bool CGUIVideoControl::CanFocusFromPoint(const CPoint &point) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  return IsVisible() && HitTest(point);
}
