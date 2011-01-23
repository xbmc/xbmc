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
#include "GUISliderControl.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"
#include "utils/TimeUtils.h"
#include "StringUtils.h"
#include "FileItem.h"
#include "GUISettings.h"

#define SEEK_BAR_DISPLAY_TIME 2000L
#define SEEK_BAR_SEEK_TIME     500L

#define POPUP_SEEK_SLIDER       401
#define POPUP_SEEK_LABEL        402

CGUIDialogSeekBar::CGUIDialogSeekBar(void)
    : CGUIDialog(WINDOW_DIALOG_SEEK_BAR, "DialogSeekBar.xml")
{
  m_fSeekPercentage = 0.0f;
  m_bRequireSeek = false;
  m_loadOnDemand = false;    // the application class handles our resources
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void)
{
}

bool CGUIDialogSeekBar::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_ANALOG_SEEK_FORWARD || action.GetID() == ACTION_ANALOG_SEEK_BACK)
  {
    if (!m_bRequireSeek)
    { // start of seeking

      if (g_infoManager.GetTotalPlayTime())
        m_fSeekPercentage = (float)g_infoManager.GetPlayTime() / g_infoManager.GetTotalPlayTime() * 0.1f;
      else
        m_fSeekPercentage = 0.0f;

      // tell info manager that we have started a seekbar operation
      m_bRequireSeek = true;
      g_infoManager.SetSeeking(true);
    }

    // calculate our seek amount
    if (g_application.m_pPlayer && !g_infoManager.m_performingSeek)
    {
      //100% over 1 second.
      float speed = 100.0f;
      if( action.GetRepeat() )
        speed *= action.GetRepeat();
      else
        speed /= g_infoManager.GetFPS();

      if (action.GetID() == ACTION_ANALOG_SEEK_FORWARD)
        m_fSeekPercentage += action.GetAmount() * action.GetAmount() * speed;
      else
        m_fSeekPercentage -= action.GetAmount() * action.GetAmount() * speed;
      if (m_fSeekPercentage > 100.0f) m_fSeekPercentage = 100.0f;
      if (m_fSeekPercentage < 0.0f) m_fSeekPercentage = 0.0f;
      CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
      if (pSlider) pSlider->SetPercentage((int)m_fSeekPercentage);   // Update our seek bar accordingly
    }

    ResetTimer();
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
    { // new song started while our window is up - update our details

      m_bRequireSeek = false;
      m_fSeekPercentage = 0.0f;

    }
    break;

  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogSeekBar::ResetTimer()
{
  m_timer = CTimeUtils::GetFrameTime();
}

void CGUIDialogSeekBar::FrameMove()
{
  if (!g_application.m_pPlayer)
  {
    Close(true);
    return;
  }

  // check if we should seek or exit
  if (!g_infoManager.m_performingSeek && CTimeUtils::GetFrameTime() - m_timer > SEEK_BAR_DISPLAY_TIME)
    g_infoManager.SetSeeking(false);

  // render our controls
  if (!m_bRequireSeek && !g_infoManager.m_performingSeek)
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
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(GetSeekTimeLabel());
    OnMessage(msg);
  }

  // Check for seek timeout, and perform the seek
  if (m_bRequireSeek && CTimeUtils::GetFrameTime() - m_timer > SEEK_BAR_SEEK_TIME)
  {
    g_infoManager.m_performingSeek = true;
    double time = g_infoManager.GetTotalPlayTime() * m_fSeekPercentage * 0.01;
    g_application.SeekTime(time);
    m_bRequireSeek = false;
  }
  CGUIDialog::FrameMove();
}

CStdString CGUIDialogSeekBar::GetSeekTimeLabel(TIME_FORMAT format)
{
  int time = (int)(g_infoManager.GetTotalPlayTime() * m_fSeekPercentage * 0.01f);
  return StringUtils::SecondsToTimeString(time, format);
}

