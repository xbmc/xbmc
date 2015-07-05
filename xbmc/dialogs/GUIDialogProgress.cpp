/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
  m_showProgress = false;
  m_bCanCancel = true;
}

CGUIDialogProgress::~CGUIDialogProgress(void)
{

}

void CGUIDialogProgress::SetCanCancel(bool bCanCancel)
{
  CSingleLock lock(m_section);
  m_bCanCancel = bCanCancel;
  SetInvalid();
}

void CGUIDialogProgress::StartModal()
{
  CSingleLock lock(g_graphicsContext);

  CLog::Log(LOGDEBUG, "DialogProgress::StartModal called %s", m_active ? "(already running)!" : "");
  m_bCanceled = false;

  // set running before it's routed, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_active = true;
  m_modalityType = DialogModalityType::MODAL;
  m_closing = false;
  g_windowManager.RegisterDialog(this);

  // active this window...
  ShowProgressBar(false);
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);

  lock.Leave();

  while (m_active && IsAnimating(ANIM_TYPE_WINDOW_OPEN))
  {
    Progress();
    // we should have rendered at least once by now - if we haven't, then
    // we must be running from fullscreen video or similar where the
    // calling thread handles rendering (ie not main app thread) but
    // is waiting on this routine before rendering begins
    if (!HasProcessed())
      break;
  }
}

void CGUIDialogProgress::Progress()
{
  if (m_active)
  {
    g_windowManager.ProcessRenderLoop();
  }
}

void CGUIDialogProgress::ProgressKeys()
{
  if (m_active)
  {
    g_application.FrameMove(true);
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
      if (iControl == CONTROL_CANCEL_BUTTON && m_bCanCancel && !m_bCanceled)
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

bool CGUIDialogProgress::OnBack(int actionID)
{
  if (m_bCanCancel)
  {
    m_bCanceled = true;
    return true;
  }
  return false;
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
  return m_active ? m_bCanceled : false;
}

void CGUIDialogProgress::ShowProgressBar(bool bOnOff)
{
  CSingleLock lock(m_section);
  m_showProgress = bOnOff;
  SetInvalid();
}

void CGUIDialogProgress::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  { // take a copy to save holding the lock for too long
    bool showProgress, showCancel;
    {
      CSingleLock lock(m_section);
      showProgress = m_showProgress;
      showCancel   = m_bCanCancel;
    }
    if (showProgress)
      SET_CONTROL_VISIBLE(CONTROL_PROGRESS_BAR);
    else
      SET_CONTROL_HIDDEN(CONTROL_PROGRESS_BAR);
    if (showCancel)
      SET_CONTROL_VISIBLE(CONTROL_CANCEL_BUTTON);
    else
      SET_CONTROL_HIDDEN(CONTROL_CANCEL_BUTTON);
  }
  CGUIDialogBoxBase::Process(currentTime, dirtyregions);
}

int CGUIDialogProgress::GetDefaultLabelID(int controlId) const
{
  if (controlId == CONTROL_CANCEL_BUTTON)
    return 222;
  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}
