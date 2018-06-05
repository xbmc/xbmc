#pragma once

#include "libexif.h"

class CIptcParse
{
  public:
    static bool Process(const unsigned char* const Data, const unsigned short length, IPTCInfo_t *info);
};

