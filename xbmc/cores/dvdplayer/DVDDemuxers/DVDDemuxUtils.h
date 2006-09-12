
#pragma once

#include "DVDDemux.h"

class CDVDDemuxUtils
{
public:
  static void FreeDemuxPacket(CDVDDemux::DemuxPacket* pPacket);
  static CDVDDemux::DemuxPacket* AllocateDemuxPacket(int iDataSize = 0);
};

