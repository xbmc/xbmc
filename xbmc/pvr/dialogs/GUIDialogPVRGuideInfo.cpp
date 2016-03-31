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
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/windows/GUIWindowPVRBase.h"

#include "GUIDialogPVRGuideInfo.h"

#include <utility>

using namespace PVR;
using namespace EPG;
using namespace KODI::MESSAGING;

#define CONTROL_BTN_FIND                4
#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7
#define CONTROL_BTN_PLAY_RECORDING      8

CGUIDialogPVRGuideInfo::CGUIDialogPVRGuideInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_INFO, "DialogPVRInfo.xml")
{
}

CGUIDialogPVRGuideInfo::~CGUIDialogPVRGuideInfo(void)
{
}

bool CGUIDialogPVRGuideInfo::ActionStartTimer(const CEpgInfoTagPtr &tag)
{
  bool bReturn = false;

  CFileItemPtr item(new CFileItem(tag));
  bReturn = CGUIWindowPVRBase::AddTimer(item.get());

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

    if (!m_progItem || !m_progItem->HasPVRChannel())
    {
      /* invalid channel */
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19067});
      Close();
      return bReturn;
    }

    CPVRTimerInfoTagPtr timerTag = m_progItem->Timer();
    if (timerTag)
      ActionCancelTimer(CFileItemPtr(new CFileItem(timerTag)));
    else
      ActionStartTimer(m_progItem);
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonPlay(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_SWITCH || message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING)
  {
    Close();

    if (m_progItem)
    {
      if (message.GetSenderId() == CONTROL_BTN_PLAY_RECORDING && m_progItem->HasRecording())
        g_application.PlayFile(CFileItem(m_progItem->Recording()), "videoplayer");
      else if (m_progItem->HasPVRChannel())
      {
        CPVRChannelPtr channel = m_progItem->ChannelTag();
        // try a fast switch
        bool bSwitchSuccessful = false;
        if ((g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio()) &&
            (channel->IsRadio() == g_PVRManager.IsPlayingRadio()))
        {
          if (channel->StreamURL().empty())
            bSwitchSuccessful = g_application.m_pPlayer->SwitchChannel(channel);
        }

        if (!bSwitchSuccessful)
        {
          CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(channel)), "videoplayer");
        }
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGuideInfo::OnClickButtonFind(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == CONTROL_BTN_FIND)
  {
    if (m_progItem && m_progItem->HasPVRChannel())
    {
      int windowSearchId = m_progItem->ChannelTag()->IsRadio() ? WINDOW_RADIO_SEARCH : WINDOW_TV_SEARCH;
      CGUIWindowPVRBase *windowSearch = (CGUIWindowPVRBase*) g_windowManager.GetWindow(windowSearchId);
      if (windowSearch)
      {
        Close();
        g_windowManager.ActivateWindow(windowSearchId);
        bReturn = windowSearch->OnContextButton(CFileItem(m_progItem), CONTEXT_BUTTON_FIND);
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
           OnClickButtonFind(message);
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogPVRGuideInfo::OnInfo(int actionID)
{
  Close();
  return true;
}

void CGUIDialogPVRGuideInfo::SetProgInfo(const EPG::CEpgInfoTagPtr &tag)
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

  if (!m_progItem->HasRecording())
  {
    /* not recording. hide the play recording button */
    SET_CONTROL_HIDDEN(CONTROL_BTN_PLAY_RECORDING);
  }

  bool bHideRecord(true);
  if (m_progItem->HasTimer())
  {
    if (m_progItem->Timer()->IsRecording())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19059); /* Stop recording */
      bHideRecord = false;
    }
    else if (m_progItem->Timer()->HasTimerType() && !m_progItem->Timer()->GetTimerType()->IsReadOnly())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19060); /* Delete timer */
      bHideRecord = false;
    }
  }
  else if (m_progItem->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
  {
    SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264);     /* Record */
    bHideRecord = false;
  }

  if (bHideRecord)
    SET_CONTROL_HIDDEN(CONTROL_BTN_RECORD);
}
