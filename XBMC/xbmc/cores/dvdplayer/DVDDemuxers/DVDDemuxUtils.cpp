
#include "../../../stdafx.h"
#include "DVDDemuxUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CDVDDemuxUtils::FreeDemuxPacket(CDVDDemux::DemuxPacket* pPacket)
{
  if (pPacket->pData) delete pPacket->pData;
  delete pPacket;
}

CDVDDemux::DemuxPacket* CDVDDemuxUtils::AllocateDemuxPacket()
{
  CDVDDemux::DemuxPacket* pPacket = new CDVDDemux::DemuxPacket;
  if (!pPacket) return NULL;

  fast_memset(pPacket, 0, sizeof(CDVDDemux::DemuxPacket));
  return pPacket;
}
