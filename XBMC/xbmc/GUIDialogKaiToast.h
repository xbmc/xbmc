#pragma once
#include "guiDialog.h"

#include <queue>

class CGUIImage;

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
  };

  void QueueNotification(const CStdString& aCaption, const CStdString& aDescription);
  void QueueNotification(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription);
  bool DoWork();

  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
  void ResetTimer();

protected:

  DWORD m_dwTimer;
  CGUIImage* m_pIcon;

  INT m_iIconPosX;
  INT m_iIconPosY;
  DWORD m_dwIconWidth;
  DWORD m_dwIconHeight;

  typedef std::queue<Notification> TOASTQUEUE;
  TOASTQUEUE m_notifications;
  CRITICAL_SECTION m_critical;
};
