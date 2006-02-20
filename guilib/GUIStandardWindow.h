#pragma once
#include "GUIWindow.h"

// This class is designed to be the base class for any standard
// full screen window.  Default implementations for action keys
// can be placed into this class to make creating new window
// classes that much easier.

class CGUIStandardWindow :
      public CGUIWindow
{
public:
  CGUIStandardWindow(void);
  virtual ~CGUIStandardWindow(void);

  virtual bool OnAction(const CAction &action);
};
