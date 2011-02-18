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

#include <stdint.h>
#include <limits.h>
#include <libavcodec/avcodec.h> // For codec id's
#include "HTSPDemux.h"

extern "C" {
#include "libhts/net.h"
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

cHTSPDemux::cHTSPDemux()
  : m_subs(0)
  , m_channel(0)
  , m_tag(0)
  , m_StatusCount(0)
  , m_SkipIFrame(0)
{
  m_Streams.nstreams = 0;
}

cHTSPDemux::~cHTSPDemux()
{
  Close();
}

bool cHTSPDemux::Open(const PVR_CHANNEL &channelinfo)
{
  m_channel = channelinfo.uid;
  m_tag     = channelinfo.bouquet;

  if(!m_session.Connect(g_szHostname, g_iPortHTSP))
    return false;

  if(!g_szUsername.IsEmpty())
    m_session.Auth(g_szUsername, g_szPassword);

  m_session.SendEnableAsync();

  if(!m_session.SendSubscribe(m_subs, m_channel))
    return false;

  m_StatusCount = 0;
  m_SkipIFrame = 0;

  while(m_Streams.nstreams == 0 && m_StatusCount == 0 )
  {
    DemuxPacket* pkg = Read();
    if(!pkg)
    {
      Close();
      return false;
    }
    PVR->FreeDemuxPacket(pkg);
  }

  return true;
}

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
    props->stream[i].height       = m_Streams.stream[i].height;
    props->stream[i].width        = m_Streams.stream[i].width;
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
  m_Streams.nstreams = 0;
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
    {
      SubscriptionStart(msg);
      m_SkipIFrame      = 0;
      DemuxPacket* pkt  = PVR->AllocateDemuxPacket(0);
      pkt->iStreamId    = DMX_SPECIALID_STREAMCHANGE;
      htsmsg_destroy(msg);
      return pkt;
    }
    else if(strcmp("subscriptionStop",   method) == 0)
      SubscriptionStop (msg);
    else if(strcmp("subscriptionStatus", method) == 0)
      SubscriptionStatus(msg);
    else if(strcmp("queueStatus"       , method) == 0)
      cHTSPSession::ParseQueueStatus(msg, m_QueueStatus);
    else if(strcmp("signalStatus"       , method) == 0)
      cHTSPSession::ParseSignalStatus(msg, m_Quality);
    else if(strcmp("muxpkt"            , method) == 0)
    {
      uint32_t    index, duration, frametype;
      const void* bin;
      size_t      binlen;
      int64_t     ts;
      char        frametypechar[1];

      htsmsg_get_u32(msg, "frametype", &frametype);
      frametypechar[0] = static_cast<char>( frametype );
//      XBMC->Log(LOG_DEBUG, "%s - Frame type %c", __FUNCTION__, frametypechar[0]);

      // Not the best solution - need to find how to pause video player for longer time.
      // This way video player could get enough audio packets in buffer to play without audio discontinuity

      // Jumpy video error on channel change is fixed if first I-frame is not send to demuxer on MPEG2 stream
      // Problem exists on some HD H264 stream, but it can help if 2 I-frames are skipped :D
      // SD H264 stream not tested
      if (g_bSkipIFrame && m_SkipIFrame == 0 && frametypechar[0]=='I')
      {
        m_SkipIFrame++;
        XBMC->Log(LOG_DEBUG, "%s - Skipped first I-frame", __FUNCTION__);
        htsmsg_destroy(msg);
        DemuxPacket* pkt  = PVR->AllocateDemuxPacket(0);
        return pkt;
      }

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
    DemuxPacket* pkt  = PVR->AllocateDemuxPacket(0);
    return pkt;
  }
  return NULL;
}

bool cHTSPDemux::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "%s - changing to channel %d", __FUNCTION__, channelinfo.number);

  if (!m_session.SendUnsubscribe(m_subs))
    XBMC->Log(LOG_ERROR, "%s - failed to unsubscribe from previous channel", __FUNCTION__);

  if (!m_session.SendSubscribe(m_subs+1, channelinfo.number))
    XBMC->Log(LOG_ERROR, "%s - failed to set channel", __FUNCTION__);
  else
  {
    m_channel           = channelinfo.number;
    m_subs              = m_subs+1;
    m_Streams.nstreams  = 0;
    m_StatusCount       = 0;
    while (m_Streams.nstreams == 0 && m_StatusCount == 0)
    {
      DemuxPacket* pkg = Read();
      if (!pkg)
      {
        return false;
      }
      PVR->FreeDemuxPacket(pkg);
    }
    return true;
  }
  return false;
}

