/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxUtils.h"

#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

void CDVDDemuxUtils::FreeDemuxPacket(DemuxPacket* pPacket)
{
  if (pPacket)
  {
    if (pPacket->pData)
      KODI::MEMORY::AlignedFree(pPacket->pData);
    if (pPacket->iSideDataElems)
    {
      AVPacket avPkt;
      av_init_packet(&avPkt);
      avPkt.side_data = static_cast<AVPacketSideData*>(pPacket->pSideData);
      avPkt.side_data_elems = pPacket->iSideDataElems;
      av_packet_free_side_data(&avPkt);
    }
    if (pPacket->cryptoInfo)
      delete pPacket->cryptoInfo;
    delete pPacket;
  }
}

DemuxPacket* CDVDDemuxUtils::AllocateDemuxPacket(int iDataSize)
{
  DemuxPacket* pPacket = new DemuxPacket();

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
    pPacket->pData = static_cast<uint8_t*>(KODI::MEMORY::AlignedMalloc(iDataSize + AV_INPUT_BUFFER_PADDING_SIZE, 16));
    if (!pPacket->pData)
    {
      FreeDemuxPacket(pPacket);
      return NULL;
    }

    // reset the last 8 bytes to 0;
    memset(pPacket->pData + iDataSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
  }

  return pPacket;
}

DemuxPacket* CDVDDemuxUtils::AllocateDemuxPacket(unsigned int iDataSize, unsigned int encryptedSubsampleCount)
{
  DemuxPacket *ret(AllocateDemuxPacket(iDataSize));
  if (ret && encryptedSubsampleCount > 0)
    ret->cryptoInfo = new DemuxCryptoInfo(encryptedSubsampleCount);
  return ret;
}

void CDVDDemuxUtils::StoreSideData(DemuxPacket *pkt, AVPacket *src)
{
  AVPacket avPkt;
  av_init_packet(&avPkt);
  av_packet_copy_props(&avPkt, src);
  pkt->pSideData = avPkt.side_data;
  pkt->iSideDataElems = avPkt.side_data_elems;
}
