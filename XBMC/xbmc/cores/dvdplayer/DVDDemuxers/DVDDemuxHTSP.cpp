/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "stdafx.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamHTSP.h"
#include "DVDDemuxHTSP.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h"

extern "C" {
#include "lib/libhts/net.h"
#include "lib/libhts/htsmsg.h"
#include "lib/libhts/htsmsg_binary.h"
}


CDVDDemuxHTSP::CDVDDemuxHTSP() : CDVDDemux()
{
}

CDVDDemuxHTSP::~CDVDDemuxHTSP()
{
  Dispose();
}

bool CDVDDemuxHTSP::Open(CDVDInputStream* input)
{
  Dispose();

  if(!input->IsStreamType(DVDSTREAM_TYPE_HTSP))
    return false;

  m_Input = (CDVDInputStreamHTSP*)input;

  while(m_Streams.size() == 0 && m_Status == "")
    CDVDDemuxUtils::FreeDemuxPacket(Read());

  return m_Streams.size() > 0;
}

void CDVDDemuxHTSP::Dispose()
{
}

void CDVDDemuxHTSP::Reset()
{
}


void CDVDDemuxHTSP::Flush()
{
}

DemuxPacket* CDVDDemuxHTSP::Read()
{
  htsmsg_t *  msg;
  const char* method;
  while((msg = m_Input->ReadStream()))
  {
    method = htsmsg_get_str(msg, "method");
    if(method == NULL)
      break;

    if     (strcmp("subscriptionStart",  method) == 0)
      SubscriptionStart(msg);
    else if(strcmp("subscriptionStop",   method) == 0)
      SubscriptionStop (msg);
    else if(strcmp("subscriptionStatus", method) == 0)
      SubscriptionStatus(msg);
    else if(strcmp("muxpkt"            , method) == 0)
    {
      uint32_t    index, duration;
      const void* bin;
      size_t      binlen;
      int64_t     ts;

      if(htsmsg_get_u32(msg, "stream" , &index)  ||
         htsmsg_get_bin(msg, "payload", &bin, &binlen))
        break;

      DemuxPacket* pkt = CDVDDemuxUtils::AllocateDemuxPacket(binlen);

      memcpy(pkt->pData, bin, binlen);
      pkt->iSize = binlen;

      if(!htsmsg_get_u32(msg, "duration", &duration))
        pkt->duration = (double)duration * DVD_TIME_BASE / 1000000;

      if(!htsmsg_get_s64(msg, "dts", &ts))
        pkt->dts = (double)ts * DVD_TIME_BASE / 1000000;
      else
        pkt->dts = DVD_NOPTS_VALUE;

      if(!htsmsg_get_s64(msg, "pts", &ts))
        pkt->pts = (double)ts * DVD_TIME_BASE / 1000000;
      else
        pkt->pts = DVD_NOPTS_VALUE;

      pkt->iStreamId = -1;
      for(int i = 0; i < (int)m_Streams.size(); i++)
      {
        if(m_Streams[i]->iPhysicalId == (int)index)
        {
          pkt->iStreamId = i;
          break;
        }
      }

      htsmsg_destroy(msg);
      return pkt;
    }

    break;
  }

  if(msg)
  {
    htsmsg_destroy(msg);
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }
  return NULL;
}

void CDVDDemuxHTSP::SubscriptionStart (htsmsg_t *m)
{
  htsmsg_t       *streams;
  htsmsg_field_t *f;

  if((streams = htsmsg_get_list(m, "streams")) == NULL)
  {
    CLog::Log(LOGERROR, "CDVDDemuxHTSP::SubscriptionStart - malformed message");
    return;
  }

  for(int i = 0; i < (int)m_Streams.size(); i++)
    delete m_Streams[i];
  m_Streams.clear();

  HTSMSG_FOREACH(f, streams)
  {
    uint32_t    index;
    const char* type;
    htsmsg_t* sub;

    if(f->hmf_type != HMF_MAP)
      continue;
    sub = &f->hmf_msg;

    if((type = htsmsg_get_str(sub, "type")) == NULL)
      continue;

    if(htsmsg_get_u32(sub, "index", &index))
      continue;

    union {
      CDemuxStream*      g;
      CDemuxStreamAudio* a;
      CDemuxStreamVideo* v;
    } st;

    CLog::Log(LOGDEBUG, "CDVDDemuxHTSP::SubscriptionStart - id: %d, type: %s", index, type);

    if(!strcmp(type, "AC3")) {
      st.a = new CDemuxStreamAudio();
      st.a->codec = CODEC_ID_AC3;
    } else if(!strcmp(type, "MPEG2AUDIO")) {
      st.a = new CDemuxStreamAudio();
      st.a->codec = CODEC_ID_MP2;
    } else if(!strcmp(type, "MPEG2VIDEO")) {
      st.v = new CDemuxStreamVideo();
      st.v->codec = CODEC_ID_MPEG2VIDEO;
    } else if(!strcmp(type, "H264")) {
      st.v = new CDemuxStreamVideo();
      st.v->codec = CODEC_ID_H264;
    } else {
      continue;
    }
    st.g->iId         = m_Streams.size();
    st.g->iPhysicalId = index;
    m_Streams.push_back(st.g);
  }
}
void CDVDDemuxHTSP::SubscriptionStop  (htsmsg_t *m)
{
  for(int i = 0; i < (int)m_Streams.size(); i++)
    delete m_Streams[i];
  m_Streams.clear();
}

void CDVDDemuxHTSP::SubscriptionStatus(htsmsg_t *m)
{
  const char* status;
  status = htsmsg_get_str(m, "status");
  if(status == NULL)
    m_Status = "";
  else
  {
    m_Status = status;
    CLog::Log(LOGDEBUG, "CDVDDemuxHTSP::SubscriptionStatus - %s", status);
  }
}


CDemuxStream* CDVDDemuxHTSP::GetStream(int iStreamId)
{
  if(iStreamId >= 0 && iStreamId < (int)m_Streams.size())
    return m_Streams[iStreamId];

  return NULL;
}

int CDVDDemuxHTSP::GetNrOfStreams()
{
  return m_Streams.size();
}

std::string CDVDDemuxHTSP::GetFileName()
{
  if(m_Input)
    return m_Input->GetFileName(); 
  else 
    return ""; 
}

