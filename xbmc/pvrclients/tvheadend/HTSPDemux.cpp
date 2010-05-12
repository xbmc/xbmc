/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "HTSPDemux.h"
#include <limits.h>
#include <libavcodec/avcodec.h> // For codec id's

extern "C" {
#include "libhts/net.h"
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

cHTSPDemux::cHTSPDemux()
  : m_subs(0)
  , m_startup(false)
  , m_channel(0)
  , m_tag(0)
  , m_StatusCount(0)
{
  m_Streams.nstreams = 0;
}

cHTSPDemux::~cHTSPDemux()
{
  Close();
}

bool cHTSPDemux::Open(const PVR_CHANNEL &channelinfo)
{
  m_channel = channelinfo.number;
  m_tag     = channelinfo.bouquet;

  if(!m_session.Connect(g_szHostname, g_iPortHTSP))
    return false;

  if(!g_szUsername.IsEmpty())
    m_session.Auth(g_szUsername, g_szPassword);

  m_session.SendEnableAsync();

  if(!m_session.SendSubscribe(m_subs, m_channel))
    return false;

  m_StatusCount = 0;

  while(m_Streams.nstreams == 0 && m_StatusCount == 0)
  {
    DemuxPacket* pkg = Read();
    if(!pkg)
      return false;
    PVR->FreeDemuxPacket(pkg);
  }

  m_startup = true;
  return true;
}
int videoid = 0;
void cHTSPDemux::Close()
{
  m_session.Close();
}

bool cHTSPDemux::GetStreamProperties(PVR_STREAMPROPS* props)
{
  props->nstreams = m_Streams.nstreams;
  for (int i = 0; i < m_Streams.nstreams; i++)
  {
    props->stream[i].id           = m_Streams.stream[i].id;
    props->stream[i].physid       = m_Streams.stream[i].physid;
    props->stream[i].codec_type   = m_Streams.stream[i].codec_type;
    props->stream[i].codec_id     = m_Streams.stream[i].codec_id;
    props->stream[i].language[0]  = m_Streams.stream[i].language[0];
    props->stream[i].language[1]  = m_Streams.stream[i].language[1];
    props->stream[i].language[2]  = m_Streams.stream[i].language[2];
    props->stream[i].language[3]  = m_Streams.stream[i].language[3];
    props->stream[i].identifier   = m_Streams.stream[i].identifier;
  }
  return (props->nstreams > 0);
}

void cHTSPDemux::Abort()
{
  m_session.Abort();
}

DemuxPacket* cHTSPDemux::Read()
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
      cHTSPSession::ParseQueueStatus(msg, m_QueueStatus, m_Quality);
    else if(strcmp("muxpkt"            , method) == 0)
    {
      uint32_t    index, duration;
      const void* bin;
      size_t      binlen;
      int64_t     ts;

      if(htsmsg_get_u32(msg, "stream" , &index)  ||
         htsmsg_get_bin(msg, "payload", &bin, &binlen))
        break;

      DemuxPacket* pkt = PVR->AllocateDemuxPacket(binlen);
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
      for(int i = 0; i < m_Streams.nstreams; i++)
      {
        if(m_Streams.stream[i].physid == (int)index)
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
    return PVR->AllocateDemuxPacket(0);
  }
  return NULL;
}

bool cHTSPDemux::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "cHTSPDemux::SetChannel - changing to channel %d", channelinfo.number);

  if(!m_session.SendUnsubscribe(m_subs))
    XBMC->Log(LOG_ERROR, "cHTSPDemux::SetChannel - failed to unsubscribe from previous channel");

  if(!m_session.SendSubscribe(m_subs+1, channelinfo.number))
  {
    if(m_session.SendSubscribe(m_subs, m_channel))
      XBMC->Log(LOG_ERROR, "cHTSPDemux::SetChannel - failed to set channel");
    else
      XBMC->Log(LOG_ERROR, "cHTSPDemux::SetChannel - failed to set channel and restore old channel");

    return false;
  }

  m_channel = channelinfo.number;
  m_subs    = m_subs+1;
  m_startup = true;
  return true;
}

bool cHTSPDemux::GetSignalStatus(PVR_SIGNALQUALITY &qualityinfo)
{
  if (m_Quality.fe_name.IsEmpty())
    return false;

  strncpy(qualityinfo.frontend_name, m_Quality.fe_name.c_str(), sizeof(qualityinfo.frontend_name));
  strncpy(qualityinfo.frontend_status, m_Quality.fe_status.c_str(), sizeof(qualityinfo.frontend_status));
  qualityinfo.signal = (uint16_t)m_Quality.fe_signal;
  qualityinfo.snr = (uint16_t)m_Quality.fe_snr;
  qualityinfo.ber = (uint32_t)m_Quality.fe_ber;
  qualityinfo.unc = (uint32_t)m_Quality.fe_unc;
  qualityinfo.video_bitrate = 0;
  qualityinfo.audio_bitrate = 0;
  qualityinfo.dolby_bitrate = 0;

  return true;
}

void cHTSPDemux::SubscriptionStart (htsmsg_t *m)
{
  htsmsg_t       *streams;
  htsmsg_field_t *f;

  if((streams = htsmsg_get_list(m, "streams")) == NULL)
  {
    XBMC->Log(LOG_ERROR, "cHTSPDemux::SubscriptionStart - malformed message");
    return;
  }

  m_Streams.nstreams = 0;

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

    XBMC->Log(LOG_DEBUG, "cHTSPDemux::SubscriptionStart - id: %d, type: %s", index, type);

    if(!strcmp(type, "AC3"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_AC3;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_MP2;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "AAC"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_AAC;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      videoid = index;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_VIDEO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_MPEG2VIDEO;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "H264"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_VIDEO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_H264;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "DVBSUB"))
    {
      uint32_t composition_id = 0, ancillary_id = 0;
      htsmsg_get_u32(sub, "composition_id", &composition_id);
      htsmsg_get_u32(sub, "ancillary_id"  , &ancillary_id);

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_DVB_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "TEXTSUB"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_TEXT;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "TELETEXT"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_DVB_TELETEXT;
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }

    if (m_Streams.nstreams >= PVR_STREAM_MAX_STREAMS)
    {
      XBMC->Log(LOG_ERROR, "cHTSPDemux::SubscriptionStart - max amount of streams reached");
      break;
    }
  }
}

void cHTSPDemux::SubscriptionStop  (htsmsg_t *m)
{
  m_Streams.nstreams = 0;
}

void cHTSPDemux::SubscriptionStatus(htsmsg_t *m)
{
  const char* status;
  status = htsmsg_get_str(m, "status");
  if(status == NULL)
    m_Status = "";
  else
  {
    m_StatusCount++;
    m_Status = status;
    XBMC->Log(LOG_DEBUG, "cHTSPDemux::SubscriptionStatus - %s", status);
    XBMC->QueueNotification(QUEUE_INFO, status);
  }
}

htsmsg_t* cHTSPDemux::ReadStream()
{
  htsmsg_t* msg;

  /* after anything has started reading, *
   * we can guarantee a new stream       */
  m_startup = false;

  while((msg = m_session.ReadMessage(1000)))
  {
    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
      return msg;

    if     (strstr(method, "channelAdd"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelRemove"))
      cHTSPSession::ParseChannelRemove(msg, m_channels);

    uint32_t subs;
    if(htsmsg_get_u32(msg, "subscriptionId", &subs) || subs != m_subs)
    {
      htsmsg_destroy(msg);
      continue;
    }

    return msg;
  }
  return NULL;
}
