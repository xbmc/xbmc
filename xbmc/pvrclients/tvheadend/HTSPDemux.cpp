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
#include "HTSPDemux.h"
#include <libavcodec/avcodec.h> // For codec id's

extern "C" {
#include "libhts/net.h"
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

using namespace ADDON;

CHTSPDemux::CHTSPDemux() :
    m_bGotFirstIframe(false),
    m_bIsRadio(false),
    m_bAbort(false),
    m_subs(0),
    m_channel(0),
    m_tag(0),
    m_StatusCount(0)
{
  m_session = new CHTSPConnection();
  m_Streams.iStreamCount = 0;
}

CHTSPDemux::~CHTSPDemux()
{
  Close();
  delete m_session;
}

bool CHTSPDemux::Open(const PVR_CHANNEL &channelinfo)
{
  m_channel = channelinfo.iUniqueId;
  m_bIsRadio = channelinfo.bIsRadio;

  if (!m_session)
    m_session = new CHTSPConnection();

  if(!m_session->Connect())
    return false;

  if(!SendSubscribe(m_subs, m_channel))
    return false;

  m_Streams.iStreamCount  = 0;
  m_StatusCount = 0;
  return true;
}

void CHTSPDemux::Close()
{
  m_bAbort = true;
  m_session->Close();
}

bool CHTSPDemux::GetStreamProperties(PVR_STREAM_PROPERTIES* props)
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

void CHTSPDemux::Abort()
{
  m_Streams.iStreamCount = 0;
  m_bAbort = true;
  m_session->Abort();
}

DemuxPacket* CHTSPDemux::Read()
{
  htsmsg_t *  msg;
  const char* method;
  while((msg = m_session->ReadMessage(1000)) && !m_bAbort)
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
      ParseSubscriptionStart(msg);
      DemuxPacket* pkt  = PVR->AllocateDemuxPacket(0);
      pkt->iStreamId    = DMX_SPECIALID_STREAMCHANGE;
      htsmsg_destroy(msg);
      return pkt;
    }
    else if(strcmp("subscriptionStop",   method) == 0)
      ParseSubscriptionStop (msg);
    else if(strcmp("subscriptionStatus", method) == 0)
      ParseSubscriptionStatus(msg);
    else if(strcmp("queueStatus"       , method) == 0)
      ParseQueueStatus(msg);
    else if(strcmp("signalStatus"       , method) == 0)
      ParseSignalStatus(msg);
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

DemuxPacket *CHTSPDemux::ParseMuxPacket(htsmsg_t *msg)
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
     htsmsg_get_bin(msg, "payload", &bin, &binlen) ||
     (!m_bGotFirstIframe && frametypechar[0] != 'I'))
  {
    return pkt;
  }

  m_bGotFirstIframe = true;
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

bool CHTSPDemux::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "%s - changing to channel '%s'", __FUNCTION__, channelinfo.strChannelName);

  if (!SendUnsubscribe(m_subs))
    XBMC->Log(LOG_ERROR, "%s - failed to unsubscribe from previous channel", __FUNCTION__);

  if (!SendSubscribe(m_subs+1, channelinfo.iUniqueId))
    XBMC->Log(LOG_ERROR, "%s - failed to set channel", __FUNCTION__);
  else
  {
    m_channel           = channelinfo.iChannelNumber;
    m_subs              = m_subs+1;
    m_Streams.iStreamCount = 0;
    m_StatusCount = 0;

    return true;
  }
  return false;
}

bool CHTSPDemux::GetSignalStatus(PVR_SIGNAL_STATUS &qualityinfo)
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

inline void HTSPResetDemuxStreamInfo(PVR_STREAM_PROPERTIES::PVR_STREAM &stream)
{
  stream.iFPSScale      = 0;
  stream.iFPSRate       = 0;
  stream.iHeight        = 0;
  stream.iWidth         = 0;
  stream.fAspect        = 0.0;
  stream.iChannels      = 0;
  stream.iSampleRate    = 0;
  stream.iBlockAlign    = 0;
  stream.iBitRate       = 0;
  stream.iBitsPerSample = 0;
  stream.strLanguage[0] = 0;
  stream.strLanguage[1] = 0;
  stream.strLanguage[2] = 0;
  stream.strLanguage[3] = 0;
  stream.iIdentifier    = -1;
}

