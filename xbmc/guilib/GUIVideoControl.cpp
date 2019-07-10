/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIVideoControl.h"

#include "Application.h"
#include "GUIComponent.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "WindowIDs.h"
#include "input/Key.h"
#include "utils/Color.h"

CGUIVideoControl::CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VIDEO;
}

CGUIVideoControl::~CGUIVideoControl(void) = default;

void CGUIVideoControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed. Just mark always for now.
  if (g_application.GetAppPlayer().IsRenderingGuiLayer())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIVideoControl::Render()
{
  if (g_application.GetAppPlayer().IsRenderingVideo())
  {
    if (!g_application.GetAppPlayer().IsPausedPlayback())
      g_application.ResetScreenSaver();

    CServiceBroker::GetWinSystem()->GetGfxContext().SetViewWindow(m_posX, m_posY, m_posX + m_width, m_posY + m_height);
    TransformMatrix mat;
    CServiceBroker::GetWinSystem()->GetGfxContext().SetTransform(mat, 1.0, 1.0);

    UTILS::Color alpha = CServiceBroker::GetWinSystem()->GetGfxContext().MergeAlpha(0xFF000000) >> 24;
    if (g_application.GetAppPlayer().IsRenderingVideoLayer())
    {
      CRect old = CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors();
      CRect region = GetRenderRegion();
      region.Intersect(old);
      CServiceBroker::GetWinSystem()->GetGfxContext().SetScissors(region);
      CServiceBroker::GetWinSystem()->GetGfxContext().Clear(0);
      CServiceBroker::GetWinSystem()->GetGfxContext().SetScissors(old);
    }
    else
      g_application.GetAppPlayer().Render(false, alpha);

    CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
  }
  CGUIControl::Render();
}

void CGUIVideoControl::RenderEx()
{
  if (g_application.GetAppPlayer().IsRenderingVideo())
    g_application.GetAppPlayer().Render(false, 255, false);

  CGUIControl::RenderEx();
}

EVENT_RESULT CGUIVideoControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (!g_application.GetAppPlayer().IsPlayingVideo()) return EVENT_RESULT_UNHANDLED;
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
