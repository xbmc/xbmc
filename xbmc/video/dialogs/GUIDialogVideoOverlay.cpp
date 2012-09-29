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

#include "GUIDialogVideoOverlay.h"
#include "guilib/GUIWindowManager.h"

#define CONTROL_PLAYTIME     2
#define CONTROL_PLAY_LOGO    3
#define CONTROL_PAUSE_LOGO   4
#define CONTROL_INFO         5
#define CONTROL_BIG_PLAYTIME 6
#define CONTROL_FF_LOGO      7
#define CONTROL_RW_LOGO      8


CGUIDialogVideoOverlay::CGUIDialogVideoOverlay()
    : CGUIDialog(WINDOW_DIALOG_VIDEO_OVERLAY, "VideoOverlay.xml")
{
  m_renderOrder = 0;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogVideoOverlay::~CGUIDialogVideoOverlay()
{}

void CGUIDialogVideoOverlay::FrameMove()
{
  if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  { // close immediately
    Close(true);
    return;
  }
  CGUIDialog::FrameMove();
}

EVENT_RESULT CGUIDialogVideoOverlay::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  return EVENT_RESULT_UNHANDLED;
}

void CGUIDialogVideoOverlay::SetDefaults()
{
  CGUIDialog::SetDefaults();
  m_renderOrder = 0;
  SetVisibleCondition("skin.hasvideooverlay");
}

