/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "GUIDialogPVRGuideInfo.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "epg/EpgInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"

using namespace std;
using namespace PVR;
using namespace EPG;

#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7

CGUIDialogPVRGuideInfo::CGUIDialogPVRGuideInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_INFO, "DialogPVRGuideInfo.xml")
    , m_progItem(new CFileItem)
{
}

CGUIDialogPVRGuideInfo::~CGUIDialogPVRGuideInfo(void)
{
}

bool CGUIDialogPVRGuideInfo::ActionStartTimer(const CEpgInfoTag *tag)
{
  bool bReturn = false;

  // prompt user for confirmation of channel record
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

  if (pDialog)
  {
    pDialog->SetHeading(264);
    pDialog->SetLine(0, "");
    pDialog->SetLine(1, tag->Title());
    pDialog->SetLine(2, "");
    pDialog->DoModal();

    if (pDialog->IsConfirmed())
    {
      Close();
      CPVRTimerInfoTag *newTimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
      bReturn = CPVRTimers::AddTimer(*newTimer);
      delete newTimer;
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::ActionCancelTimer(const CPVRTimerInfoTag *tag)
{
  bool bReturn = false;

  // prompt user for confirmation of timer deletion
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

  if (pDialog)
  {
    pDialog->SetHeading(265);
    pDialog->SetLine(0, "");
    pDialog->SetLine(1, tag->m_strTitle);
    pDialog->SetLine(2, "");
    pDialog->DoModal();

    if (pDialog->IsConfirmed())
    {
      Close();
      bReturn = CPVRTimers::DeleteTimer(*tag);
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonOK(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_OK)
  {
    Close();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonRecord(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_RECORD)
  {
    bReturn = true;

    const CEpgInfoTag *tag = m_progItem->GetEPGInfoTag();
    if (!tag || !tag->HasPVRChannel())
    {
      /* invalid channel */
      CGUIDialogOK::ShowAndGetInput(19033,19067,0,0);
      Close();
      return bReturn;
    }

    const CPVRTimerInfoTag *timerTag = g_PVRTimers->GetMatch(tag);
    bool bHasTimer = timerTag != NULL;

    if (!bHasTimer)
      ActionStartTimer(tag);
    else
      ActionCancelTimer(timerTag);
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonSwitch(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_SWITCH)
  {
    Close();

    if (!m_progItem->GetEPGInfoTag()->HasPVRChannel() ||
        !g_application.PlayFile(CFileItem(*m_progItem->GetEPGInfoTag()->ChannelTag())))
      CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);
    else
      bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    CGUIDialog::OnMessage(message);
    Update();
    break;
  case GUI_MSG_CLICKED:
    return OnClickButtonOK(message) ||
           OnClickButtonRecord(message) ||
           OnClickButtonSwitch(message);
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRGuideInfo::SetProgInfo(const CFileItem *item)
{
  *m_progItem = *item;
}

CFileItemPtr CGUIDialogPVRGuideInfo::GetCurrentListItem(int offset)
{
  return m_progItem;
}

void CGUIDialogPVRGuideInfo::Update()
{
  const CEpgInfoTag *tag = m_progItem->GetEPGInfoTag();
  if (!tag)
  {
    /* no epg event selected */
    return;
  }

  if (tag->EndAsLocalTime() <= CDateTime::GetCurrentDateTime())
  {
    /* event has passed. hide the record button */
    SET_CONTROL_HIDDEN(CONTROL_BTN_RECORD);
    return;
  }

  bool bHasTimer = g_PVRTimers->GetMatch(tag) != NULL;
  if (!bHasTimer)
  {
    /* no timer present on this tag */
    if (tag->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264);
    else
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19061);
  }
  else
  {
    /* timer present on this tag */
    if (tag->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19059);
    else
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19060);
  }
}
