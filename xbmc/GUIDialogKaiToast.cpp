/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogKaiToast.h"
#include "GUISliderControl.h"
#include "Application.h"
#include "GUIAudioManager.h"

#define POPUP_ICON     400
#define POPUP_CAPTION_TEXT   401
#define POPUP_NOTIFICATION_BUTTON 402

CGUIDialogKaiToast::CGUIDialogKaiToast(void)
: CGUIDialog(WINDOW_DIALOG_KAI_TOAST, "DialogKaiToast.xml")
{
  m_defaultIcon = "";
  m_loadOnDemand = false;
  InitializeCriticalSection(&m_critical);
}

CGUIDialogKaiToast::~CGUIDialogKaiToast(void)
{
  DeleteCriticalSection(&m_critical);
}

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

void CGUIDialogKaiToast::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  CGUIImage *image = (CGUIImage *)GetControl(POPUP_ICON);
  if (image)
    m_defaultIcon = image->GetFileName();
}

void CGUIDialogKaiToast::QueueNotification(const CStdString& aCaption, const CStdString& aDescription)
{
  QueueNotification("", aCaption, aDescription);
}

void CGUIDialogKaiToast::QueueNotification(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/)
{
  EnterCriticalSection(&m_critical);

  if (!Initialize())
    return;

  Notification toast;
  toast.imagefile = aImageFile;
  toast.caption = aCaption;
  toast.description = aDescription;
  toast.displayTime = displayTime > TOAST_MESSAGE_TIME + 500 ? displayTime : TOAST_MESSAGE_TIME + 500;

  m_notifications.push(toast);

  LeaveCriticalSection(&m_critical);
}


bool CGUIDialogKaiToast::DoWork()
{
  EnterCriticalSection(&m_critical);

  bool bPending = m_notifications.size() > 0;
  if (bPending && timeGetTime() - m_dwTimer > TOAST_MESSAGE_TIME)
  {
    Notification toast = m_notifications.front();

    m_toastDisplayTime = toast.displayTime;

    m_notifications.pop();

    g_graphicsContext.Lock();

    SET_CONTROL_LABEL(POPUP_CAPTION_TEXT, toast.caption);

    SET_CONTROL_LABEL(POPUP_NOTIFICATION_BUTTON, toast.description);

    CGUIImage *image = (CGUIImage *)GetControl(POPUP_ICON);
    if (image)
    {
      if (!toast.imagefile.IsEmpty())
        image->SetFileName(toast.imagefile);
      else
        image->SetFileName(m_defaultIcon);
    }

    g_graphicsContext.Unlock();

    //  Play the window specific init sound for each notification queued
    g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

    ResetTimer();
  }

  LeaveCriticalSection(&m_critical);

  return bPending;
}


void CGUIDialogKaiToast::ResetTimer()
{
  m_dwTimer = timeGetTime();
}

void CGUIDialogKaiToast::Render()
{
  CGUIDialog::Render();

  //  Fading does not count as display time
  if (IsAnimating(ANIM_TYPE_WINDOW_OPEN))
    ResetTimer();

  // now check if we should exit
  if (timeGetTime() - m_dwTimer > m_toastDisplayTime)
    Close();
}
