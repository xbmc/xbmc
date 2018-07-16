/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogProgress.h"
#include "guilib/GUIProgressControl.h"
#include "Application.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/Variant.h"

CGUIDialogProgress::CGUIDialogProgress(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_PROGRESS, "DialogConfirm.xml")
{
  Reset();
}

CGUIDialogProgress::~CGUIDialogProgress(void) = default;

void CGUIDialogProgress::Reset()
{
  CSingleLock lock(m_section);
  m_bCanceled = false;
  m_iCurrent = 0;
  m_iMax = 0;
  m_percentage = 0;
  m_showProgress = true;
  m_bCanCancel = true;
  SetInvalid();
}

void CGUIDialogProgress::SetCanCancel(bool bCanCancel)
{
  CSingleLock lock(m_section);
  m_bCanCancel = bCanCancel;
  SetInvalid();
}

void CGUIDialogProgress::Open(const std::string &param /* = "" */)
{
  CLog::Log(LOGDEBUG, "DialogProgress::Open called %s", m_active ? "(already running)!" : "");

  {
    CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
    ShowProgressBar(true);
  }

  CGUIDialog::Open_Internal(false, param);

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
    ProcessRenderLoop();
  }
}

bool CGUIDialogProgress::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {

  case GUI_MSG_WINDOW_DEINIT:
    Reset();
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_NO_BUTTON && m_bCanCancel && !m_bCanceled)
      {
        std::string strHeading = m_strHeading;
        strHeading.append(" : ");
        strHeading.append(g_localizeStrings.Get(16024));
        CGUIDialogBoxBase::SetHeading(CVariant{strHeading});
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
  CGUIControl *control = GetControl(CONTROL_PROGRESS_BAR);
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_PROGRESS)
  {
    // make sure we have the appropriate info set
    CGUIProgressControl *progress = static_cast<CGUIProgressControl*>(control);
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

  if (m_iMax > 0)
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

bool CGUIDialogProgress::Wait(int progresstime /*= 10*/)
{
  CEvent m_done;
  while (!m_done.WaitMSec(progresstime) && m_active && !m_bCanceled)
    Progress();

  return !m_bCanceled;
}

bool CGUIDialogProgress::WaitOnEvent(CEvent& event)
{
  while (!event.WaitMSec(1))
  {
    if (m_bCanceled)
      return false;

    Progress();
  }

  return !m_bCanceled;
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
      SET_CONTROL_VISIBLE(CONTROL_NO_BUTTON);
    else
      SET_CONTROL_HIDDEN(CONTROL_NO_BUTTON);
  }
  CGUIDialogBoxBase::Process(currentTime, dirtyregions);
}

void CGUIDialogProgress::OnInitWindow()
{
  SET_CONTROL_HIDDEN(CONTROL_YES_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_CUSTOM_BUTTON);
  SET_CONTROL_FOCUS(CONTROL_NO_BUTTON, 0);

  CGUIDialogBoxBase::OnInitWindow();
}

int CGUIDialogProgress::GetDefaultLabelID(int controlId) const
{
  if (controlId == CONTROL_NO_BUTTON)
    return 222;
  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}
