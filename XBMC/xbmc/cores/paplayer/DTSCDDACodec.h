#pragma once

#include "DTSCodec.h"

class DTSCDDACodec : public DTSCodec
{
public:
  DTSCDDACodec();
  virtual ~DTSCDDACodec();
  virtual __int64 Seek(__int64 iSeekTime);

protected:
  virtual bool CalculateTotalTime();
};
