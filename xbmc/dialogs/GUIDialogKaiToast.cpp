/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogKaiToast.h"

#include "ServiceBroker.h"
#include "guilib/GUIFadeLabelControl.h"
#include "guilib/GUIMessage.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"

#define POPUP_ICON                400
#define POPUP_CAPTION_TEXT        401
#define POPUP_NOTIFICATION_BUTTON 402

CGUIDialogKaiToast::TOASTQUEUE CGUIDialogKaiToast::m_notifications;
CCriticalSection CGUIDialogKaiToast::m_critical;

CGUIDialogKaiToast::CGUIDialogKaiToast(void)
  : CGUIDialog(WINDOW_DIALOG_KAI_TOAST, "DialogNotification.xml", DialogModalityType::MODELESS)
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_timer = 0;
  m_toastDisplayTime = 0;
  m_toastMessageTime = 0;
}

CGUIDialogKaiToast::~CGUIDialogKaiToast(void) = default;

bool CGUIDialogKaiToast::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      ResetTimer();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogKaiToast::QueueNotification(eMessageType eType, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/, bool withSound /*= true*/, unsigned int messageTime /*= TOAST_MESSAGE_TIME*/)
{
  AddToQueue("", eType, aCaption, aDescription, displayTime, withSound, messageTime);
}

void CGUIDialogKaiToast::QueueNotification(const std::string& aCaption, const std::string& aDescription)
{
  QueueNotification("", aCaption, aDescription);
}

void CGUIDialogKaiToast::QueueNotification(const std::string& aImageFile, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/, bool withSound /*= true*/, unsigned int messageTime /*= TOAST_MESSAGE_TIME*/)
{
  AddToQueue(aImageFile, Default, aCaption, aDescription, displayTime, withSound, messageTime);
}

void CGUIDialogKaiToast::AddToQueue(const std::string& aImageFile, const eMessageType eType, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/, bool withSound /*= true*/, unsigned int messageTime /*= TOAST_MESSAGE_TIME*/)
{
  CSingleLock lock(m_critical);

  Notification toast;
  toast.eType = eType;
  toast.imagefile = aImageFile;
  toast.caption = aCaption;
  toast.description = aDescription;
  toast.displayTime = displayTime > TOAST_MESSAGE_TIME + 500 ? displayTime : TOAST_MESSAGE_TIME + 500;
  toast.messageTime = messageTime;
  toast.withSound = withSound;

  m_notifications.push(toast);
}

bool CGUIDialogKaiToast::DoWork()
{
  CSingleLock lock(m_critical);

  if (!m_notifications.empty() &&
      CTimeUtils::GetFrameTime() - m_timer > m_toastMessageTime)
  {
    // if we have a fade label control for the text to display, ensure the whole text was shown
    // (scrolled to the end) before we move on to the next message
    const CGUIFadeLabelControl* notificationText =
        dynamic_cast<const CGUIFadeLabelControl*>(GetControl(POPUP_NOTIFICATION_BUTTON));
    if (notificationText && !notificationText->AllLabelsShown())
      return false;

    Notification toast = m_notifications.front();
    m_notifications.pop();
    lock.Leave();

    m_toastDisplayTime = toast.displayTime;
    m_toastMessageTime = toast.messageTime;

    CSingleLock lock2(CServiceBroker::GetWinSystem()->GetGfxContext());

    if(!Initialize())
      return false;

    SET_CONTROL_LABEL(POPUP_CAPTION_TEXT, toast.caption);

    SET_CONTROL_LABEL(POPUP_NOTIFICATION_BUTTON, toast.description);

    // set the appropriate icon
    {
      std::string icon = toast.imagefile;
      if (icon.empty())
      {
        if (toast.eType == Warning)
          icon = "DefaultIconWarning.png";
        else if (toast.eType == Error)
          icon = "DefaultIconError.png";
        else
          icon = "DefaultIconInfo.png";
      }
      SET_CONTROL_FILENAME(POPUP_ICON, icon);
    }

    //  Play the window specific init sound for each notification queued
    SetSound(toast.withSound);

    // Activate haptics for this notification
    CServiceBroker::GetPeripherals().OnUserNotification();

    ResetTimer();
    return true;
  }

  return false;
}


void CGUIDialogKaiToast::ResetTimer()
{
  m_timer = CTimeUtils::GetFrameTime();
}

void CGUIDialogKaiToast::FrameMove()
{
  //  Fading does not count as display time
  if (IsAnimating(ANIM_TYPE_WINDOW_OPEN))
    ResetTimer();

  // now check if we should exit
  if (CTimeUtils::GetFrameTime() - m_timer > m_toastDisplayTime)
  {
    bool bClose = true;

    // if we have a fade label control for the text to display, ensure the whole text was shown
    // (scrolled to the end) before we're closing the toast dialog
    const CGUIFadeLabelControl* notificationText =
        dynamic_cast<const CGUIFadeLabelControl*>(GetControl(POPUP_NOTIFICATION_BUTTON));
    if (notificationText)
    {
      CSingleLock lock(m_critical);
      bClose = notificationText->AllLabelsShown() && m_notifications.empty();
    }

    if (bClose)
      Close();
  }

  CGUIDialog::FrameMove();
}
