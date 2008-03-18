
#pragma once
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDCodecs/Overlay/DVDOverlaySpu.h"
struct AVPicture;

typedef struct SPUData
{
  BYTE* data;
  unsigned int iSize; // current data size
  unsigned int iNeededSize; // wanted packet size
  unsigned int iAllocatedSize;
  double pts;
}
SPUData;

class CSPUInfo : public CDVDOverlaySpu
{
public:
  CSPUInfo() : CDVDOverlaySpu()
  {
    pData = result;
  }
  
  BYTE result[2*65536 + 20];
};

// upto 32 streams can exist
#define DVD_MAX_SPUSTREAMS 32

class CDVDDemuxSPU
{
public:
  CDVDDemuxSPU();
  virtual ~CDVDDemuxSPU();

  CSPUInfo* AddData(BYTE* data, int iSize, double pts); // returns a packet from ParsePacket if possible

  CSPUInfo* ParseRLE(CSPUInfo* pSPU, BYTE* pUnparsedData);
  void FindSubtitleColor(int last_color, int stats[4], CSPUInfo* pSPU);

  void Reset();
  void FlushCurrentPacket(); // flushes current unparsed data
  
  // m_clut set by libdvdnav once in a time
  // color lokup table is representing 16 different yuv colors
  // [][0] = Y, [][1] = Cr, [][2] = Cb
  BYTE m_clut[16][3];
  bool m_bHasClut;

protected:
  CSPUInfo* ParsePacket(SPUData* pSPUData);

  SPUData m_spuData;
};
