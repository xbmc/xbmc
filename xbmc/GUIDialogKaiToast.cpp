#include "stdafx.h"
#include "GUIDialogKaiToast.h"
#include "GUISliderControl.h"
#include "application.h"
#include "GUIAudioManager.h"


// May need to change this so that it is "modeless" rather than Modal,
// though it works reasonably well as is...

#define TOAST_DISPLAY_TIME   5000L
#define TOAST_MESSAGE_TIME   TOAST_DISPLAY_TIME-1000L

#define POPUP_ICON     400
#define POPUP_CAPTION_TEXT   401
#define POPUP_NOTIFICATION_BUTTON 402

CGUIDialogKaiToast::CGUIDialogKaiToast(void)
: CGUIDialog(WINDOW_DIALOG_KAI_TOAST, "DialogKaiToast.xml")
{
  m_pIcon = NULL;
  m_iIconPosX = 0;
  m_iIconPosY = 0;
  m_dwIconWidth = 0;
  m_dwIconHeight = 0;
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

      CGUIImage* pIcon = (CGUIImage*) GetControl(POPUP_ICON);

      if (pIcon)
      {
        m_iIconPosX = pIcon->GetXPosition();
        m_iIconPosY = pIcon->GetYPosition();
        m_dwIconWidth = pIcon->GetWidth();
        m_dwIconHeight = pIcon->GetHeight();
      }

      ResetTimer();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_pIcon)
      {
        m_pIcon->FreeResources();
        delete m_pIcon;
        m_pIcon = NULL;
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}


void CGUIDialogKaiToast::QueueNotification(const CStdString& aCaption, const CStdString& aDescription)
{
  EnterCriticalSection(&m_critical);

  Notification toast;
  toast.caption = aCaption;
  toast.description = aDescription;
  m_notifications.push(toast);

  LeaveCriticalSection(&m_critical);
}

void CGUIDialogKaiToast::QueueNotification(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription)
{
  EnterCriticalSection(&m_critical);

  Notification toast;
  toast.imagefile = aImageFile;
  toast.caption = aCaption;
  toast.description = aDescription;
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
    m_notifications.pop();

    g_graphicsContext.Lock();

    CGUIMessage msg1(GUI_MSG_LABEL_SET, GetID(), POPUP_CAPTION_TEXT);
    msg1.SetLabel(toast.caption);
    OnMessage(msg1);

    CGUIMessage msg2(GUI_MSG_LABEL_SET, GetID(), POPUP_NOTIFICATION_BUTTON);
    msg2.SetLabel(toast.description);
    OnMessage(msg2);

    if (m_pIcon)
    {
      m_pIcon->FreeResources();
      delete m_pIcon;
      m_pIcon = NULL;
    }

    if (toast.imagefile.size()>0)
    {
      m_pIcon = new CGUIImage(0, 0, 0, 0, m_dwIconWidth, m_dwIconHeight, toast.imagefile);
      m_pIcon->AllocResources();
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
  if (m_bRunning)
  {
    if (m_pIcon)
    {
      SET_CONTROL_HIDDEN(POPUP_ICON);
      CGUIDialog::Render();
      m_pIcon->Render(m_iIconPosX + m_iPosX, m_iIconPosY + m_iPosY, m_dwIconWidth, m_dwIconHeight);
    }
    else
    {
      SET_CONTROL_VISIBLE(POPUP_ICON);
      CGUIDialog::Render();
    }

    // now check if we should exit
    if (timeGetTime() - m_dwTimer > TOAST_DISPLAY_TIME)
    {
      Close();
    }
  }
}
