#pragma once

#include "GUIDialog.h"

class CGUIWindowVideoOverlay: public CGUIDialog
{
public:
  CGUIWindowVideoOverlay(void);
  virtual ~CGUIWindowVideoOverlay(void);
  virtual void Render();
protected:
  virtual void SetDefaults();
};
