/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDDemuxUtils.h"
#include "DVDClock.h"
#include "DemuxCrypto.h"
#include "utils/log.h"
#include "system.h"

#ifdef TARGET_POSIX
#include "linux/XMemUtils.h"
#endif

extern "C" {
#include "libavcodec/avcodec.h"
}

void CDVDDemuxUtils::FreeDemuxPacket(DemuxPacket* pPacket)
{
  if (pPacket)
  {
    try {
      if (pPacket->pData) _aligned_free(pPacket->pData);
      delete pPacket;
    }
    catch(...) {
      CLog::Log(LOGERROR, "%s - Exception thrown while freeing packet", __FUNCTION__);
    }
  }
}

DemuxPacket* CDVDDemuxUtils::AllocateDemuxPacket(int iDataSize)
{
  DemuxPacket* pPacket = new DemuxPacket;
  if (!pPacket) return NULL;

  try
  {
    memset(pPacket, 0, sizeof(DemuxPacket));

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
      pPacket->pData =(uint8_t*)_aligned_malloc(iDataSize + FF_INPUT_BUFFER_PADDING_SIZE, 16);
      if (!pPacket->pData)
      {
        FreeDemuxPacket(pPacket);
        return NULL;
      }

      // reset the last 8 bytes to 0;
      memset(pPacket->pData + iDataSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
    }

    // setup defaults
    pPacket->dts       = DVD_NOPTS_VALUE;
    pPacket->pts       = DVD_NOPTS_VALUE;
    pPacket->iStreamId = -1;
    pPacket->dispTime = 0;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown", __FUNCTION__);
    FreeDemuxPacket(pPacket);
    pPacket = NULL;
  }
  return pPacket;
}

DemuxPacket* CDVDDemuxUtils::AllocateDemuxPacket(unsigned int iDataSize, unsigned int encryptedSubsampleCount)
{
  DemuxPacket *ret(AllocateDemuxPacket(iDataSize));
  if (ret && encryptedSubsampleCount > 0)
    ret->cryptoInfo = std::shared_ptr<DemuxCryptoInfo>(new DemuxCryptoInfo(encryptedSubsampleCount));
  return ret;
}
