
#pragma once

#include "DVDDemux.h"

// forward declarations
void fast_memcpy(void* d, const void* s, unsigned n);
void fast_memset(void* d, int c, unsigned int n);

class CDVDDemuxUtils
{
public:
  static void FreeDemuxPacket(CDVDDemux::DemuxPacket* pPacket);
  static CDVDDemux::DemuxPacket* AllocateDemuxPacket();
};

