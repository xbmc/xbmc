#pragma once

#include "GUIWindow.h"

class CGUIWindowStartup :
      public CGUIWindow
{
public:
  CGUIWindowStartup(void);
  virtual ~CGUIWindowStartup(void);
  virtual void OnMouseAction() {}; // dummy implementation that ignores mouse on startup
};
