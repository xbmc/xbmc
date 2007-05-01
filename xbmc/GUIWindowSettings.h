#pragma once
#include "GUIWindow.h"

class CGUIWindowSettings :
      public CGUIWindow
{
public:
  CGUIWindowSettings(void);
  virtual ~CGUIWindowSettings(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
};
