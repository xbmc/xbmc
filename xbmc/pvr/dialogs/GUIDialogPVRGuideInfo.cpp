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

#include "Application.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "epg/EpgInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/windows/GUIWindowPVRBase.h"

#include "GUIDialogPVRGuideInfo.h"

#include <utility>

using namespace PVR;
using namespace EPG;

#define CONTROL_BTN_FIND                4
#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7
#define CONTROL_BTN_PLAY_RECORDING      8
#define CONTROL_BTN_ENABLE              9

CGUIDialogPVRGuideInfo::CGUIDialogPVRGuideInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_INFO, "DialogPVRInfo.xml")
    , m_progItem(new CFileItem)
{
}

CGUIDialogPVRGuideInfo::~CGUIDialogPVRGuideInfo(void)
{
}

bool CGUIDialogPVRGuideInfo::ActionStartTimer(const CEpgInfoTagPtr &tag)
{
  bool bReturn = false;

  CFileItemPtr item(new CFileItem(tag));
  bReturn = CGUIWindowPVRBase::AddTimer(item.get(), false);

  if (bReturn)
    Close();

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::ActionCancelTimer(const CFileItemPtr &timer)
{
  bool bReturn = false;

  if (timer->GetPVRTimerInfoTag()->IsRecording())
    bReturn = CGUIWindowPVRBase::StopRecordFile(timer.get());
  else
    bReturn = CGUIWindowPVRBase::DeleteTimer(timer.get());

  if (bReturn)
    Close();

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

    const CEpgInfoTagPtr tag(m_progItem->GetEPGInfoTag());
    if (!tag || !tag->HasPVRChannel())
    {
      /* invalid channel */
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19067});
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

bool CGUIDialogPVRGuideInfo::OnClickButtonEnable(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_ENABLE)
  {
    bReturn = true;

    const CEpgInfoTagPtr tag(m_progItem->GetEPGInfoTag());
    if (!tag || !tag->HasPVRChannel())
    {
      /* invalid channel */
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19067});
      Close();
      return bReturn;
    }

    CFileItemPtr timerTag = g_PVRTimers->GetTimerForEpgTag(m_progItem.get());
    bool bHasTimer = timerTag != NULL && timerTag->HasPVRTimerInfoTag();

    if (!bHasTimer)
      return false;

    /* Toggle enabled state */
    bReturn = CGUIWindowPVRBase::EnableDisableTimer(timerTag.get());
    Close();
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonPlay(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_SWITCH || message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING)
  {
    Close();
    PlayBackRet ret = PLAYBACK_CANCELED;
    CEpgInfoTagPtr epgTag(m_progItem->GetEPGInfoTag());

    if (epgTag)
    {
      if (message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING && epgTag->HasRecording())
        ret = g_application.PlayFile(CFileItem(epgTag->Recording()), "videoplayer");
      else if (epgTag->HasPVRChannel())
        ret = g_application.PlayFile(CFileItem(epgTag->ChannelTag()), "videoplayer");
    }
    else
      ret = PLAYBACK_FAIL;

    if (ret == PLAYBACK_FAIL)
    {
      std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), g_localizeStrings.Get(19029).c_str()); // Channel could not be played. Check the log for details.
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{std::move(msg)});
    }
    else if (ret == PLAYBACK_OK)
    {
      bReturn = true;
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonFind(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_FIND)
  {
    const CEpgInfoTagPtr tag(m_progItem->GetEPGInfoTag());
    if (tag && tag->HasPVRChannel())
    {
      int windowSearchId = tag->ChannelTag()->IsRadio() ? WINDOW_RADIO_SEARCH : WINDOW_TV_SEARCH;
      CGUIWindowPVRBase *windowSearch = (CGUIWindowPVRBase*) g_windowManager.GetWindow(windowSearchId);
      if (windowSearch)
      {
        Close();
        g_windowManager.ActivateWindow(windowSearchId);
        bReturn = windowSearch->OnContextButton(*m_progItem.get(), CONTEXT_BUTTON_FIND);
      }
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
           OnClickButtonPlay(message) ||
           OnClickButtonFind(message) ||
           OnClickButtonEnable(message);
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

  const CEpgInfoTagPtr tag(m_progItem->GetEPGInfoTag());
  if (!tag)
  {
    /* no epg event selected */
    return;
  }

  if (!tag->HasRecording())
  {
    /* not recording. hide the play recording button */
    SET_CONTROL_HIDDEN(CONTROL_BTN_PLAY_RECORDING);
  }

  bool bHideRecord(true);
  bool bHideEnable(true);
  if (tag->HasTimer())
  {
    if (tag->Timer()->IsRecording())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19059); /* Stop recording */
      bHideRecord = false;
    }
    else if (tag->Timer()->HasTimerType())
    {
      /* 1) Read only timers cannot be deleted, so disable them if possible */
      /* 2) Activate disabled timers (record -> enable) */
      /* 3) Delete for all the rest */
      if (tag->Timer()->GetTimerType()->IsReadOnly() ||
          tag->Timer()->m_state == PVR_TIMER_STATE_DISABLED)
      {
        if (tag->Timer()->GetTimerType()->SupportsEnableDisable())
        {
          if (tag->Timer()->m_state == PVR_TIMER_STATE_DISABLED)
            SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, 843);  /* Activate */
          else
            SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, 844);  /* Deactivate */

          bHideEnable = false;
        }
      }
      else
      {
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19060); /* Delete timer */
        bHideRecord = false;
      }
    }
  }
  else if (tag->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
  {
    SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264);     /* Record */
    bHideRecord = false;
  }

  if (bHideRecord)
    SET_CONTROL_HIDDEN(CONTROL_BTN_RECORD);
  if (bHideEnable)
    SET_CONTROL_HIDDEN(CONTROL_BTN_ENABLE);
}
