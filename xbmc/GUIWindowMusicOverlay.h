#pragma once
#include "GUIWindow.h"

class CGUIWindowMusicOverlay: public CGUIDialog
{
public:
  CGUIWindowMusicOverlay(void);
  virtual ~CGUIWindowMusicOverlay(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnMouse(const CPoint &point);
  virtual void Render();
protected:
  virtual void SetDefaults();
};
