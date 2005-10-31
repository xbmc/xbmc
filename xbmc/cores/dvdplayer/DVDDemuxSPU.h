
#pragma once
#include "DVDCodecs\Video\DVDVideoCodec.h"
#include "DVDOverlay.h"
struct AVPicture;

typedef struct SPUData
{
  BYTE* data;
  unsigned int iSize; // current data size
  unsigned int iNeededSize; // wanted packet size
  unsigned int iAllocatedSize;
  __int64 pts;
}
SPUData;

class CSPUInfo : public CDVDOverlaySpu
{
public:
  CSPUInfo() : CDVDOverlaySpu()
  {
    pData = result;
    bHasColor = false;
  }
  
  BYTE result[65536 + 20];
  bool bHasColor;
};

// upto 32 streams can exist
#define DVD_MAX_SPUSTREAMS 32

class CDVDDemuxSPU
{
public:
  CDVDDemuxSPU();
  virtual ~CDVDDemuxSPU();

  CSPUInfo* AddData(BYTE* data, int iSize, __int64 pts); // returns a packet from ParsePacket if possible
  //void FreePacket(SPUInfo* pSPUInfo);

  CSPUInfo* ParseRLE(CSPUInfo* pSPU, BYTE* pUnparsedData);
  // void RenderI420(DVDVideoPicture* pPicture, SPUInfo* pSPU, bool b_crop);

  // m_clut set by libdvdnav once in a time
  // color lokup table is representing 16 different yuv colors
  // [][0] = Y, [][1] = Cr, [][2] = Cb
  BYTE m_clut[16][3];
  bool m_bHasClut;

protected:
  CSPUInfo* ParsePacket(SPUData* pSPUData); // return value should be manualy freed with FreePacket

  SPUData m_spuData;
};
