/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>

struct AVFrame;
class CDVDOverlaySpu;

typedef struct SPUData
{
  uint8_t* data;
  unsigned int iSize; // current data size
  unsigned int iNeededSize; // wanted packet size
  unsigned int iAllocatedSize;
  double pts;
}
SPUData;

// upto 32 streams can exist
#define DVD_MAX_SPUSTREAMS 32

class CDVDDemuxSPU final
{
public:
  CDVDDemuxSPU();
  ~CDVDDemuxSPU();

  std::shared_ptr<CDVDOverlaySpu> AddData(
      uint8_t* data, int iSize, double pts); // returns a packet from ParsePacket if possible

  std::shared_ptr<CDVDOverlaySpu> ParseRLE(std::shared_ptr<CDVDOverlaySpu> pSPU,
                                           uint8_t* pUnparsedData);
  static void FindSubtitleColor(int last_color, int stats[4], CDVDOverlaySpu& pSPU);
  static bool CanDisplayWithAlphas(const int a[4], const int stats[4]);

  void Reset();
  void FlushCurrentPacket(); // flushes current unparsed data

  // m_clut set by libdvdnav once in a time
  // color lookup table is representing 16 different yuv colors
  // [][0] = Y, [][1] = Cr, [][2] = Cb
  uint8_t m_clut[16][3];
  bool m_bHasClut;

protected:
  std::shared_ptr<CDVDOverlaySpu> ParsePacket(SPUData* pSPUData);

  SPUData m_spuData;
};
