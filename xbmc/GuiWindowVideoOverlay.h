#pragma once
#include "GUIWindow.h"

class CGUIWindowVideoOverlay: public CGUIWindow
{
public:
  CGUIWindowVideoOverlay(void);
  virtual ~CGUIWindowVideoOverlay(void);
  virtual void Render();
  void Update();
protected:
  virtual void OnWindowLoaded();
};
