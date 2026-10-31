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
#include "utils/DefaultArt.h"
#include "utils/TimeUtils.h"
#include "windowing/WinSystem.h"

#include <mutex>

#define POPUP_ICON                400
#define POPUP_CAPTION_TEXT        401
#define POPUP_NOTIFICATION_BUTTON 402

namespace
{
/*!
 * \brief Check whether a notification supersedes another one, meaning both are successive
 *        updates of the same information and the older one is obsolete. Callers state
 *        this by tagging such notifications with a common group. Example: the stream names
 *        emitted while cycling through the streams of the playing video.
 * \param newer The notification to be displayed next
 * \param current The notification currently on display
 * \return True if newer makes current obsolete, false otherwise
 */
bool Supersedes(const CGUIDialogKaiToast::Notification& newer,
                const CGUIDialogKaiToast::Notification& current)
{
  return !newer.supersedeGroup.empty() && newer.supersedeGroup == current.supersedeGroup;
}
} // unnamed namespace

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
      m_currentNotification = {};
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogKaiToast::QueueNotification(eMessageType eType,
                                           const std::string& aCaption,
                                           const std::string& aDescription,
                                           unsigned int displayTime /*= TOAST_DISPLAY_TIME*/,
                                           bool withSound /*= true*/,
                                           unsigned int messageTime /*= TOAST_MESSAGE_TIME*/,
                                           std::string_view supersedeGroup /*= {}*/)
{
  AddToQueue("", eType, aCaption, aDescription, displayTime, withSound, messageTime,
             supersedeGroup);
}

void CGUIDialogKaiToast::QueueNotification(const std::string& aCaption, const std::string& aDescription)
{
  QueueNotification("", aCaption, aDescription);
}

void CGUIDialogKaiToast::QueueNotification(const std::string& aImageFile, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/, bool withSound /*= true*/, unsigned int messageTime /*= TOAST_MESSAGE_TIME*/)
{
  AddToQueue(aImageFile, Default, aCaption, aDescription, displayTime, withSound, messageTime);
}

void CGUIDialogKaiToast::AddToQueue(const std::string& aImageFile,
                                    const eMessageType eType,
                                    const std::string& aCaption,
                                    const std::string& aDescription,
                                    unsigned int displayTime /*= TOAST_DISPLAY_TIME*/,
                                    bool withSound /*= true*/,
                                    unsigned int messageTime /*= TOAST_MESSAGE_TIME*/,
                                    std::string_view supersedeGroup /*= {}*/)
{
  std::unique_lock lock(m_critical);

  if (!m_notifications.empty())
  {
    const auto& last = m_notifications.back();
    if (last.eType == eType && last.imagefile == aImageFile && last.caption == aCaption &&
        last.description == aDescription && last.supersedeGroup == supersedeGroup)
      return; // avoid duplicates in a row.
  }

  Notification toast;
  toast.eType = eType;
  toast.imagefile = aImageFile;
  toast.caption = aCaption;
  toast.description = aDescription;
  toast.supersedeGroup = supersedeGroup;
  toast.displayTime = displayTime > TOAST_MESSAGE_TIME + 500 ? displayTime : TOAST_MESSAGE_TIME + 500;
  toast.messageTime = messageTime;
  toast.withSound = withSound;

  // A newer notification makes the queued ones of the same group obsolete
  std::erase_if(m_notifications,
                [&toast](const Notification& queued) { return Supersedes(toast, queued); });
  m_notifications.push_back(toast);
}

bool CGUIDialogKaiToast::DoWork()
{
  std::unique_lock lock(m_critical);

  if (!m_notifications.empty() &&
      CTimeUtils::GetFrameTime() - m_timer > m_toastMessageTime)
  {
    // if we have a fade label control for the text to display, ensure the whole text was shown
    // (scrolled to the end) before we move on to the next message. Exception: the next message
    // supersedes the one currently on display, whose text is outdated and must not delay it.
    if (!Supersedes(m_notifications.front(), m_currentNotification))
    {
      const CGUIFadeLabelControl* notificationText =
          dynamic_cast<const CGUIFadeLabelControl*>(GetControl(POPUP_NOTIFICATION_BUTTON));
      if (notificationText && !notificationText->AllLabelsShown())
        return false;
    }

    Notification toast = m_notifications.front();
    m_notifications.pop_front();
    lock.unlock();

    m_toastDisplayTime = toast.displayTime;
    m_toastMessageTime = toast.messageTime;

    std::unique_lock lock2(CServiceBroker::GetWinSystem()->GetGfxContext());

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
          icon = KODI::ART::DEFAULT::ICON_WARNING;
        else if (toast.eType == Error)
          icon = KODI::ART::DEFAULT::ICON_ERROR;
        else
          icon = KODI::ART::DEFAULT::ICON_INFO;
      }
      SET_CONTROL_FILENAME(POPUP_ICON, icon);
    }

    //  Play the window specific init sound for each notification queued
    SetSound(toast.withSound);

    // Activate haptics for this notification
    CServiceBroker::GetPeripherals().OnUserNotification();

    m_currentNotification = toast;

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
      std::unique_lock lock(m_critical);
      bClose = notificationText->AllLabelsShown() && m_notifications.empty();
    }

    if (bClose)
      Close();
  }

  CGUIDialog::FrameMove();
}
