
#pragma once
#include "DVDCodecs\DVDVideoCodec.h"
struct AVPicture;

typedef struct SPUData
{
  BYTE* data;
  unsigned int iSize; // current data size
  unsigned int iNeededSize; // wanted packet size
  unsigned int iAllocatedSize;
  __int64 pts;
} SPUData;

typedef struct SPUInfo
{
  BYTE result[65536 + 20];
  BYTE* pData;//[65536 + 20]; // buffer for parsed rle data
  int pTFData; // pointer to top field picture data (needs rle parsing)
  int pBFData; // pointer to bottom field picture data (needs rle parsing)
  int x;
  int y;
  int width;
  int height;
  int iSPUSize; // size of picture data
  __int64 iPTSStartTime;
  __int64 iPTSStopTime;
  int iStream; //mpeg stream id
  bool bForced;
  // the four contrasts  [0] = background
  int alpha[4]; 
  
  // the four yuv colors, containing [][0] = Y, [][1] = Cr, [][2] = Cb
  // [0][] = background, [1][] = pattern, [2][] = emphasis1, [3][] = emphasis2
  int color[4][3];
  
  bool bHasColor;
  struct SPUInfo* next; // pointer to next SPU
} SPUInfo;

// upto 32 streams can exist
#define DVD_MAX_SPUSTREAMS 32

class CDVDDemuxSPU
{
public:
  CDVDDemuxSPU();
  virtual ~CDVDDemuxSPU();

  SPUInfo* AddData(BYTE* data, int iSize, int iStream, __int64 pts); // returns a packet from ParsePacket if possible
  //void FreePacket(SPUInfo* pSPUInfo);
  
  SPUInfo* ParseRLE(SPUInfo* pSPU);
  // void RenderI420(DVDVideoPicture* pPicture, SPUInfo* pSPU, bool b_crop);
  
  // m_clut set by libdvdnav once in a time
  // color lokup table is representing 16 different yuv colors
  // [][0] = Y, [][1] = Cr, [][2] = Cb
  BYTE      m_clut[16][3]; 
  bool      m_bHasClut;
  
protected:
  SPUInfo* ParsePacket(SPUData* pSPUData); // return value should be manualy freed with FreePacket
  
  SPUData  m_spuSreams[DVD_MAX_SPUSTREAMS]; 
};
