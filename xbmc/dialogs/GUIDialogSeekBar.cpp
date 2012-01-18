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

#include "GUIDialogSeekBar.h"
#include "guilib/GUISliderControl.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "utils/StringUtils.h"

#define POPUP_SEEK_SLIDER       401
#define POPUP_SEEK_LABEL        402

CGUIDialogSeekBar::CGUIDialogSeekBar(void)
    : CGUIDialog(WINDOW_DIALOG_SEEK_BAR, "DialogSeekBar.xml")
{
  m_loadOnDemand = false;    // the application class handles our resources
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void)
{
}

bool CGUIDialogSeekBar::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_ANALOG_SEEK_FORWARD || action.GetID() == ACTION_ANALOG_SEEK_BACK)
  {
    m_handler.Seek(action.GetID() == ACTION_ANALOG_SEEK_FORWARD, action.GetAmount(), action.GetRepeat());
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSeekBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_DEINIT:
    return CGUIDialog::OnMessage(message);

  case GUI_MSG_LABEL_SET:
    {
      if (message.GetSenderId() == GetID() && message.GetControlId() == POPUP_SEEK_LABEL)
        CGUIDialog::OnMessage(message);
    }
    break;
  case GUI_MSG_PLAYBACK_STARTED:
    { // new song started while our window is up - reset our seek handler
      m_handler.Reset();
    }
    break;

  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogSeekBar::FrameMove()
{
  if (!g_application.m_pPlayer)
  {
    Close(true);
    return;
  }

  // update controls
  if (!m_handler.InProgress() && !g_infoManager.m_performingSeek)
  { // position the bar at our current time
    CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
    if (pSlider && g_infoManager.GetTotalPlayTime())
      pSlider->SetPercentage((int)((float)g_infoManager.GetPlayTime()/g_infoManager.GetTotalPlayTime() * 0.1f));

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(g_infoManager.GetCurrentPlayTime());
    OnMessage(msg);
  }
  else
  {
    CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
    if (pSlider)
      pSlider->SetPercentage((int)m_handler.GetPercent());

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(GetSeekTimeLabel());
    OnMessage(msg);
  }

  // Check for seek timeout, and perform the seek
  m_handler.Process();

  CGUIDialog::FrameMove();
}

CStdString CGUIDialogSeekBar::GetSeekTimeLabel(TIME_FORMAT format)
{
  int time = (int)(g_infoManager.GetTotalPlayTime() * m_handler.GetPercent() * 0.01f);
  return StringUtils::SecondsToTimeString(time, format);
}

