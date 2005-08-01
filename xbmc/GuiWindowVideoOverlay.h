#pragma once
#include "GUIWindow.h"

class CGUIWindowVideoOverlay: public CGUIDialog
{
public:
  CGUIWindowVideoOverlay(void);
  virtual ~CGUIWindowVideoOverlay(void);
  virtual void Render();
};
