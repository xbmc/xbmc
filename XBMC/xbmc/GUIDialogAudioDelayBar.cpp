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

#include "stdafx.h"
#include "GUIDialogAudioDelayBar.h"
#include "GUISliderControl.h"

#define DELAY_BAR_DISPLAY_TIME 1000L

CGUIDialogAudioDelayBar::CGUIDialogAudioDelayBar(void)
    : CGUIDialog(WINDOW_DIALOG_AUDIO_DELAY_BAR, "DialogAudioDelayBar.xml")
{
  m_loadOnDemand = false;
}

CGUIDialogAudioDelayBar::~CGUIDialogAudioDelayBar(void)
{}

bool CGUIDialogAudioDelayBar::OnAction(const CAction &action)
{
  if (action.wID == ACTION_AUDIO_DELAY_MIN || action.wID == ACTION_AUDIO_DELAY_PLUS)
  { // reset the timer, as we've changed the delay
    ResetTimer();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogAudioDelayBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      //resources are allocated in g_application
      CGUIDialog::OnMessage(message);
      ResetTimer();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      //don't deinit, g_application handles it
      return CGUIDialog::OnMessage(message);
    }
    break;
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogAudioDelayBar::ResetTimer()
{
  m_dwTimer = timeGetTime();
}

void CGUIDialogAudioDelayBar::Render()
{
  // and render the controls
  CGUIDialog::Render();
  // now check if we should exit
  if (timeGetTime() - m_dwTimer > DELAY_BAR_DISPLAY_TIME)
  {
    Close();
  }
}

