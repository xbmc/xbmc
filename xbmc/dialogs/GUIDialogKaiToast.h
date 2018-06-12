/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "guilib/GUIDialog.h"

#include <queue>

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
    eMessageType eType;
    unsigned int displayTime;
    unsigned int messageTime;
    bool withSound;
  };

  typedef std::queue<Notification> TOASTQUEUE;

  static void QueueNotification(eMessageType eType, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME, bool withSound = true, unsigned int messageTime = TOAST_MESSAGE_TIME);
  static void QueueNotification(const std::string& aCaption, const std::string& aDescription);
  static void QueueNotification(const std::string& aImageFile, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME, bool withSound = true, unsigned int messageTime = TOAST_MESSAGE_TIME);
  bool DoWork();

  bool OnMessage(CGUIMessage& message) override;
  void FrameMove() override;
  void ResetTimer();

protected:
  static void AddToQueue(const std::string& aImageFile, const eMessageType eType, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime, bool withSound, unsigned int messageTime);

  unsigned int m_timer;

  unsigned int m_toastDisplayTime;
  unsigned int m_toastMessageTime;

  static TOASTQUEUE m_notifications;
  static CCriticalSection m_critical;
};