inline void HTSPSetDemuxStreamInfoAudio(PVR_STREAM_PROPERTIES::PVR_STREAM &stream, htsmsg_t *msg)
{
  stream.iChannels   = htsmsg_get_u32_or_default(msg, "channels" , 0);
  stream.iSampleRate = htsmsg_get_u32_or_default(msg, "rate" , 0);
}

inline void HTSPSetDemuxStreamInfoVideo(PVR_STREAM_PROPERTIES::PVR_STREAM &stream, htsmsg_t *msg)
{
  stream.iWidth  = htsmsg_get_u32_or_default(msg, "width" , 0);
  stream.iHeight = htsmsg_get_u32_or_default(msg, "height" , 0);
  stream.fAspect = (float) (htsmsg_get_u32_or_default(msg, "aspect_num", 1) / htsmsg_get_u32_or_default(msg, "aspect_den", 1));
}

inline void HTSPSetDemuxStreamInfoLanguage(PVR_STREAM_PROPERTIES::PVR_STREAM &stream, htsmsg_t *msg)
{
  if (const char *strLanguage = htsmsg_get_str(msg, "language"))
  {
    stream.strLanguage[0] = strLanguage[0];
    stream.strLanguage[1] = strLanguage[1];
    stream.strLanguage[2] = strLanguage[2];
    stream.strLanguage[3] = 0;
  }
}

void CHTSPDemux::ParseSubscriptionStart(htsmsg_t *m)
{
  htsmsg_t       *streams;
  htsmsg_field_t *f;
  if((streams = htsmsg_get_list(m, "streams")) == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message", __FUNCTION__);
    return;
  }

  m_Streams.iStreamCount = 0;
  m_bGotFirstIframe = m_bIsRadio; // only wait for the first I frame when playing back a tv stream

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

    XBMC->Log(LOG_DEBUG, "%s - id: %d, type: %s", __FUNCTION__, index, type);

    bool bValidStream(true);
    HTSPResetDemuxStreamInfo(m_Streams.stream[m_Streams.iStreamCount]);

    if(!strcmp(type, "AC3"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_AC3;
    }
    else if(!strcmp(type, "EAC3"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_EAC3;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_MP2;
    }
    else if(!strcmp(type, "AAC"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_AUDIO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_AAC;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_VIDEO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_MPEG2VIDEO;
    }
    else if(!strcmp(type, "H264"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_VIDEO;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_H264;
    }
    else if(!strcmp(type, "DVBSUB"))
    {
      uint32_t composition_id = 0, ancillary_id = 0;
      htsmsg_get_u32(sub, "composition_id", &composition_id);
      htsmsg_get_u32(sub, "ancillary_id"  , &ancillary_id);

      m_Streams.stream[m_Streams.iStreamCount].iCodecType  = AVMEDIA_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId    = CODEC_ID_DVB_SUBTITLE;
      m_Streams.stream[m_Streams.iStreamCount].iIdentifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
      HTSPSetDemuxStreamInfoLanguage(m_Streams.stream[m_Streams.iStreamCount], sub);
    }
    else if(!strcmp(type, "TEXTSUB"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_TEXT;
      HTSPSetDemuxStreamInfoLanguage(m_Streams.stream[m_Streams.iStreamCount], sub);
    }
    else if(!strcmp(type, "TELETEXT"))
    {
      m_Streams.stream[m_Streams.iStreamCount].iCodecType = AVMEDIA_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.iStreamCount].iCodecId   = CODEC_ID_DVB_TELETEXT;
    }
    else
    {
      bValidStream = false;
    }

    if (bValidStream)
    {
      m_Streams.stream[m_Streams.iStreamCount].iStreamIndex = m_Streams.iStreamCount;
      m_Streams.stream[m_Streams.iStreamCount].iPhysicalId  = index;
      if (m_Streams.stream[m_Streams.iStreamCount].iCodecType == AVMEDIA_TYPE_AUDIO)
      {
        HTSPSetDemuxStreamInfoAudio(m_Streams.stream[m_Streams.iStreamCount], sub);
        HTSPSetDemuxStreamInfoLanguage(m_Streams.stream[m_Streams.iStreamCount], sub);
      }
      else if (m_Streams.stream[m_Streams.iStreamCount].iCodecType == AVMEDIA_TYPE_VIDEO)
        HTSPSetDemuxStreamInfoVideo(m_Streams.stream[m_Streams.iStreamCount], sub);
      ++m_Streams.iStreamCount;
    }

    if (m_Streams.iStreamCount >= PVR_STREAM_MAX_STREAMS)
    {
      XBMC->Log(LOG_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }
  }

  if (ParseSourceInfo(m))
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

void CHTSPDemux::ParseSubscriptionStop  (htsmsg_t *m)
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

void CHTSPDemux::ParseSubscriptionStatus(htsmsg_t *m)
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

bool CHTSPDemux::SendUnsubscribe(int subscription)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "unsubscribe");
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return m_session->ReadSuccess(m, true, "unsubscribe from channel");
}

bool CHTSPDemux::SendSubscribe(int subscription, int channel)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "subscribe");
  htsmsg_add_s32(m, "channelId"     , channel);
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return m_session->ReadSuccess(m, true, "subscribe to channel");
}

bool CHTSPDemux::ParseQueueStatus(htsmsg_t* msg)
{
  if(htsmsg_get_u32(msg, "packets", &m_QueueStatus.packets)
  || htsmsg_get_u32(msg, "bytes",   &m_QueueStatus.bytes)
  || htsmsg_get_u32(msg, "Bdrops",  &m_QueueStatus.bdrops)
  || htsmsg_get_u32(msg, "Pdrops",  &m_QueueStatus.pdrops)
  || htsmsg_get_u32(msg, "Idrops",  &m_QueueStatus.idrops))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return false;
  }

  /* delay isn't always transmitted */
  if(htsmsg_get_u32(msg, "delay", &m_QueueStatus.delay))
    m_QueueStatus.delay = 0;

  return true;
}

