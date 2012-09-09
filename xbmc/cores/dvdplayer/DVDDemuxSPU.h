#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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

// upto 32 streams can exist
#define DVD_MAX_SPUSTREAMS 32

class CDVDDemuxSPU
{
public:
  CDVDDemuxSPU();
  virtual ~CDVDDemuxSPU();

  CDVDOverlaySpu* AddData(BYTE* data, int iSize, double pts); // returns a packet from ParsePacket if possible

  CDVDOverlaySpu* ParseRLE(CDVDOverlaySpu* pSPU, BYTE* pUnparsedData);
  void FindSubtitleColor(int last_color, int stats[4], CDVDOverlaySpu* pSPU);
  bool CanDisplayWithAlphas(int a[4], int stats[4]);

  void Reset();
  void FlushCurrentPacket(); // flushes current unparsed data

  // m_clut set by libdvdnav once in a time
  // color lokup table is representing 16 different yuv colors
  // [][0] = Y, [][1] = Cr, [][2] = Cb
  BYTE m_clut[16][3];
  bool m_bHasClut;

protected:
  CDVDOverlaySpu* ParsePacket(SPUData* pSPUData);

  SPUData m_spuData;
};
