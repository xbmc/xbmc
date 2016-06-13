/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "system.h"
#include "GUIVideoControl.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "input/Key.h"
#include "WindowIDs.h"

CGUIVideoControl::CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VIDEO;
}

CGUIVideoControl::~CGUIVideoControl(void)
{}

void CGUIVideoControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed. Just mark always for now.
  if (g_application.m_pPlayer->IsRenderingGuiLayer())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIVideoControl::Render()
{
  if (g_application.m_pPlayer->IsRenderingVideo())
  {
    if (!g_application.m_pPlayer->IsPausedPlayback())
      g_application.ResetScreenSaver();

    g_graphicsContext.SetViewWindow(m_posX, m_posY, m_posX + m_width, m_posY + m_height);
    TransformMatrix mat;
    g_graphicsContext.SetTransform(mat, 1.0, 1.0);

    color_t alpha = g_graphicsContext.MergeAlpha(0xFF000000) >> 24;
    if (g_application.m_pPlayer->IsRenderingVideoLayer())
    {
      CRect old = g_graphicsContext.GetScissors();
      CRect region = GetRenderRegion();
      region.Intersect(old);
      g_graphicsContext.SetScissors(region);
#ifdef HAS_IMXVPU
      g_graphicsContext.Clear((16 << 16)|(8 << 8)|16);
#else
      g_graphicsContext.Clear(0);
#endif
      g_graphicsContext.SetScissors(old);
    }
    else
      g_application.m_pPlayer->Render(false, alpha);

    g_graphicsContext.RemoveTransform();
  }
  //! @todo remove this crap: HAS_VIDEO_PLAYBACK
  //! instantiating a video control having no playback is complete nonsense
  CGUIControl::Render();
}

void CGUIVideoControl::RenderEx()
{
  if (g_application.m_pPlayer->IsRenderingVideo())
    g_application.m_pPlayer->Render(false, 255, false);
  
  CGUIControl::RenderEx();
}

EVENT_RESULT CGUIVideoControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (!g_application.m_pPlayer->IsPlayingVideo()) return EVENT_RESULT_UNHANDLED;
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  { // switch to fullscreen
    CGUIMessage message(GUI_MSG_FULLSCREEN, GetID(), GetParentID());
    g_windowManager.SendMessage(message);
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