bool cHTSPDemux::GetSignalStatus(PVR_SIGNALQUALITY &qualityinfo)
{
  if (m_SourceInfo.si_adapter.IsEmpty() || m_Quality.fe_status.IsEmpty())
    return false;

  strncpy(qualityinfo.frontend_name, m_SourceInfo.si_adapter.c_str(), sizeof(qualityinfo.frontend_name));
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
    XBMC->Log(LOG_ERROR, "%s - malformed message", __FUNCTION__);
    return;
  }

  m_Streams.nstreams = 0;

  HTSMSG_FOREACH(f, streams)
  {
    uint32_t    index;
    const char* type;
    htsmsg_t* sub;

    if (f->hmf_type != HMF_MAP)
      continue;

    sub = &f->hmf_msg;

    if ((type = htsmsg_get_str(sub, "type")) == NULL)
      continue;

    if (htsmsg_get_u32(sub, "index", &index))
      continue;

    const char *language = htsmsg_get_str(sub, "language");
    XBMC->Log(LOG_DEBUG, "%s - id: %d, type: %s, language: %s", __FUNCTION__, index, type, language);

    m_Streams.stream[m_Streams.nstreams].fpsscale         = 0;
    m_Streams.stream[m_Streams.nstreams].fpsrate          = 0;
    m_Streams.stream[m_Streams.nstreams].height           = 0;
    m_Streams.stream[m_Streams.nstreams].width            = 0;
    m_Streams.stream[m_Streams.nstreams].aspect           = 0.0;

    m_Streams.stream[m_Streams.nstreams].channels         = 0;
    m_Streams.stream[m_Streams.nstreams].samplerate       = 0;
    m_Streams.stream[m_Streams.nstreams].blockalign       = 0;
    m_Streams.stream[m_Streams.nstreams].bitrate          = 0;
    m_Streams.stream[m_Streams.nstreams].bits_per_sample  = 0;

    if(!strcmp(type, "AC3"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_AC3;
      if (language == NULL)
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= 0;
        m_Streams.stream[m_Streams.nstreams].language[1]= 0;
        m_Streams.stream[m_Streams.nstreams].language[2]= 0;
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      else
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
        m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
        m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "EAC3"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_EAC3;
      if (language == NULL)
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= 0;
        m_Streams.stream[m_Streams.nstreams].language[1]= 0;
        m_Streams.stream[m_Streams.nstreams].language[2]= 0;
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      else
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
        m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
        m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_MP2;
      if (language == NULL)
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= 0;
        m_Streams.stream[m_Streams.nstreams].language[1]= 0;
        m_Streams.stream[m_Streams.nstreams].language[2]= 0;
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      else
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
        m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
        m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "AAC"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_AAC;
      if (language == NULL)
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= 0;
        m_Streams.stream[m_Streams.nstreams].language[1]= 0;
        m_Streams.stream[m_Streams.nstreams].language[2]= 0;
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      else
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
        m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
        m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_VIDEO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_MPEG2VIDEO;
      m_Streams.stream[m_Streams.nstreams].width      = htsmsg_get_u32_or_default(sub, "width" , 0);
      m_Streams.stream[m_Streams.nstreams].height     = htsmsg_get_u32_or_default(sub, "height" , 0);
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
      m_Streams.stream[m_Streams.nstreams].width      = htsmsg_get_u32_or_default(sub, "width" , 0);
      m_Streams.stream[m_Streams.nstreams].height     = htsmsg_get_u32_or_default(sub, "height" , 0);
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
      if (language == NULL)
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= 0;
        m_Streams.stream[m_Streams.nstreams].language[1]= 0;
        m_Streams.stream[m_Streams.nstreams].language[2]= 0;
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      else
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
        m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
        m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      m_Streams.stream[m_Streams.nstreams].identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "TEXTSUB"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_TEXT;
      if (language == NULL)
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= 0;
        m_Streams.stream[m_Streams.nstreams].language[1]= 0;
        m_Streams.stream[m_Streams.nstreams].language[2]= 0;
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
      else
      {
        m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
        m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
        m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
        m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      }
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
      XBMC->Log(LOG_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }
  }

  if (cHTSPSession::ParseSourceInfo(m, m_SourceInfo))
  {
    XBMC->Log(LOG_DEBUG, "%s - subscription started on adapter %s, mux %s, network %s, provider %s, service %s"
        , __FUNCTION__, m_SourceInfo.si_adapter.c_str(), m_SourceInfo.si_mux.c_str(),
        m_SourceInfo.si_network.c_str(), m_SourceInfo.si_provider.c_str(),
        m_SourceInfo.si_service.c_str());
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "%s - subscription started on an unknown device", __FUNCTION__);
  }
}

void cHTSPDemux::SubscriptionStop  (htsmsg_t *m)
{
  XBMC->Log(LOG_DEBUG, "%s - subscription ended on adapter %s", __FUNCTION__, m_SourceInfo.si_adapter.c_str());
  m_Streams.nstreams = 0;

  /* reset the signal status */
  m_Quality.fe_status = "";
  m_Quality.fe_ber    = -2;
  m_Quality.fe_signal = -2;
  m_Quality.fe_snr    = -2;
  m_Quality.fe_unc    = -2;

  /* reset the source info */
  m_SourceInfo.si_adapter = "";
  m_SourceInfo.si_mux = "";
  m_SourceInfo.si_network = "";
  m_SourceInfo.si_provider = "";
  m_SourceInfo.si_service = "";
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
    XBMC->Log(LOG_DEBUG, "%s - %s", __FUNCTION__, status);
    XBMC->QueueNotification(QUEUE_INFO, status);
  }
}

htsmsg_t* cHTSPDemux::ReadStream()
{
  htsmsg_t* msg;

  /* after anything has started reading, *
   * we can guarantee a new stream       */

  while((msg = m_session.ReadMessage(1000)))
  {
    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
      return msg;

    if     (strstr(method, "channelAdd"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelDelete"))
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
