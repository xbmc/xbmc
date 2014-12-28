/*
 *      Copyright (C) 2005-2014 Team XBMC
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
#include "DVDDemuxCC.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/contrib/cc_decoder708.h"
#include "utils/log.h"

class CCaptionBlock
{
public:
  CCaptionBlock(int size)
  {
    m_data = (uint8_t*)malloc(size);
    m_size = size;
  }
  virtual ~CCaptionBlock()
  {
    free(m_data);
  }
  double m_pts;
  uint8_t *m_data;
  int m_size;
};

bool reorder_sort (CCaptionBlock *lhs, CCaptionBlock *rhs)
{
  return (lhs->m_pts > rhs->m_pts);
}

CDVDDemuxCC::CDVDDemuxCC()
{
  m_hasData = false;
  m_ccDecoder = NULL;
}

CDVDDemuxCC::~CDVDDemuxCC()
{
  Dispose();
}

CDemuxStream* CDVDDemuxCC::GetStream(int iStreamId)
{
  return &m_streams[iStreamId];
}

int CDVDDemuxCC::GetNrOfStreams()
{
  return m_streams.size();
}

DemuxPacket* CDVDDemuxCC::Read(DemuxPacket *pSrcPacket)
{
  DemuxPacket *pPacket = NULL;
  uint32_t startcode = 0xffffffff;
  int picType = 0;
  int p = 0;
  int len;

  if (!pSrcPacket)
  {
    pPacket = Decode();
    return pPacket;
  }
  if (pSrcPacket->pts == DVD_NOPTS_VALUE)
  {
    return pPacket;
  }

  while (!m_ccTempBuffer.empty())
  {
    m_ccReorderBuffer.push_back(m_ccTempBuffer.back());
    m_ccTempBuffer.pop_back();
  }

  while ((len = pSrcPacket->iSize - p) > 3)
  {
    if ((startcode & 0xffffff00) == 0x00000100)
    {
      int scode = startcode & 0xFF;
      if (scode == 0x00)
      {
        if (len > 4)
        {
          uint8_t *buf = pSrcPacket->pData + p;
          picType = (buf[1] & 0x38) >> 3;
        }
      }
      else if (scode == 0xb2) // user data
      {
        uint8_t *buf = pSrcPacket->pData + p;
        if (len >= 6 &&
            buf[0] == 'G' && buf[1] == 'A' && buf[2] == '9' && buf[3] == '4' &&
            buf[4] == 3 && (buf[5] & 0x40))
        {
          int cc_count = buf[5] & 0x1f;
          if (cc_count > 0 && len >= 7 + cc_count * 3)
          {
            CCaptionBlock *cc = new CCaptionBlock(cc_count * 3);
            memcpy(cc->m_data, buf+7, cc_count * 3);
            cc->m_pts = pSrcPacket->pts;
            if (picType == 1 || picType == 2)
              m_ccTempBuffer.push_back(cc);
            else
              m_ccReorderBuffer.push_back(cc);
          }
        }
      }
    }
    startcode = startcode << 8 | pSrcPacket->pData[p++];
  }

  if ((picType == 1 || picType == 2) && !m_ccReorderBuffer.empty())
  {
    if (!m_ccDecoder)
    {
      if (!OpenDecoder())
        return NULL;
    }
    std::sort(m_ccReorderBuffer.begin(), m_ccReorderBuffer.end(), reorder_sort);
    pPacket = Decode();
  }
  return pPacket;
}

void CDVDDemuxCC::Handler(int service, void *userdata)
{
  CDVDDemuxCC *ctx = (CDVDDemuxCC*)userdata;

  int idx;
  for (idx = 0; idx < ctx->m_streamdata.size(); idx++)
  {
    if (ctx->m_streamdata[idx].service == service)
      break;
  }
  if (idx >= ctx->m_streamdata.size())
  {
    CDemuxStreamSubtitle stream;
    strcpy(stream.language, "cc");
    stream.codec = AV_CODEC_ID_TEXT;
    stream.iPhysicalId = service;
    stream.iId = idx;
    ctx->m_streams.push_back(stream);

    streamdata data;
    data.streamIdx = idx;
    data.service = service;
    ctx->m_streamdata.push_back(data);
  }

  ctx->m_streamdata[idx].pts = ctx->m_curPts;
  ctx->m_streamdata[idx].hasData = true;
  ctx->m_hasData = true;
}

bool CDVDDemuxCC::OpenDecoder()
{
  m_ccDecoder = new CDecoderCC708();
  m_ccDecoder->Init(Handler, this);
  return true;
}

void CDVDDemuxCC::Dispose()
{
  m_streams.clear();
  m_streamdata.clear();
  delete m_ccDecoder;
  m_ccDecoder = NULL;

  while (!m_ccReorderBuffer.empty())
  {
    delete m_ccReorderBuffer.back();
    m_ccReorderBuffer.pop_back();
  }
  while (!m_ccTempBuffer.empty())
  {
    delete m_ccTempBuffer.back();
    m_ccTempBuffer.pop_back();
  }
}

DemuxPacket* CDVDDemuxCC::Decode()
{
  DemuxPacket *pPacket = NULL;

  while(!m_hasData && !m_ccReorderBuffer.empty())
  {
    CCaptionBlock *cc = m_ccReorderBuffer.back();
    m_ccReorderBuffer.pop_back();
    m_curPts = cc->m_pts;
    m_ccDecoder->Decode(cc->m_data, cc->m_size);
    delete cc;
  }

  if (m_hasData)
  {
    for (int i=0; i<m_streamdata.size(); i++)
    {
      if (m_streamdata[i].hasData)
      {
        int service = m_streamdata[i].service;
        pPacket = CDVDDemuxUtils::AllocateDemuxPacket(m_ccDecoder->m_cc708decoders[service].textlen);
        pPacket->iSize = m_ccDecoder->m_cc708decoders[service].textlen;
        memcpy(pPacket->pData, m_ccDecoder->m_cc708decoders[service].text, pPacket->iSize);
        pPacket->iStreamId = i;
        pPacket->pts = m_streamdata[i].pts;
        pPacket->duration = 0;
        m_streamdata[i].hasData = false;
        break;
      }
    }
    m_hasData = false;
  }
  return pPacket;
}
