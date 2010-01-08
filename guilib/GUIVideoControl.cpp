/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "GUIVideoControl.h"
#include "GUIWindowManager.h"
#include "MouseStat.h"
#include "Application.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#else
#include "cores/DummyVideoPlayer.h"
#endif

CGUIVideoControl::CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VIDEO;
}

CGUIVideoControl::~CGUIVideoControl(void)
{}


void CGUIVideoControl::Render()
{
#ifdef HAS_VIDEO_PLAYBACK
  // don't render if we aren't playing video, or if the renderer isn't started
  // (otherwise the lock we have from CApplication::Render() may clash with the startup
  // locks in the RenderManager.)
  if (g_application.IsPlayingVideo() && g_renderManager.IsStarted())
  {
#else
  if (g_application.IsPlayingVideo())
  {
#endif
    if (!g_application.m_pPlayer->IsPaused())
      g_application.ResetScreenSaver();

    g_graphicsContext.SetViewWindow(m_posX, m_posY, m_posX + m_width, m_posY + m_height);

#ifdef HAS_VIDEO_PLAYBACK
    color_t alpha = g_graphicsContext.MergeAlpha(0xFF000000) >> 24;
    g_renderManager.RenderUpdate(false, 0, alpha);
#else
    ((CDummyVideoPlayer *)g_application.m_pPlayer)->Render();
#endif
  }
  CGUIControl::Render();
}

bool CGUIVideoControl::OnMouseClick(int button, const CPoint &point)
{ // mouse has clicked in the video control
  // switch to fullscreen video
  if (!g_application.IsPlayingVideo()) return false;
  if (button == MOUSE_LEFT_BUTTON)
  {
    CGUIMessage message(GUI_MSG_FULLSCREEN, GetID(), GetParentID());
    g_windowManager.SendMessage(message);
    return true;
  }
  if (button == MOUSE_RIGHT_BUTTON)
  { // toggle the playlist window
    if (g_windowManager.GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
      g_windowManager.PreviousWindow();
    else
      g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    // reset the mouse button.
    g_Mouse.bClick[MOUSE_RIGHT_BUTTON] = false;
    return true;
  }
  return false;
}

bool CGUIVideoControl::OnMouseOver(const CPoint &point)
{
  // unfocusable, so return false
  CGUIControl::OnMouseOver(point);
  return false;
}

bool CGUIVideoControl::CanFocus() const
{ // unfocusable
  return false;
}

bool CGUIVideoControl::CanFocusFromPoint(const CPoint &point, CPoint &controlPoint) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  controlPoint = point;
  m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
  return HitTest(controlPoint);
}
