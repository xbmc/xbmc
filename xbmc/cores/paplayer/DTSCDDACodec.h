#pragma once

#include "DTSCodec.h"

#ifdef HAS_DTS_CODEC
class DTSCDDACodec : public DTSCodec
{
public:
  DTSCDDACodec();
  virtual ~DTSCDDACodec();
  virtual __int64 Seek(__int64 iSeekTime);

protected:
  virtual bool CalculateTotalTime();
};
#endif
