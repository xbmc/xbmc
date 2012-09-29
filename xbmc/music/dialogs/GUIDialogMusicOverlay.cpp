/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogMusicOverlay.h"
#include "guilib/GUIWindowManager.h"
#include "input/MouseStat.h"

#define CONTROL_LOGO_PIC    1

CGUIDialogMusicOverlay::CGUIDialogMusicOverlay()
    : CGUIDialog(WINDOW_DIALOG_MUSIC_OVERLAY, "MusicOverlay.xml")
{
  m_renderOrder = 0;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogMusicOverlay::~CGUIDialogMusicOverlay()
{
}

bool CGUIDialogMusicOverlay::OnMessage(CGUIMessage& message)
{ // check that the mouse wasn't clicked on the thumb...
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetControlId() == GetID() && message.GetSenderId() == 0)
    {
      if (message.GetParam1() == ACTION_SELECT_ITEM)
      { // switch to fullscreen visualisation mode...
        CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, GetID());
        g_windowManager.SendMessage(msg);
      }
    }
  }
  return CGUIDialog::OnMessage(message);
}

EVENT_RESULT CGUIDialogMusicOverlay::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_LOGO_PIC);
  if (pControl && pControl->HitTest(point))
  {
    // send highlight message
    g_Mouse.SetState(MOUSE_STATE_FOCUS);
    if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
    { // send mouse message
      CGUIMessage message(GUI_MSG_FULLSCREEN, CONTROL_LOGO_PIC, GetID());
      g_windowManager.SendMessage(message);
    }
    if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
    { // toggle the playlist window
      if (g_windowManager.GetActiveWindow() == WINDOW_MUSIC_PLAYLIST)
        g_windowManager.PreviousWindow();
      else
        g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUIDialogMusicOverlay::SetDefaults()
{
  CGUIDialog::SetDefaults();
  m_renderOrder = 0;
  SetVisibleCondition("skin.hasmusicoverlay");
}

