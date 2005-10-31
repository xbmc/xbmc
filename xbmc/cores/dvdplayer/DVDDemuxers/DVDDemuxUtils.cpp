
#include "../../../stdafx.h"
#include "DVDDemuxUtils.h"

#define INPUT_BUFFER_PADDING_SIZE 8

void CDVDDemuxUtils::FreeDemuxPacket(CDVDDemux::DemuxPacket* pPacket)
{
  if (pPacket->pData) delete[] pPacket->pData;
  delete pPacket;
}

CDVDDemux::DemuxPacket* CDVDDemuxUtils::AllocateDemuxPacket(int iDataSize)
{
  CDVDDemux::DemuxPacket* pPacket = new CDVDDemux::DemuxPacket;
  if (!pPacket) return NULL;

  fast_memset(pPacket, 0, sizeof(CDVDDemux::DemuxPacket));
  
  if (iDataSize > 0)
  {
    // need to allocate a few bytes more.
    // From avcodec.h (ffmpeg)
    /**
      * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
      * this is mainly needed because some optimized bitstream readers read 
      * 32 or 64 bit at once and could read over the end<br>
      * Note, if the first 23 bits of the additional bytes are not 0 then damaged
      * MPEG bitstreams could cause overread and segfault
      */ 
    
    pPacket->pData = new BYTE[iDataSize + INPUT_BUFFER_PADDING_SIZE];
    if (!pPacket->pData)
    {
      FreeDemuxPacket(pPacket);
      return NULL;
    }
    
    // reset the last 8 bytes to 0;
    memset(pPacket->pData + iDataSize, 0, INPUT_BUFFER_PADDING_SIZE);
  }
  
  return pPacket;
}
