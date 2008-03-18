#pragma once

#include "LCD.h"
class CLCDFactory
{
public:
  CLCDFactory(void);
  virtual ~CLCDFactory(void);
  ILCD* Create();
};

