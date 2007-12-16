#pragma once
#include "GUIDialog.h"

class CGUIDialogAccessPoints : public CGUIDialog
{
public:
  CGUIDialogAccessPoints(void);
  virtual ~CGUIDialogAccessPoints(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
};
