/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRGuideInfo.h"

#include <utility>

#include "Application.h"
#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVRSearch.h"

using namespace PVR;
using namespace KODI::MESSAGING;

#define CONTROL_BTN_FIND                4
#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7
#define CONTROL_BTN_PLAY_RECORDING      8
#define CONTROL_BTN_ADD_TIMER           9
#define CONTROL_BTN_PLAY_EPGTAG        10

CGUIDialogPVRGuideInfo::CGUIDialogPVRGuideInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_INFO, "DialogPVRInfo.xml")
{
}

CGUIDialogPVRGuideInfo::~CGUIDialogPVRGuideInfo(void) = default;

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

    const std::shared_ptr<CPVRTimerInfoTag> timerTag = CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(m_progItem);
    if (timerTag)
    {
      const CFileItemPtr item(new CFileItem(timerTag));
      if (timerTag->IsRecording())
        bReturn = CServiceBroker::GetPVRManager().GUIActions()->StopRecording(item);
      else
        bReturn = CServiceBroker::GetPVRManager().GUIActions()->DeleteTimer(item);
    }
    else
    {
      const CFileItemPtr item(new CFileItem(m_progItem));
      bReturn = CServiceBroker::GetPVRManager().GUIActions()->AddTimer(item, false);
    }
  }

  if (bReturn)
    Close();

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonAddTimer(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_ADD_TIMER)
  {
    if (m_progItem && !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(m_progItem))
    {
      const CFileItemPtr item(new CFileItem(m_progItem));
      bReturn = CServiceBroker::GetPVRManager().GUIActions()->AddTimerRule(item, true);
    }
  }

  if (bReturn)
    Close();

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonPlay(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_SWITCH ||
      message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING ||
      message.GetSenderId() == CONTROL_BTN_PLAY_EPGTAG)
  {
    Close();

    const CFileItemPtr item(new CFileItem(m_progItem));
    if (message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING)
      CServiceBroker::GetPVRManager().GUIActions()->PlayRecording(item, true /* bCheckResume */);
    else if (message.GetSenderId() == CONTROL_BTN_PLAY_EPGTAG && m_progItem->IsPlayable())
      CServiceBroker::GetPVRManager().GUIActions()->PlayEpgTag(item);
    else
      CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(item, true /* bCheckResume */);

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonFind(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_FIND)
  {
    Close();
    return CServiceBroker::GetPVRManager().GUIActions()->FindSimilar(std::make_shared<CFileItem>(m_progItem));
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
           OnClickButtonPlay(message) ||
           OnClickButtonFind(message) ||
           OnClickButtonAddTimer(message);
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogPVRGuideInfo::OnInfo(int actionID)
{
  Close();
  return true;
}

void CGUIDialogPVRGuideInfo::SetProgInfo(const CPVREpgInfoTagPtr &tag)
{
  m_progItem = tag;
}

CFileItemPtr CGUIDialogPVRGuideInfo::GetCurrentListItem(int offset)
{
  return CFileItemPtr(new CFileItem(m_progItem));
}

void CGUIDialogPVRGuideInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (!m_progItem)
  {
    /* no epg event selected */
    return;
  }

  if (!CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(m_progItem))
  {
    /* not recording. hide the play recording button */
    SET_CONTROL_HIDDEN(CONTROL_BTN_PLAY_RECORDING);
  }

  bool bHideRecord(true);
  bool bHideAddTimer(true);

  const std::shared_ptr<CPVRTimerInfoTag> timer = CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(m_progItem);
  if (timer)
  {
    if (timer->IsRecording())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19059); /* Stop recording */
      bHideRecord = false;
    }
    else if (timer->HasTimerType() && !timer->GetTimerType()->IsReadOnly())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19060); /* Delete timer */
      bHideRecord = false;
    }
  }
  else if (m_progItem->IsRecordable())
  {
    const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_progItem->ClientID());
    if (client && client->GetClientCapabilities().SupportsTimers())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264);     /* Record */
      bHideRecord = false;
      bHideAddTimer = false;
    }
  }

  if (!m_progItem->IsPlayable())
    SET_CONTROL_HIDDEN(CONTROL_BTN_PLAY_EPGTAG);

  if (bHideRecord)
    SET_CONTROL_HIDDEN(CONTROL_BTN_RECORD);

  if (bHideAddTimer)
    SET_CONTROL_HIDDEN(CONTROL_BTN_ADD_TIMER);
}

void CGUIDialogPVRGuideInfo::ShowFor(const CFileItemPtr& item)
{
  CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(item);
}
