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

#include "GUIDialogProgress.h"
#include "guilib/GUIProgressControl.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace std;

#define CONTROL_CANCEL_BUTTON 10
#define CONTROL_PROGRESS_BAR 20

CGUIDialogProgress::CGUIDialogProgress(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_PROGRESS, "DialogProgress.xml")
{
  m_bCanceled = false;
  m_iCurrent=0;
  m_iMax=0;
  m_percentage = 0;
  m_bCanCancel = true;
}

CGUIDialogProgress::~CGUIDialogProgress(void)
{

}

void CGUIDialogProgress::SetCanCancel(bool bCanCancel)
{
  m_bCanCancel = bCanCancel;
  CGUIMessage msg(bCanCancel ? GUI_MSG_VISIBLE : GUI_MSG_HIDDEN, GetID(), CONTROL_CANCEL_BUTTON);
  if(OwningCriticalSection(g_graphicsContext))
    OnMessage(msg);
  else
    g_windowManager.SendThreadMessage(msg, GetID());
}

void CGUIDialogProgress::StartModal()
{
  CSingleLock lock(g_graphicsContext);

  CLog::DebugLog("DialogProgress::StartModal called %s", m_bRunning ? "(already running)!" : "");
  m_bCanceled = false;

  // set running before it's routed, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_bRunning = true;
  m_bModal = true;
  m_dialogClosing = false;
  g_windowManager.RouteToWindow(this);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);
  ShowProgressBar(false);

  lock.Leave();

  while (m_bRunning && IsAnimating(ANIM_TYPE_WINDOW_OPEN))
  {
    Progress();
    // we should have rendered at least once by now - if we haven't, then
    // we must be running from fullscreen video or similar where the
    // calling thread handles rendering (ie not main app thread) but
    // is waiting on this routine before rendering begins
    if (!m_hasRendered)
      break;
  }
}

void CGUIDialogProgress::Progress()
{
  if (m_bRunning)
  {
    g_windowManager.Process();
  }
}

void CGUIDialogProgress::ProgressKeys()
{
  if (m_bRunning)
  {
    g_application.FrameMove();
  }
}

bool CGUIDialogProgress::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {

  case GUI_MSG_WINDOW_DEINIT:
    SetCanCancel(true);
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_CANCEL_BUTTON && m_bCanCancel)
      {
        string strHeading = m_strHeading;
        strHeading.append(" : ");
        strHeading.append(g_localizeStrings.Get(16024));
        CGUIDialogBoxBase::SetHeading(strHeading);
        m_bCanceled = true;
        return true;
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogProgress::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_CLOSE_DIALOG || action.GetID() == ACTION_PREVIOUS_MENU)
  {
    if (m_bCanCancel)
    {
      m_bCanceled = true;
      return true;
    }
    else
      return false;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogProgress::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  const CGUIControl *control = GetControl(CONTROL_PROGRESS_BAR);
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_PROGRESS)
  {
    // make sure we have the appropriate info set
    CGUIProgressControl *progress = (CGUIProgressControl *)control;
    if (!progress->GetInfo())
      progress->SetInfo(SYSTEM_PROGRESS_BAR);
  }
}

void CGUIDialogProgress::SetPercentage(int iPercentage)
{
  if (iPercentage < 0) iPercentage = 0;
  if (iPercentage > 100) iPercentage = 100;

  m_percentage = iPercentage;
}

void CGUIDialogProgress::SetProgressMax(int iMax)
{
  m_iMax=iMax;
  m_iCurrent=0;
}

void CGUIDialogProgress::SetProgressAdvance(int nSteps/*=1*/)
{
  m_iCurrent+=nSteps;

  if (m_iCurrent>m_iMax)
    m_iCurrent=0;

  SetPercentage((m_iCurrent*100)/m_iMax);
}

bool CGUIDialogProgress::Abort()
{
  return m_bRunning ? m_bCanceled : false;
}

void CGUIDialogProgress::ShowProgressBar(bool bOnOff)
{
  CGUIMessage msg(bOnOff ? GUI_MSG_VISIBLE : GUI_MSG_HIDDEN, GetID(), CONTROL_PROGRESS_BAR);
  if(OwningCriticalSection(g_graphicsContext))
    OnMessage(msg);
  else
    g_windowManager.SendThreadMessage(msg, GetID());
}

void CGUIDialogProgress::SetHeading(const string& strLine)
{
  m_strHeading = strLine;
  CGUIDialogBoxBase::SetHeading(m_strHeading);
}

void CGUIDialogProgress::SetHeading(int iString)
{
  m_strHeading = g_localizeStrings.Get(iString);
  CGUIDialogBoxBase::SetHeading(m_strHeading);
}

