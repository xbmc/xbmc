#pragma once

#include "GUIWindow.h"

class CGUIWindowWebBrowser : public CGUIWindow
{
public:
  CGUIWindowWebBrowser();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
};
