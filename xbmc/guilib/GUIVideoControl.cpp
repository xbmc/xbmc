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
#include "cores/VideoRenderers/RenderManager.h"
#ifndef HAS_VIDEO_PLAYBACK
#include "cores/DummyVideoPlayer.h"
#endif

CGUIVideoControl::CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VIDEO;
}

CGUIVideoControl::~CGUIVideoControl(void)
{}

void CGUIVideoControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  g_renderManager.FrameMove();

  // TODO Proper processing which marks when its actually changed. Just mark always for now.
  if (g_renderManager.IsGuiLayer())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIVideoControl::Render()
{
#ifdef HAS_VIDEO_PLAYBACK
  // don't render if we aren't playing video, or if the renderer isn't started
  // (otherwise the lock we have from CApplication::Render() may clash with the startup
  // locks in the RenderManager.)
  if (g_application.m_pPlayer->IsPlayingVideo() && g_renderManager.IsStarted())
  {
#else
  if (g_application.m_pPlayer->IsPlayingVideo())
  {
#endif
    if (!g_application.m_pPlayer->IsPausedPlayback())
      g_application.ResetScreenSaver();

    g_graphicsContext.SetViewWindow(m_posX, m_posY, m_posX + m_width, m_posY + m_height);
    TransformMatrix mat;
    g_graphicsContext.SetTransform(mat, 1.0, 1.0);

#ifdef HAS_VIDEO_PLAYBACK
    color_t alpha = g_graphicsContext.MergeAlpha(0xFF000000) >> 24;
    if (g_renderManager.IsVideoLayer())
    {
      CRect old = g_graphicsContext.GetScissors();
      CRect region = GetRenderRegion();
      region.Intersect(old);
      g_graphicsContext.BeginPaint();
      g_graphicsContext.SetScissors(region);
#ifdef HAS_IMXVPU
      g_graphicsContext.Clear((16 << 16)|(8 << 8)|16);
#else
      g_graphicsContext.Clear(0);
#endif
      g_graphicsContext.SetScissors(old);
      g_graphicsContext.EndPaint();
    }
    else
      g_renderManager.Render(false, 0, alpha);
#else
    ((CDummyVideoPlayer *)(g_application.m_pPlayer->GetInternal()).get())->Render();
#endif

    g_graphicsContext.RemoveTransform();
  }
  // TODO: remove this crap: HAS_VIDEO_PLAYBACK
  // instantiating a video control having no playback is complete nonsense
  CGUIControl::Render();
}

void CGUIVideoControl::RenderEx()
{
#ifdef HAS_VIDEO_PLAYBACK
  if (g_application.m_pPlayer->IsPlayingVideo() && g_renderManager.IsStarted())
    g_renderManager.Render(false, 0, 255, false);
  g_renderManager.FrameFinish();
#endif
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
  else if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  { // toggle the playlist window
    if (g_windowManager.GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
      g_windowManager.PreviousWindow();
    else
      g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
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
