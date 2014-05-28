/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#ifndef DIALOGS_GUIDIALOGPVRGUIDEINFO_H_INCLUDED
#define DIALOGS_GUIDIALOGPVRGUIDEINFO_H_INCLUDED
#include "GUIDialogPVRGuideInfo.h"
#endif

#ifndef DIALOGS_APPLICATION_H_INCLUDED
#define DIALOGS_APPLICATION_H_INCLUDED
#include "Application.h"
#endif

#ifndef DIALOGS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#define DIALOGS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#include "guilib/GUIWindowManager.h"
#endif

#ifndef DIALOGS_DIALOGS_GUIDIALOGOK_H_INCLUDED
#define DIALOGS_DIALOGS_GUIDIALOGOK_H_INCLUDED
#include "dialogs/GUIDialogOK.h"
#endif

#ifndef DIALOGS_DIALOGS_GUIDIALOGYESNO_H_INCLUDED
#define DIALOGS_DIALOGS_GUIDIALOGYESNO_H_INCLUDED
#include "dialogs/GUIDialogYesNo.h"
#endif

#ifndef DIALOGS_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define DIALOGS_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif


#ifndef DIALOGS_PVR_PVRMANAGER_H_INCLUDED
#define DIALOGS_PVR_PVRMANAGER_H_INCLUDED
#include "pvr/PVRManager.h"
#endif

#ifndef DIALOGS_PVR_CHANNELS_PVRCHANNELGROUPSCONTAINER_H_INCLUDED
#define DIALOGS_PVR_CHANNELS_PVRCHANNELGROUPSCONTAINER_H_INCLUDED
#include "pvr/channels/PVRChannelGroupsContainer.h"
#endif

#ifndef DIALOGS_EPG_EPGINFOTAG_H_INCLUDED
#define DIALOGS_EPG_EPGINFOTAG_H_INCLUDED
#include "epg/EpgInfoTag.h"
#endif

#ifndef DIALOGS_PVR_TIMERS_PVRTIMERS_H_INCLUDED
#define DIALOGS_PVR_TIMERS_PVRTIMERS_H_INCLUDED
#include "pvr/timers/PVRTimers.h"
#endif

#ifndef DIALOGS_PVR_TIMERS_PVRTIMERINFOTAG_H_INCLUDED
#define DIALOGS_PVR_TIMERS_PVRTIMERINFOTAG_H_INCLUDED
#include "pvr/timers/PVRTimerInfoTag.h"
#endif


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

  if (!tag)
    return false;

  CPVRChannelPtr channel = tag->ChannelTag();
  if (!channel || !g_PVRManager.CheckParentalLock(*channel))
    return false;

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
      if (newTimer)
      {
        bReturn = CPVRTimers::AddTimer(*newTimer);
        delete newTimer;
      }
      else
      {
        bReturn = false;
      }
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::ActionCancelTimer(CFileItemPtr timer)
{
  bool bReturn = false;
  if (!timer || !timer->HasPVRTimerInfoTag())
  {
    return bReturn;
  }

  // prompt user for confirmation of timer deletion
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

  if (pDialog)
  {
    pDialog->SetHeading(265);
    pDialog->SetLine(0, "");
    pDialog->SetLine(1, timer->GetPVRTimerInfoTag()->m_strTitle);
    pDialog->SetLine(2, "");
    pDialog->DoModal();

    if (pDialog->IsConfirmed())
    {
      Close();
      bReturn = CPVRTimers::DeleteTimer(*timer);
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

    CFileItemPtr timerTag = g_PVRTimers->GetTimerForEpgTag(m_progItem.get());
    bool bHasTimer = timerTag != NULL && timerTag->HasPVRTimerInfoTag();

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
    PlayBackRet ret = PLAYBACK_CANCELED;
    if (!m_progItem->GetEPGInfoTag()->HasPVRChannel() ||
        (ret = g_application.PlayFile(CFileItem(*m_progItem->GetEPGInfoTag()->ChannelTag()))) == PLAYBACK_FAIL)
    {
      CStdString msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), g_localizeStrings.Get(19029).c_str()); // Channel could not be played. Check the log for details.
      CGUIDialogOK::ShowAndGetInput(19033, 0, msg, 0);
    }
    else if (ret == PLAYBACK_OK)
    {
      bReturn = true;
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
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

void CGUIDialogPVRGuideInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

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

  CFileItemPtr match = g_PVRTimers->GetTimerForEpgTag(m_progItem.get());
  if (!match || !match->HasPVRTimerInfoTag())
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
