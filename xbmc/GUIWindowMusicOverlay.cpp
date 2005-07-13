
#include "stdafx.h"
#include "GUIWindowMusicOverlay.h"
#include "util.h"
#include "application.h"
#include "utils/GUIInfoManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CONTROL_LOGO_PIC_BACK    0
#define CONTROL_LOGO_PIC    1
#define CONTROL_PLAYTIME  2
#define CONTROL_PLAY_LOGO   3
#define CONTROL_PAUSE_LOGO  4
#define CONTROL_INFO     5
#define CONTROL_BIG_PLAYTIME 6
#define CONTROL_FF_LOGO  7
#define CONTROL_RW_LOGO  8

#define CONTROL_TITLE  51
#define CONTROL_ALBUM  52
#define CONTROL_ARTIST 53
#define CONTROL_YEAR  54

#define STEPS 25

CGUIWindowMusicOverlay::CGUIWindowMusicOverlay()
    : CGUIWindow(0)
{
}

CGUIWindowMusicOverlay::~CGUIWindowMusicOverlay()
{
}

bool CGUIWindowMusicOverlay::OnMessage(CGUIMessage& message)
{ // check that the mouse wasn't clicked on the thumb...
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetControlId() == GetID() && message.GetSenderId() == 0)
    {
      if (message.GetParam1() == ACTION_SELECT_ITEM)
      { // switch to fullscreen visualisation mode...
        CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, GetID());
        g_graphicsContext.SendMessage(msg);
      }
    }
  }
  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowMusicOverlay::OnMouse()
{
  CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_LOGO_PIC);
  if (pControl && pControl->HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  {
    // send highlight message
    g_Mouse.SetState(MOUSE_STATE_FOCUS);
    if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
    { // send mouse message
      CGUIMessage message(GUI_MSG_FULLSCREEN, CONTROL_LOGO_PIC, GetID());
      g_graphicsContext.SendMessage(message);
      // reset the mouse button
      g_Mouse.bClick[MOUSE_LEFT_BUTTON] = false;
    }
    if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
    { // toggle the playlist window
      if (m_gWindowManager.GetActiveWindow() == WINDOW_MUSIC_PLAYLIST)
        m_gWindowManager.PreviousWindow();
      else
        m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
      // reset it so that we don't call other actions
      g_Mouse.bClick[MOUSE_RIGHT_BUTTON] = false;
    }
    return true;
  }
  else
  {
    return CGUIWindow::OnMouse();
  }
}

void CGUIWindowMusicOverlay::Render()
{
  if (!g_application.m_pPlayer) return ;
  if ( g_application.m_pPlayer->HasVideo()) return ;
  CGUIWindow::Render();
}

