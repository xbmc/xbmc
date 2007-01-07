#pragma once

#include "AC3Codec.h"

#ifdef HAS_AC3_CODEC
class AC3CDDACodec : public AC3Codec
{
public:
  AC3CDDACodec();
  virtual ~AC3CDDACodec();
  virtual __int64 Seek(__int64 iSeekTime);

protected:
  virtual bool CalculateTotalTime();
};
#endif
