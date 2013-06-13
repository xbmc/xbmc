#pragma once

#include "video/windows/GUIWindowVideoBase.h"

class CGUIWindowPlexMyChannels : public CGUIWindowVideoBase {
public:
  CGUIWindowPlexMyChannels();
  
  bool OnClick(int iItem);
};