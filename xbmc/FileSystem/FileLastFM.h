#pragma once

#include "FileCurl.h"
#include "RingBuffer.h"
#include "../cores/paplayer/RingHoldBuffer.h"

namespace XFILE
{

  class CFileLastFM : public CFileCurl
  {
  public:
    CFileLastFM();
    virtual ~CFileLastFM();
  protected:
  };

}
