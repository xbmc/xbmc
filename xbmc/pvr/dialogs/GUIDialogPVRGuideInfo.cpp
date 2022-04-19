/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRGuideInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActions.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"

#include <memory>

using namespace PVR;

#define CONTROL_BTN_FIND                4
#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7
#define CONTROL_BTN_PLAY_RECORDING      8
#define CONTROL_BTN_ADD_TIMER           9
#define CONTROL_BTN_PLAY_EPGTAG        10
#define CONTROL_BTN_SET_REMINDER       11

CGUIDialogPVRGuideInfo::CGUIDialogPVRGuideInfo()
    : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_INFO, "DialogPVRInfo.xml")
{
}

CGUIDialogPVRGuideInfo::~CGUIDialogPVRGuideInfo() = default;

bool CGUIDialogPVRGuideInfo::OnClickButtonOK(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_OK)
  {
    Close();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonRecord(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_RECORD)
  {
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

bool CGUIDialogPVRGuideInfo::OnClickButtonAddTimer(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_ADD_TIMER)
  {
    if (m_progItem && !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(m_progItem))
    {
      const CFileItemPtr item(new CFileItem(m_progItem));
      bReturn = CServiceBroker::GetPVRManager().GUIActions()->AddTimerRule(item, true, true);
    }
  }

  if (bReturn)
    Close();

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonSetReminder(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_SET_REMINDER)
  {
    if (m_progItem && !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(m_progItem))
    {
      const std::shared_ptr<CFileItem> item = std::make_shared<CFileItem>(m_progItem);
      bReturn = CServiceBroker::GetPVRManager().GUIActions()->AddReminder(item);
    }
  }

  if (bReturn)
    Close();

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonPlay(const CGUIMessage& message)
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

bool CGUIDialogPVRGuideInfo::OnClickButtonFind(const CGUIMessage& message)
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
           OnClickButtonAddTimer(message) ||
           OnClickButtonSetReminder(message);
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogPVRGuideInfo::OnInfo(int actionID)
{
  Close();
  return true;
}

void CGUIDialogPVRGuideInfo::SetProgInfo(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  m_progItem = tag;
}

CFileItemPtr CGUIDialogPVRGuideInfo::GetCurrentListItem(int offset)
{
  if (!m_progItem)
    return {};

  return std::make_shared<CFileItem>(m_progItem);
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

  bool bHideRecord = true;
  bool bHideAddTimer = true;
  const std::shared_ptr<CPVRTimerInfoTag> timer = CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(m_progItem);
  bool bHideSetReminder = timer || (m_progItem->StartAsLocalTime() <= CDateTime::GetCurrentDateTime());

  if (timer)
  {
    if (timer->IsRecording())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19059); /* Stop recording */
      bHideRecord = false;
    }
    else if (!timer->GetTimerType()->IsReadOnly())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19060); /* Delete timer */
      bHideRecord = false;
    }
  }
  else if (m_progItem->IsRecordable())
  {
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_progItem->ClientID());
    if (client && client->GetClientCapabilities().SupportsTimers())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264); /* Record */
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

  if (bHideSetReminder)
    SET_CONTROL_HIDDEN(CONTROL_BTN_SET_REMINDER);
}
