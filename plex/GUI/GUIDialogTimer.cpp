/*
 *  GUIDialogTimer.cpp
 *  Plex
 *
 *  Created by James Clarke on 18/10/2009.
 *  Copyright 2009 Plex Incorporated. All rights reserved.
 *
 */

#include "GUIDialogTimer.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "GUIImage.h"
#include "Key.h"
#include <boost/lexical_cast.hpp>
#include "Util.h"
#include "PlexTypes.h"
#include "threads/SingleLock.h"

#define TIMER_MAX 720

using namespace std;

CGUIDialogTimer::CGUIDialogTimer(void)
: CGUIDialog(WINDOW_DIALOG_TIMER, "DialogTimer.xml")
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_iTime = 0;
  m_typedString = "";
}

CGUIDialogTimer::~CGUIDialogTimer(void)
{
}

/* Convenience functions */

void CGUIDialogTimer::SendGUIMessage(CGUIMessage msg)
{
  CSingleTryLock tryLock(g_graphicsContext);

  if(tryLock.IsOwner())
    CGUIDialog::OnMessage(msg);
  else
    g_windowManager.SendThreadMessage(msg, GetID());  
}

void CGUIDialogTimer::SetControlLabel(int controlId, CStdString str)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), controlId);
  msg.SetLabel(str);
  SendGUIMessage(msg);
}

void CGUIDialogTimer::SetControlLabel(int controlId, int str)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), controlId);
  msg.SetLabel(str);
  SendGUIMessage(msg);
}


void CGUIDialogTimer::ShowControl(int controlId)
{
  CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), controlId);
  SendGUIMessage(msg);
}

void CGUIDialogTimer::HideControl(int controlId)
{
  CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), controlId);
  SendGUIMessage(msg);
}


/* Key handling */

bool CGUIDialogTimer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    // Intercept remote buttons for direct input
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
    {
      ShowControl(100);
      
      m_typedString.Format("%s%d", m_typedString.c_str(), action.GetID() - REMOTE_0);
      CStdString m_labelString = m_typedString;
      while (m_labelString.length() < 4) m_labelString = "-" + m_labelString;
      SetControlLabel(21, m_labelString.substr(0, 2));
      SetControlLabel(22, m_labelString.substr(2, 2));
      if (m_typedString.length() == 4)
      {
        ConvertTypedTextToTime();
      }
      else
      {
        m_typingStopWatch.StartZero();
        HideControl(101);
      }
      return true;
    }
      
    // Increase/decrease the timer on up/down (5 minute increments, between 0 and TIMER_MAX minutes)
    case ACTION_MOVE_UP:
    {
      if (m_iTime < TIMER_MAX)
      {
        m_typedString = "";
        m_iTime = ((m_iTime / 5) + 1) * 5;
        UpdateLabels();
      }
      return true;
    }
    case ACTION_MOVE_DOWN:
    {
      if (m_iTime > 0)
      {
        m_typedString = "";
        m_iTime = ((m_iTime / 5) - 1) * 5;
        UpdateLabels();
      }
      return true;
    }
    case ACTION_SELECT_ITEM:
    {
      ConvertTypedTextToTime(); // Make sure we grab any text entered so far - this will abort if the user is not currently inputting.
      m_bConfirmed = true;
      Close(false);
      return true;
    }
  }
  return CGUIDialog::OnAction(action);
}


void CGUIDialogTimer::DoModal(int iHeadLabel, int iInfoLabel, int iTime)
{
  // Set up the dialog with the supplied values
  m_typedString = "";
  m_iTime = iTime;
  SetControlLabel(1, iHeadLabel);
  SetControlLabel(2, iHeadLabel);
  SetControlLabel(30, iInfoLabel);
  if (m_iTime > 0)
  {
    HideControl(100);
    HideControl(101);
  }
  m_bConfirmed = false;
  UpdateLabels();
  CGUIDialog::DoModal();
}

void CGUIDialogTimer::Render()
{
  // Check if the final time has changed - if so, update the label
  int time = m_iTime;
  if (time == 0) time = 5; // Fake this value in the UI for animating between time cell & off states.
  CDateTime dateTime = CDateTime::GetCurrentDateTime();;
  dateTime += CDateTimeSpan(0, 0, time, 0);
  if (m_lastDateTime != dateTime)
  {
#ifdef _MSC_VER
#pragma message(__WARNING__"do we need to support the short time format here? TIME_FORMAT_SHORT")
#else
#pragma warning do we need to support the short time format here? TIME_FORMAT_SHORT
#endif
    CStdString strTime = g_infoManager.LocalizeTime(dateTime, TIME_FORMAT_HH_MM_SS);
    SetControlLabel(31, strTime);
    m_lastDateTime = dateTime;
  }
  
  if (m_typingStopWatch.IsRunning() && m_typingStopWatch.GetElapsedSeconds() >= 3)
  {
    m_typingStopWatch.Stop();
    ConvertTypedTextToTime();
  }

  CGUIDialog::Render();
}


/* Static method that displays the dialog with the supplied values, and returns the user selected value */

int CGUIDialogTimer::ShowAndGetInput(int iHeadLabel, int iInfoLabel, int iTime)
{
  CGUIDialogTimer *dialog = (CGUIDialogTimer *)g_windowManager.GetWindow(WINDOW_DIALOG_TIMER);
  if (!dialog) return -1;
  
  dialog->DoModal(iHeadLabel, iInfoLabel, iTime);
  if (dialog->m_bConfirmed)
  {
    return dialog->GetTime();
  }
  else
  {
    return -1;
  }
}

void CGUIDialogTimer::UpdateLabels()
{
  if (m_iTime == 0)
    HideControl(100);
  else
  {
    ShowControl(100);
    ShowControl(101);
  }
  
  int time = m_iTime;
  if (time == 0) time = 5; // Fake this value in the UI for animating between time cell & off states.
  CStdString strHours;
  CStdString strMinutes;
  strHours.Format("%02d", time / 60);
  strMinutes.Format("%02d", time % 60);
  SetControlLabel(21, strHours);
  SetControlLabel(22, strMinutes);
  m_typedString = "";
}

/* Convert a user entered string of numbers into a time value */

void CGUIDialogTimer::ConvertTypedTextToTime()
{
  if (m_typedString.length() == 0) return;
  while (m_typedString.length() < 4) m_typedString = "0" + m_typedString;
  string strHours = m_typedString.substr(0, 2);
  string strMinutes = m_typedString.substr(2, 2);
  m_iTime = (boost::lexical_cast<int>(strHours) * 60) + boost::lexical_cast<int>(strMinutes);
  if (m_iTime > TIMER_MAX) m_iTime = TIMER_MAX;
  ShowControl(101);
  UpdateLabels();
}
