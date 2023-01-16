/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxCC.h"

#include "DVDDemuxUtils.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/contrib/cc_decoder708.h"
#include "cores/VideoPlayer/Interface/CaptionBlock.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"

#include <algorithm>

CDVDDemuxCC::CDVDDemuxCC() : m_hasData{false}, m_curPts{0.0}
{
}

CDVDDemuxCC::~CDVDDemuxCC()
{
  Dispose();
}

CDemuxStream* CDVDDemuxCC::GetStream(int iStreamId) const
{
  for (int i=0; i<GetNrOfStreams(); i++)
  {
    if (m_streams[i].uniqueId == iStreamId)
      return const_cast<CDemuxStreamSubtitle*>(&m_streams[i]);
  }
  return nullptr;
}

std::vector<CDemuxStream*> CDVDDemuxCC::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  int num = GetNrOfStreams();
  streams.reserve(num);
  for (int i = 0; i < num; ++i)
  {
    streams.push_back(const_cast<CDemuxStreamSubtitle*>(&m_streams[i]));
  }

  return streams;
}

int CDVDDemuxCC::GetNrOfStreams() const
{
  return m_streams.size();
}

DemuxPacket* CDVDDemuxCC::Process(CCaptionBlock* captionBlock)
{
  if (captionBlock)
  {
    m_ccTempBuffer.push_back(captionBlock);
  }

  if (!m_ccDecoder)
  {
    if (!OpenDecoder())
      return nullptr;
  }
  DemuxPacket* pPacket = Decode();
  return pPacket;
}

void CDVDDemuxCC::Handler(int service, void *userdata)
{
  CDVDDemuxCC *ctx = static_cast<CDVDDemuxCC*>(userdata);

  unsigned int idx;

  // switch back from 608 fallback if we got 708
  if (ctx->m_ccDecoder->m_seen608 && ctx->m_ccDecoder->m_seen708)
  {
    for (idx = 0; idx < ctx->m_streamdata.size(); idx++)
    {
      if (ctx->m_streamdata[idx].service == 0)
        break;
    }
    if (idx < ctx->m_streamdata.size())
    {
      ctx->m_streamdata.erase(ctx->m_streamdata.begin() + idx);
      ctx->m_ccDecoder->m_seen608 = false;
    }
    if (service == 0)
      return;
  }

  for (idx = 0; idx < ctx->m_streamdata.size(); idx++)
  {
    if (ctx->m_streamdata[idx].service == service)
      break;
  }
  if (idx >= ctx->m_streamdata.size())
  {
    CDemuxStreamSubtitle stream;
    stream.source = STREAM_SOURCE_VIDEOMUX;
    stream.language = "cc";
    stream.flags = FLAG_HEARING_IMPAIRED;
    stream.codec = AV_CODEC_ID_TEXT;
    stream.uniqueId = service;
    ctx->m_streams.push_back(std::move(stream));

    streamdata data;
    data.streamIdx = idx;
    data.service = service;
    ctx->m_streamdata.push_back(data);

    if (service == 0)
      ctx->m_ccDecoder->m_seen608 = true;
    else
      ctx->m_ccDecoder->m_seen708 = true;
  }

  ctx->m_streamdata[idx].pts = ctx->m_curPts;
  ctx->m_streamdata[idx].hasData = true;
  ctx->m_hasData = true;
}

bool CDVDDemuxCC::OpenDecoder()
{
  m_ccDecoder = std::make_unique<CDecoderCC708>();
  m_ccDecoder->Init(Handler, this);
  return true;
}

void CDVDDemuxCC::Dispose()
{
  m_streams.clear();
  m_streamdata.clear();
  m_ccDecoder.reset();

  while (!m_ccTempBuffer.empty())
  {
    delete m_ccTempBuffer.back();
    m_ccTempBuffer.pop_back();
  }
}

DemuxPacket* CDVDDemuxCC::Decode()
{
  DemuxPacket *pPacket = NULL;

  while (!m_hasData && !m_ccTempBuffer.empty())
  {
    CCaptionBlock* cc = m_ccTempBuffer.back();
    m_ccTempBuffer.pop_back();
    m_curPts = cc->GetPTS();
    m_ccDecoder->Decode(cc->GetData());
    delete cc;
  }

  if (m_hasData)
  {
    for (unsigned int i=0; i<m_streamdata.size(); i++)
    {
      if (m_streamdata[i].hasData)
      {
        int service = m_streamdata[i].service;

        char *data;
        int len;
        if (service == 0)
        {
          data = m_ccDecoder->m_cc608decoder->text;
          len = m_ccDecoder->m_cc608decoder->textlen;
        }
        else
        {
          data = m_ccDecoder->m_cc708decoders[service].text;
          len = m_ccDecoder->m_cc708decoders[service].textlen;
        }

        pPacket = CDVDDemuxUtils::AllocateDemuxPacket(len);
        pPacket->iSize = len;
        memcpy(pPacket->pData, data, pPacket->iSize);

        pPacket->iStreamId = service;
        pPacket->pts = m_streamdata[i].pts;
        pPacket->duration = 0;
        m_streamdata[i].hasData = false;
        break;
      }
      m_hasData = false;
    }
  }
  return pPacket;
}
