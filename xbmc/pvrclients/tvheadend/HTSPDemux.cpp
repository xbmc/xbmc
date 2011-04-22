/*
 *      Copyright (C) 2005-2011 Team XBMC
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
{
  m_Streams.iStreamCount = 0;
}

cHTSPDemux::~cHTSPDemux()
{
  Close();
}

bool cHTSPDemux::Open(const PVR_CHANNEL &channelinfo)
{
  m_channel = channelinfo.iUniqueId;

  if(!m_session.Connect(g_strHostname, g_iPortHTSP, g_iConnectTimeout))
    return false;

  if(!g_strUsername.empty())
    m_session.Auth(g_strUsername, g_strPassword);

  if(!m_session.SendSubscribe(m_subs, m_channel))
    return false;

  m_StatusCount = 0;

  while(m_Streams.iStreamCount == 0 && m_StatusCount == 0 )
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

bool cHTSPDemux::GetStreamProperties(PVR_STREAM_PROPERTIES* props)
{
  props->iStreamCount = m_Streams.iStreamCount;
  for (unsigned int i = 0; i < m_Streams.iStreamCount; i++)
  {
    props->stream[i].iStreamIndex   = m_Streams.stream[i].iStreamIndex;
    props->stream[i].iPhysicalId    = m_Streams.stream[i].iPhysicalId;
    props->stream[i].iCodecType     = m_Streams.stream[i].iCodecType;
    props->stream[i].iCodecId       = m_Streams.stream[i].iCodecId;
    props->stream[i].iHeight        = m_Streams.stream[i].iHeight;
    props->stream[i].iWidth         = m_Streams.stream[i].iWidth;
    props->stream[i].strLanguage[0] = m_Streams.stream[i].strLanguage[0];
    props->stream[i].strLanguage[1] = m_Streams.stream[i].strLanguage[1];
    props->stream[i].strLanguage[2] = m_Streams.stream[i].strLanguage[2];
    props->stream[i].strLanguage[3] = m_Streams.stream[i].strLanguage[3];
    props->stream[i].iIdentifier    = m_Streams.stream[i].iIdentifier;
  }
  return (props->iStreamCount > 0);
}

void cHTSPDemux::Abort()
{
  m_Streams.iStreamCount = 0;
  m_session.Abort();
}

DemuxPacket* cHTSPDemux::Read()
{
  htsmsg_t *  msg;
  const char* method;
  while((msg = m_session.ReadMessage(1000)))
  {
    method = htsmsg_get_str(msg, "method");
    if(method == NULL)
      break;

    uint32_t subs;
    if(htsmsg_get_u32(msg, "subscriptionId", &subs) || subs != m_subs)
    {
      htsmsg_destroy(msg);
      continue;
    }

    if     (strcmp("subscriptionStart",  method) == 0)
    {
      SubscriptionStart(msg);
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
      DemuxPacket *pkt = ParseMuxPacket(msg);
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

DemuxPacket *cHTSPDemux::ParseMuxPacket(htsmsg_t *msg)
{
  DemuxPacket* pkt = NULL;
  uint32_t    index, duration, frametype;
  const void* bin;
  size_t      binlen;
  int64_t     ts;
  char        frametypechar[1];

  htsmsg_get_u32(msg, "frametype", &frametype);
  frametypechar[0] = static_cast<char>( frametype );

  if(htsmsg_get_u32(msg, "stream" , &index)  ||
     htsmsg_get_bin(msg, "payload", &bin, &binlen))
  {
    return pkt;
  }

  pkt = PVR->AllocateDemuxPacket(binlen);
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
  for(unsigned int i = 0; i < m_Streams.iStreamCount; i++)
  {
    if(m_Streams.stream[i].iPhysicalId == (unsigned int)index)
    {
      pkt->iStreamId = i;
      break;
    }
  }

  return pkt;
}

bool cHTSPDemux::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "%s - changing to channel '%s'", __FUNCTION__, channelinfo.strChannelName);

  if (!m_session.SendUnsubscribe(m_subs))
    XBMC->Log(LOG_ERROR, "%s - failed to unsubscribe from previous channel", __FUNCTION__);

  if (!m_session.SendSubscribe(m_subs+1, channelinfo.iUniqueId))
    XBMC->Log(LOG_ERROR, "%s - failed to set channel", __FUNCTION__);
  else
  {
    m_channel           = channelinfo.iChannelNumber;
    m_subs              = m_subs+1;
    m_Streams.iStreamCount  = 0;
    m_StatusCount       = 0;
    while (m_Streams.iStreamCount == 0 && m_StatusCount == 0)
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

bool cHTSPDemux::GetSignalStatus(PVR_SIGNAL_STATUS &qualityinfo)
{
  if (m_SourceInfo.si_adapter.empty() || m_Quality.fe_status.empty())
    return false;

  strncpy(qualityinfo.strAdapterName, m_SourceInfo.si_adapter.c_str(), sizeof(qualityinfo.strAdapterName));
  strncpy(qualityinfo.strAdapterStatus, m_Quality.fe_status.c_str(), sizeof(qualityinfo.strAdapterStatus));
  qualityinfo.iSignal       = (uint16_t)m_Quality.fe_signal;
  qualityinfo.iSNR          = (uint16_t)m_Quality.fe_snr;
  qualityinfo.iBER          = (uint32_t)m_Quality.fe_ber;
  qualityinfo.iUNC          = (uint32_t)m_Quality.fe_unc;
  qualityinfo.dVideoBitrate = 0;
  qualityinfo.dAudioBitrate = 0;
  qualityinfo.dDolbyBitrate = 0;

  return true;
}

void cHTSPDemux::SetLanguageInfo(const char *strLanguage, char *strDestination)
{
  if (strLanguage == NULL)
  {
    strDestination[0] = 0;
    strDestination[1] = 0;
    strDestination[2] = 0;
    strDestination[3] = 0;
  }
  else
  {
    strDestination[0] = strLanguage[0];
    strDestination[1] = strLanguage[1];
    strDestination[2] = strLanguage[2];
    strDestination[3] = 0;
  }
}

void cHTSPDemux::SubscriptionStart(htsmsg_t *m)
{
  htsmsg_t       *streams;
  htsmsg_field_t *f;
  if((streams = htsmsg_get_list(m, "streams")) == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message", __FUNCTION__);
    return;
  }

  m_Streams.iStreamCount = 0;

  HTSMSG_FOREACH(f, streams)
  {
    uint32_t    index;
    const char* type;
    htsmsg_t*   sub;

    if (f->hmf_type != HMF_MAP)
      continue;

    sub = &f->hmf_msg;

    if ((type = htsmsg_get_str(sub, "type")) == NULL)
      continue;

    if (htsmsg_get_u32(sub, "index", &index))
      continue;

    const char *language = htsmsg_get_str(sub, "language");
    XBMC->Log(LOG_DEBUG, "%s - id: %d, type: %s, language: %s", __FUNCTION__, index, type, language);

    m_Streams.stream[m_Streams.iStreamCount].iFPSScale          = 0;
    m_Streams.stream[m_Streams.iStreamCount].iFPSRate           = 0;
    m_Streams.stream[m_Streams.iStreamCount].iHeight            = 0;
    m_Streams.stream[m_Streams.iStreamCount].iWidth             = 0;
    m_Streams.stream[m_Streams.iStreamCount].fAspect            = 0.0;

    m_Streams.stream[m_Streams.iStreamCount].iChannels          = 0;
    m_Streams.stream[m_Streams.iStreamCount].iSampleRate        = 0;
    m_Streams.stream[m_Streams.iStreamCount].iBlockAlign        = 0;
    m_Streams.stream[m_Streams.iStreamCount].iBitRate           = 0;
    m_Streams.stream[m_Streams.iStreamCount].iBitsPerSample     = 0;

    m_Streams.stream[m_Streams.iStreamCount].strLanguage[0] = 0;
    m_Streams.stream[m_Streams.iStreamCount].strLanguage[1] = 0;
    m_Streams.stream[m_Streams.iStreamCount].strLanguage[2] = 0;
    m_Streams.stream[m_Streams.iStreamCount].strLanguage[3] = 0;

    m_Streams.stream[m_Streams.iStreamCount].iIdentifier      = -1;

    if(!strcmp(type, "AC3"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_AC3;
      m_Streams.stream[m_Streams.iStreamCount].iChannels        = htsmsg_get_u32_or_default(sub, "channels" , 0);
      m_Streams.stream[m_Streams.iStreamCount].iSampleRate      = htsmsg_get_u32_or_default(sub, "rate" , 0);
      SetLanguageInfo(language, m_Streams.stream[m_Streams.iStreamCount].strLanguage);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "EAC3"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_EAC3;
      m_Streams.stream[m_Streams.iStreamCount].iChannels        = htsmsg_get_u32_or_default(sub, "channels" , 0);
      m_Streams.stream[m_Streams.iStreamCount].iSampleRate      = htsmsg_get_u32_or_default(sub, "rate" , 0);
      XBMC->Log(LOG_DEBUG, "channels = %d, rate = %d", m_Streams.stream[m_Streams.iStreamCount].iChannels, m_Streams.stream[m_Streams.iStreamCount].iSampleRate);
      SetLanguageInfo(language, m_Streams.stream[m_Streams.iStreamCount].strLanguage);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_MP2;
      m_Streams.stream[m_Streams.iStreamCount].iChannels        = htsmsg_get_u32_or_default(sub, "channels" , 0);
      m_Streams.stream[m_Streams.iStreamCount].iSampleRate      = htsmsg_get_u32_or_default(sub, "rate" , 0);
      XBMC->Log(LOG_DEBUG, "channels = %d, rate = %d", m_Streams.stream[m_Streams.iStreamCount].iChannels, m_Streams.stream[m_Streams.iStreamCount].iSampleRate);
      SetLanguageInfo(language, m_Streams.stream[m_Streams.iStreamCount].strLanguage);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "AAC"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_AAC;
      m_Streams.stream[m_Streams.iStreamCount].iChannels        = htsmsg_get_u32_or_default(sub, "channels" , 0);
      m_Streams.stream[m_Streams.iStreamCount].iSampleRate      = htsmsg_get_u32_or_default(sub, "rate" , 0);
      XBMC->Log(LOG_DEBUG, "channels = %d, rate = %d", m_Streams.stream[m_Streams.iStreamCount].iChannels, m_Streams.stream[m_Streams.iStreamCount].iSampleRate);
      SetLanguageInfo(language, m_Streams.stream[m_Streams.iStreamCount].strLanguage);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_VIDEO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_MPEG2VIDEO;
      m_Streams.stream[m_Streams.iStreamCount].iWidth           = htsmsg_get_u32_or_default(sub, "width" , 0);
      m_Streams.stream[m_Streams.iStreamCount].iHeight          = htsmsg_get_u32_or_default(sub, "height" , 0);
      m_Streams.stream[m_Streams.iStreamCount].fAspect          = (float) (htsmsg_get_u32_or_default(sub, "aspect_num", 1) / htsmsg_get_u32_or_default(sub, "aspect_den", 1));
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "H264"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_VIDEO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_H264;
      m_Streams.stream[m_Streams.iStreamCount].iWidth           = htsmsg_get_u32_or_default(sub, "width" , 0);
      m_Streams.stream[m_Streams.iStreamCount].iHeight          = htsmsg_get_u32_or_default(sub, "height" , 0);
      m_Streams.stream[m_Streams.iStreamCount].fAspect          = (float) (htsmsg_get_u32_or_default(sub, "aspect_num", 1) / htsmsg_get_u32_or_default(sub, "aspect_den", 1));
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "DVBSUB"))
    {
      uint32_t composition_id = 0, ancillary_id = 0;
      htsmsg_get_u32(sub, "composition_id", &composition_id);
      htsmsg_get_u32(sub, "ancillary_id"  , &ancillary_id);

      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_DVB_SUBTITLE;
      SetLanguageInfo(language, m_Streams.stream[m_Streams.iStreamCount].strLanguage);
      m_Streams.stream[m_Streams.iStreamCount].iIdentifier      = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "TEXTSUB"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_TEXT;
      SetLanguageInfo(language, m_Streams.stream[m_Streams.iStreamCount].strLanguage);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "TELETEXT"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex     = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId      = index;
      m_Streams.stream[m_Streams.iStreamCount].iCodecType       = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId         = CODEC_ID_DVB_TELETEXT;
      m_Streams.iStreamCount++;
    }

    if (m_Streams.iStreamCount >= PVR_STREAM_MAX_STREAMS)
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
  m_Streams.iStreamCount = 0;

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
