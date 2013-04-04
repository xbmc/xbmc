#pragma once

#include "windows/GUIMediaWindow.h"

class CGUIWindowPlexPreplayVideo : public CGUIMediaWindow
{
public:
  CGUIWindowPlexPreplayVideo(void);
  virtual ~CGUIWindowPlexPreplayVideo();
  
  virtual bool OnMessage(CGUIMessage& message);
};