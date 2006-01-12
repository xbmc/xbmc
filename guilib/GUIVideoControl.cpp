#include "include.h"
#include "GUIVideoControl.h"
#include "GUIWindowManager.h"
#include "../xbmc/Application.h"
#include "../xbmc/cores/VideoRenderers/RenderManager.h"

CGUIVideoControl::CGUIVideoControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{}

CGUIVideoControl::~CGUIVideoControl(void)
{}


void CGUIVideoControl::Render()
{
  if (!UpdateEffectState()) return ;

  // don't render if we aren't playing video, or if the renderer isn't started
  // (otherwise the lock we have from CApplication::Render() may clash with the startup
  // locks in the RenderManager.)
  if (g_application.IsPlayingVideo() && g_renderManager.IsStarted())
  {
    if (!g_application.m_pPlayer->IsPaused())
      g_application.ResetScreenSaver();

    RECT rc;
    rc.left = m_iPosX;
    rc.top = m_iPosY;
    rc.right = rc.left + m_dwWidth;
    rc.bottom = rc.top + m_dwHeight;
    g_graphicsContext.SetViewWindow(rc);

    g_renderManager.RenderUpdate(false);
  }
  CGUIControl::Render();
}

void CGUIVideoControl::OnMouseClick(DWORD dwButton)
{ // mouse has clicked in the video control
  // switch to fullscreen video
  if (dwButton == MOUSE_LEFT_BUTTON)
  {
    CGUIMessage message(GUI_MSG_FULLSCREEN, GetID(), GetParentID());
    g_graphicsContext.SendMessage(message);
  }
  if (dwButton == MOUSE_RIGHT_BUTTON)
  { // toggle the playlist window
    if (m_gWindowManager.GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
      m_gWindowManager.PreviousWindow();
    else
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    // reset the mouse button.
    g_Mouse.bClick[MOUSE_RIGHT_BUTTON] = false;
  }
}
