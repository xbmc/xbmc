/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowMusicOverlay.h"
#include "utils/GUIInfoManager.h"

#define CONTROL_LOGO_PIC    1

CGUIWindowMusicOverlay::CGUIWindowMusicOverlay()
    : CGUIDialog(WINDOW_MUSIC_OVERLAY, "MusicOverlay.xml")
{
  m_renderOrder = 0;
  m_visibleCondition = SKIN_HAS_MUSIC_OVERLAY;
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
  return CGUIDialog::OnMessage(message);
}

bool CGUIWindowMusicOverlay::OnMouse(const CPoint &point)
{
  CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_LOGO_PIC);
  if (pControl && pControl->HitTest(point))
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
    return CGUIDialog::OnMouse(point);
  }
}

void CGUIWindowMusicOverlay::Render()
{
  CGUIDialog::Render();
}

void CGUIWindowMusicOverlay::SetDefaults()
{
  CGUIDialog::SetDefaults();
  m_renderOrder = 0;
  m_visibleCondition = SKIN_HAS_MUSIC_OVERLAY;
}

