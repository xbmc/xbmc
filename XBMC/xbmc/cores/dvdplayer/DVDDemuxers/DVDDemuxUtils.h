
#pragma once

#include "DVDDemux.h"

class CDVDDemuxUtils
{
public:
  static void FreeDemuxPacket(DemuxPacket* pPacket);
  static DemuxPacket* AllocateDemuxPacket(int iDataSize = 0);
};

