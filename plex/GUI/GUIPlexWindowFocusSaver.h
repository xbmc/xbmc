#ifndef GUIPLEXWINDOWFOCUSSAVER_H
#define GUIPLEXWINDOWFOCUSSAVER_H

#include "guilib/GUIWindow.h"

class CGUIPlexWindowFocusSaver
{
private:
  CGUIWindow* m_window;
  int m_lastFocusedControlID;
  int m_lastFocusedControlItem;

public:
  CGUIPlexWindowFocusSaver();
  void SaveFocus(CGUIWindow* window);
  void RestoreFocus(bool reset);

  inline int getLastFocusedControlID() { return m_lastFocusedControlID; }
};

#endif // GUIPLEXWINDOWFOCUSSAVER_H
