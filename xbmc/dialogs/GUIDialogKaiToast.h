/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <deque>
#include <string>
#include <string_view>

#define TOAST_DISPLAY_TIME   5000L  // default 5 seconds
#define TOAST_MESSAGE_TIME   1000L  // minimal message time 1 second

class CGUIDialogKaiToast: public CGUIDialog
{
public:
  CGUIDialogKaiToast(void);
  ~CGUIDialogKaiToast(void) override;

  enum eMessageType { Default = 0, Info, Warning, Error };

  struct Notification
  {
    std::string caption;
    std::string description;
    std::string imagefile;
    //! Notifications sharing a non-empty group are successive updates of the same information.
    std::string supersedeGroup;
    eMessageType eType;
    unsigned int displayTime;
    unsigned int messageTime;
    bool withSound;
  };

  typedef std::deque<Notification> TOASTQUEUE;

  /*!
   * \brief Queue a notification for display.
   * \param supersedeGroup Optional. Notifications carrying the same group replace each other
   *        without waiting for the text on display to finish scrolling. Use it for
   *        notifications that are successive updates of the same information, for example the
   *        stream names emitted while cycling through the streams of the playing video.
   */
  static void QueueNotification(eMessageType eType,
                                const std::string& aCaption,
                                const std::string& aDescription,
                                unsigned int displayTime = TOAST_DISPLAY_TIME,
                                bool withSound = true,
                                unsigned int messageTime = TOAST_MESSAGE_TIME,
                                std::string_view supersedeGroup = {});
  static void QueueNotification(const std::string& aCaption, const std::string& aDescription);
  static void QueueNotification(const std::string& aImageFile, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME, bool withSound = true, unsigned int messageTime = TOAST_MESSAGE_TIME);
  bool DoWork();

  bool OnMessage(CGUIMessage& message) override;
  void FrameMove() override;
  void ResetTimer();

protected:
  static void AddToQueue(const std::string& aImageFile,
                         const eMessageType eType,
                         const std::string& aCaption,
                         const std::string& aDescription,
                         unsigned int displayTime,
                         bool withSound,
                         unsigned int messageTime,
                         std::string_view supersedeGroup = {});

  unsigned int m_timer;

  unsigned int m_toastDisplayTime;
  unsigned int m_toastMessageTime;

  static TOASTQUEUE m_notifications;
  static CCriticalSection m_critical;

private:
  //! The notification currently on display, if any.
  Notification m_currentNotification;
};
