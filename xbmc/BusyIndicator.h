#pragma once
#include "ApplicationRenderer.h"

class CBusyIndicator
{
public:
  CBusyIndicator(){g_ApplicationRenderer.SetBusy(true);};
  virtual ~CBusyIndicator(){g_ApplicationRenderer.SetBusy(false);};
};

