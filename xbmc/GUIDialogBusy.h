#pragma once
#include "GUIDialog.h"


class CGUIDialogBusy: public CGUIDialog
{
public:
  CGUIDialogBusy(void);
  virtual ~CGUIDialogBusy(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void Render();

protected:
};
