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
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/guilib/PVRGUIRecordingsPlayActionProcessor.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"

#include <memory>

using namespace PVR;

#define CONTROL_BTN_FIND 4
#define CONTROL_BTN_SWITCH 5
#define CONTROL_BTN_RECORD 6
#define CONTROL_BTN_OK 7
#define CONTROL_BTN_PLAY_RECORDING 8
#define CONTROL_BTN_ADD_TIMER 9
#define CONTROL_BTN_PLAY_EPGTAG 10
#define CONTROL_BTN_SET_REMINDER 11

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
    auto& mgr = CServiceBroker::GetPVRManager();

    const std::shared_ptr<CPVRTimerInfoTag> timerTag =
        mgr.Timers()->GetTimerForEpgTag(m_progItem->GetEPGInfoTag());
    if (timerTag)
    {
      if (timerTag->IsRecording())
        bReturn = mgr.Get<PVR::GUI::Timers>().StopRecording(CFileItem(timerTag));
      else
        bReturn = mgr.Get<PVR::GUI::Timers>().DeleteTimer(CFileItem(timerTag));
    }
    else
    {
      bReturn = mgr.Get<PVR::GUI::Timers>().AddTimer(*m_progItem, false);
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
    auto& mgr = CServiceBroker::GetPVRManager();
    if (m_progItem && !mgr.Timers()->GetTimerForEpgTag(m_progItem->GetEPGInfoTag()))
    {
      bReturn = mgr.Get<PVR::GUI::Timers>().AddTimerRule(*m_progItem, true, true);
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
    auto& mgr = CServiceBroker::GetPVRManager();
    if (m_progItem && !mgr.Timers()->GetTimerForEpgTag(m_progItem->GetEPGInfoTag()))
    {
      bReturn = mgr.Get<PVR::GUI::Timers>().AddReminder(*m_progItem);
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

    if (m_progItem)
    {
      if (message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING)
      {
        const auto recording{CPVRItem(m_progItem).GetRecording()};
        if (recording)
        {
          CGUIPVRRecordingsPlayActionProcessor proc{std::make_shared<CFileItem>(recording)};
          proc.ProcessDefaultAction();
          if (proc.UserCancelled())
            Open();
        }
      }
      else if (message.GetSenderId() == CONTROL_BTN_PLAY_EPGTAG &&
               m_progItem->GetEPGInfoTag()->IsPlayable())
      {
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayEpgTag(*m_progItem);
      }
      else
      {
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().SwitchToChannel(
            *m_progItem, true /* bCheckResume */);
      }
      bReturn = true;
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonFind(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_FIND)
  {
    Close();
    if (m_progItem)
      return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().FindSimilar(*m_progItem);
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
      return OnClickButtonOK(message) || OnClickButtonRecord(message) ||
             OnClickButtonPlay(message) || OnClickButtonFind(message) ||
             OnClickButtonAddTimer(message) || OnClickButtonSetReminder(message);
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogPVRGuideInfo::OnInfo(int actionID)
{
  Close();
  return true;
}

void CGUIDialogPVRGuideInfo::SetProgInfo(const std::shared_ptr<CFileItem>& item)
{
  m_progItem = item;
}

CFileItemPtr CGUIDialogPVRGuideInfo::GetCurrentListItem(int offset)
{
  return m_progItem;
}

void CGUIDialogPVRGuideInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (!m_progItem)
  {
    /* no epg event selected */
    return;
  }

  auto& mgr = CServiceBroker::GetPVRManager();
  const auto epgTag = m_progItem->GetEPGInfoTag();

  if (!mgr.Recordings()->GetRecordingForEpgTag(epgTag))
  {
    /* not recording. hide the play recording button */
    SET_CONTROL_HIDDEN(CONTROL_BTN_PLAY_RECORDING);
  }

  bool bHideRecord = true;
  bool bHideAddTimer = true;
  const std::shared_ptr<const CPVRTimerInfoTag> timer = mgr.Timers()->GetTimerForEpgTag(epgTag);
  bool bHideSetReminder = timer || (epgTag->StartAsLocalTime() <= CDateTime::GetCurrentDateTime());

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
  else if (epgTag->IsRecordable())
  {
    const std::shared_ptr<const CPVRClient> client = mgr.GetClient(epgTag->ClientID());
    if (client && client->GetClientCapabilities().SupportsTimers())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264); /* Record */
      bHideRecord = false;
      bHideAddTimer = false;
    }
  }

  if (!epgTag->IsPlayable())
    SET_CONTROL_HIDDEN(CONTROL_BTN_PLAY_EPGTAG);

  if (bHideRecord)
    SET_CONTROL_HIDDEN(CONTROL_BTN_RECORD);

  if (bHideAddTimer)
    SET_CONTROL_HIDDEN(CONTROL_BTN_ADD_TIMER);

  if (bHideSetReminder)
    SET_CONTROL_HIDDEN(CONTROL_BTN_SET_REMINDER);
}