bool CHTSPDemux::ParseSignalStatus(htsmsg_t* msg)
{
  if(htsmsg_get_u32(msg, "feSNR", &m_Quality.fe_snr))
    m_Quality.fe_snr = -2;

  if(htsmsg_get_u32(msg, "feSignal", &m_Quality.fe_signal))
    m_Quality.fe_signal = -2;

  if(htsmsg_get_u32(msg, "feBER", &m_Quality.fe_ber))
    m_Quality.fe_ber = -2;

  if(htsmsg_get_u32(msg, "feUNC", &m_Quality.fe_unc))
    m_Quality.fe_unc = -2;

  const char* status;
  if((status = htsmsg_get_str(msg, "feStatus")))
    m_Quality.fe_status = status;
  else
    m_Quality.fe_status = "(unknown)";

//  XBMC->Log(LOG_DEBUG, "%s - updated signal status: snr=%d, signal=%d, ber=%d, unc=%d, status=%s"
//      , __FUNCTION__, quality.fe_snr, quality.fe_signal, quality.fe_ber
//      , quality.fe_unc, quality.fe_status.c_str());

  return true;
}

bool CHTSPDemux::ParseSourceInfo(htsmsg_t* msg)
{
  htsmsg_t       *sourceinfo;
  if((sourceinfo = htsmsg_get_map(msg, "sourceinfo")) == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message", __FUNCTION__);
    return false;
  }

  const char* str;
  if((str = htsmsg_get_str(sourceinfo, "adapter")) == NULL)
    m_SourceInfo.si_adapter = "";
  else
    m_SourceInfo.si_adapter = str;

  if((str = htsmsg_get_str(sourceinfo, "mux")) == NULL)
    m_SourceInfo.si_mux = "";
  else
    m_SourceInfo.si_mux = str;

  if((str = htsmsg_get_str(sourceinfo, "network")) == NULL)
    m_SourceInfo.si_network = "";
  else
    m_SourceInfo.si_network = str;

  if((str = htsmsg_get_str(sourceinfo, "provider")) == NULL)
    m_SourceInfo.si_provider = "";
  else
    m_SourceInfo.si_provider = str;

  if((str = htsmsg_get_str(sourceinfo, "service")) == NULL)
    m_SourceInfo.si_service = "";
  else
    m_SourceInfo.si_service = str;

  return true;
}
