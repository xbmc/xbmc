#pragma once
#include "GUIDialog.h"

#include <queue>

class CGUIImage;

#define TOAST_DISPLAY_TIME   5000L  // default 5 seconds
#define TOAST_MESSAGE_TIME   1000L  // minimal message time 1 second

class CGUIDialogKaiToast: public CGUIDialog
{
public:
  CGUIDialogKaiToast(void);
  virtual ~CGUIDialogKaiToast(void);

  struct Notification
  {
    CStdString caption;
    CStdString description;
    CStdString imagefile;
    unsigned int displayTime;
  };

  void QueueNotification(const CStdString& aCaption, const CStdString& aDescription);
  void QueueNotification(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime = TOAST_DISPLAY_TIME);
  bool DoWork();

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void Render();
  void ResetTimer();

protected:

  DWORD m_dwTimer;

  DWORD m_toastDisplayTime;

  CStdString m_defaultIcon;

  typedef std::queue<Notification> TOASTQUEUE;
  TOASTQUEUE m_notifications;
  CRITICAL_SECTION m_critical;
};
