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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DVDDemuxUtils.h"
#include "DVDClock.h"
#include "utils/log.h"
extern "C" {
#if (defined USE_EXTERNAL_FFMPEG)
  #if (defined HAVE_LIBAVCODEC_AVCODEC_H)
    #include <libavcodec/avcodec.h>
  #else
    #include <ffmpeg/avcodec.h>
  #endif
#else
  #include "libavcodec/avcodec.h"
#endif
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
      pPacket->pData =(BYTE*)_aligned_malloc(iDataSize + FF_INPUT_BUFFER_PADDING_SIZE, 16);
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
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown", __FUNCTION__);
    FreeDemuxPacket(pPacket);
    pPacket = NULL;
  }
  return pPacket;
}
