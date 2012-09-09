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


#include "DVDCodecs/DVDCodecs.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamHTSP.h"
#include "DVDDemuxHTSP.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/log.h"
#include <arpa/inet.h>

extern "C" {
#include "lib/libhts/net.h"
#include "lib/libhts/htsmsg.h"
#include "lib/libhts/htsmsg_binary.h"
}

using namespace std;
using namespace HTSP;

class CDemuxStreamVideoHTSP
  : public CDemuxStreamVideo
{
  CDVDDemuxHTSP *m_parent;
  string         m_codec;
public:
  CDemuxStreamVideoHTSP(CDVDDemuxHTSP *parent, const string& codec)
    : m_parent(parent)
    , m_codec(codec)
  {}
  void GetStreamInfo(std::string& strInfo)
  {
    CStdString info;
    info.Format("%s, delay: %u, drops: %ub %up %ui"
               , m_codec.c_str()
               , m_parent->m_QueueStatus.delay
               , m_parent->m_QueueStatus.bdrops
               , m_parent->m_QueueStatus.pdrops
               , m_parent->m_QueueStatus.idrops);
    strInfo = info;
  }
};

class CDemuxStreamAudioHTSP
  : public CDemuxStreamAudio
{
  CDVDDemuxHTSP *m_parent;
  string         m_codec;
public:
  CDemuxStreamAudioHTSP(CDVDDemuxHTSP *parent, const string& codec)
    : m_parent(parent)
    , m_codec(codec)

  {}
  void GetStreamInfo(string& strInfo)
  {
    CStdString info;
    info.Format("%s", m_codec.c_str());
    strInfo = info;
  }
};

CDVDDemuxHTSP::CDVDDemuxHTSP()
  : CDVDDemux()
  , m_Input(NULL)
  , m_StatusCount(0)
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

  m_Input       = (CDVDInputStreamHTSP*)input;
  m_StatusCount = 0;

  while(m_Streams.size() == 0 && m_StatusCount == 0)
  {
    DemuxPacket* pkg = Read();
    if(!pkg)
      return false;
    CDVDDemuxUtils::FreeDemuxPacket(pkg);
  }

  return true;
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

bool CDVDDemuxHTSP::ReadStream(uint8_t* buf, int len)
{
  int ret;
  while(len > 0)
  {
    ret = m_Input->Read(buf, len);
    if(ret <= 0)
      return false;
    len -= ret;
    buf += ret;
  }
  return true;
}

htsmsg_t* CDVDDemuxHTSP::ReadStream()
{
  if(m_Input->IsStreamType(DVDSTREAM_TYPE_HTSP))
    return ((CDVDInputStreamHTSP*)m_Input)->ReadStream();

  uint32_t l;
  if(!ReadStream((uint8_t*)&l, 4))
    return NULL;

  l = ntohl(l);
  if(l == 0)
    return htsmsg_create_map();

  uint8_t* buf = (uint8_t*)malloc(l);
  if(!buf)
    return NULL;

  if(!ReadStream(buf, l))
    return NULL;

  return htsmsg_binary_deserialize(buf, l, buf); /* consumes 'buf' */
}

DemuxPacket* CDVDDemuxHTSP::Read()
{
  htsmsg_t *  msg;
  const char* method;
  while((msg = ReadStream()))
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
    else if(strcmp("queueStatus"       , method) == 0)
      CHTSPSession::ParseQueueStatus(msg, m_QueueStatus);
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
  htsmsg_t       *info;
  htsmsg_field_t *f;

  if((info = htsmsg_get_map(m, "sourceinfo")))
  {
    HTSMSG_FOREACH(f, info)
    {
      if(f->hmf_type != HMF_STR)
        continue;
      CLog::Log(LOGDEBUG, "CDVDDemuxHTSP::SubscriptionStart - %s: %s", f->hmf_name, htsmsg_field_get_string(f));
    }
  }

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
    const char* lang;
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
      CDemuxStreamSubtitle* s;
      CDemuxStreamTeletext* t;
    } st;

    CLog::Log(LOGDEBUG, "CDVDDemuxHTSP::SubscriptionStart - id: %d, type: %s", index, type);

    if(!strcmp(type, "AC3")) {
      st.a = new CDemuxStreamAudioHTSP(this, type);
      st.a->codec = CODEC_ID_AC3;
    } else if(!strcmp(type, "EAC3")) {
      st.a = new CDemuxStreamAudioHTSP(this, type);
      st.a->codec = CODEC_ID_EAC3;
    } else if(!strcmp(type, "MPEG2AUDIO")) {
      st.a = new CDemuxStreamAudioHTSP(this, type);
      st.a->codec = CODEC_ID_MP2;
    } else if(!strcmp(type, "AAC")) {
      st.a = new CDemuxStreamAudioHTSP(this, type);
      st.a->codec = CODEC_ID_AAC;
    } else if(!strcmp(type, "MPEG2VIDEO")) {
      st.v = new CDemuxStreamVideoHTSP(this, type);
      st.v->codec = CODEC_ID_MPEG2VIDEO;
      st.v->iWidth  = htsmsg_get_u32_or_default(sub, "width" , 0);
      st.v->iHeight = htsmsg_get_u32_or_default(sub, "height", 0);
    } else if(!strcmp(type, "H264")) {
      st.v = new CDemuxStreamVideoHTSP(this, type);
      st.v->codec = CODEC_ID_H264;
      st.v->iWidth  = htsmsg_get_u32_or_default(sub, "width" , 0);
      st.v->iHeight = htsmsg_get_u32_or_default(sub, "height", 0);
    } else if(!strcmp(type, "DVBSUB")) {
      st.s = new CDemuxStreamSubtitle();
      st.s->codec = CODEC_ID_DVB_SUBTITLE;
      uint32_t composition_id = 0, ancillary_id = 0;
      htsmsg_get_u32(sub, "composition_id", &composition_id);
      htsmsg_get_u32(sub, "ancillary_id"  , &ancillary_id);
      st.s->identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
    } else if(!strcmp(type, "TEXTSUB")) {
      st.s = new CDemuxStreamSubtitle();
      st.s->codec = CODEC_ID_TEXT;
    } else if(!strcmp(type, "TELETEXT")) {
      st.t = new CDemuxStreamTeletext();
      st.t->codec = CODEC_ID_DVB_TELETEXT;
    } else {
      continue;
    }

    if((lang = htsmsg_get_str(sub, "language")))
    {
      strncpy(st.g->language, lang, sizeof(st.g->language));
      st.g->language[sizeof(st.g->language) - 1] = '\0';
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
    m_StatusCount++;
    m_Status = status;
    CLog::Log(LOGDEBUG, "CDVDDemuxHTSP::SubscriptionStatus - %s", status);
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "TVHeadend Status", status, TOAST_DISPLAY_TIME, false);
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

void CDVDDemuxHTSP::Abort()
{
  if(m_Input)
    return m_Input->Abort();
}
