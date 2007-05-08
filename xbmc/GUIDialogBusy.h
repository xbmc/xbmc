#pragma once
#include "guiDialog.h"


class CGUIDialogBusy: public CGUIDialog
{
public:
  CGUIDialogBusy(void);
  virtual ~CGUIDialogBusy(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void Render();
  void DoRender();

protected:
  DWORD m_dwTimer;

  void ResetTimer();

};
