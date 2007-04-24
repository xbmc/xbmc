#pragma once

#include "lcd.h"
class CLCDFactory
{
public:
  CLCDFactory(void);
  virtual ~CLCDFactory(void);
  ILCD* Create();
};

