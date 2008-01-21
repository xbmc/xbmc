#pragma once

#include "GUIWindow.h"

class CGUIWindowOSD : public CGUIDialog
{
public:

  CGUIWindowOSD(void);
  virtual ~CGUIWindowOSD(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();
};
