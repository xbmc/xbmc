#include "include.h"
#include "GUIVideoControl.h"
#include "GUIWindowManager.h"
#include "../xbmc/Application.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "../xbmc/cores/VideoRenderers/RenderManager.h"
#else
#include "../xbmc/cores/DummyVideoPlayer.h"
#endif

CGUIVideoControl::CGUIVideoControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{}

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
    g_renderManager.RenderUpdate(false);
#else
    ((CDummyVideoPlayer *)g_application.m_pPlayer)->Render();
#endif
  }
  CGUIControl::Render();
}

bool CGUIVideoControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{ // mouse has clicked in the video control
  // switch to fullscreen video
  if (!g_application.IsPlayingVideo()) return false;
  if (dwButton == MOUSE_LEFT_BUTTON)
  {
    CGUIMessage message(GUI_MSG_FULLSCREEN, GetID(), GetParentID());
    g_graphicsContext.SendMessage(message);
    return true;
  }
  if (dwButton == MOUSE_RIGHT_BUTTON)
  { // toggle the playlist window
    if (m_gWindowManager.GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
      m_gWindowManager.PreviousWindow();
    else
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    // reset the mouse button.
    g_Mouse.bClick[MOUSE_RIGHT_BUTTON] = false;
    return true;
  }
  return false;
}

bool CGUIVideoControl::OnMouseOver(const CPoint &point)
{
  // unfocusable, so return true
  CGUIControl::OnMouseOver(point);
  return true;
}

bool CGUIVideoControl::CanFocus() const
{ // unfocusable
  return false;
}

bool CGUIVideoControl::CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  controlPoint = point;
  m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
  if (HitTest(controlPoint))
  {
    *control = (CGUIControl *)this;
    return true;
  }
  *control = NULL;
  return false;
}
