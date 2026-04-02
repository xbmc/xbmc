/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxUtils.h"

#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <algorithm>

void CDVDDemuxUtils::FreeDemuxPacket(DemuxPacket* pPacket)
{
  if (pPacket)
  {
    if (pPacket->pData)
      KODI::MEMORY::AlignedFree(pPacket->pData);
    if (pPacket->iSideDataElems)
    {
      AVPacket* avPkt = av_packet_alloc();
      if (!avPkt)
      {
        CLog::Log(LOGERROR, "CDVDDemuxUtils::{} - av_packet_alloc failed: {}", __FUNCTION__,
                  strerror(errno));
      }
      else
      {
        avPkt->side_data = static_cast<AVPacketSideData*>(pPacket->pSideData);
        avPkt->side_data_elems = pPacket->iSideDataElems;

        //! @todo: properly handle avpkt side_data. this works around our improper use of the side_data
        // as we pass pointers to ffmpeg allocated memory for the side_data. we should really be allocating
        // and storing our own AVPacket. This will require some extensive changes.

        // here we make use of ffmpeg to free the side_data, we shouldn't have to allocate an intermediate AVPacket though
        av_packet_free(&avPkt);
      }
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
  AVPacket* avPkt = av_packet_alloc();
  if (!avPkt)
  {
    CLog::Log(LOGERROR, "CDVDDemuxUtils::{} - av_packet_alloc failed: {}", __FUNCTION__,
              strerror(errno));
    return;
  }

  // here we make allocate an intermediate AVPacket to allow ffmpeg to allocate the side_data
  // via the copy below. we then reference this allocated memory in the DemuxPacket. this behaviour
  // is bad and will require a larger rework.
  av_packet_copy_props(avPkt, src);
  pkt->pSideData = avPkt->side_data;
  pkt->iSideDataElems = avPkt->side_data_elems;

  //! @todo: properly handle avpkt side_data. this works around our improper use of the side_data
  // as we pass pointers to ffmpeg allocated memory for the side_data. we should really be allocating
  // and storing our own AVPacket. This will require some extensive changes.
  av_buffer_unref(&avPkt->buf);
  av_free(avPkt);
}

std::vector<ChapterFFmpeg> CDVDDemuxUtils::LoadChapters(std::span<AVChapter*> chapters)
{
  std::vector<ChapterFFmpeg> result;

  if (chapters.empty() || chapters.data() == nullptr)
    return result;

  result.reserve(chapters.size());

  std::ranges::transform(chapters, std::back_inserter(result),
                         [](const AVChapter* chapter)
                         {
                           ChapterFFmpeg newChapter{};

                           newChapter.m_startPts = chapter->start * av_q2d(chapter->time_base);
                           newChapter.m_endPts = chapter->end * av_q2d(chapter->time_base);

                           const AVDictionaryEntry* titleTag =
                               av_dict_get(chapter->metadata, "title", nullptr, 0);

                           if (titleTag)
                             newChapter.m_name = titleTag->value;

                           return newChapter;
                         });

  std::ranges::sort(result, std::less(), &ChapterFFmpeg::m_startPts);

  return result;
}
