/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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
#include "VNSIDemux.h"
#include "tools.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "vdrcommand.h"

cVNSIDemux::cVNSIDemux()
  : m_startup(false)
  , m_channel(0)
  , m_StatusCount(0)
{
  m_Streams.nstreams = 0;
}

cVNSIDemux::~cVNSIDemux()
{
  Close();
}

bool cVNSIDemux::Open(const PVR_CHANNEL &channelinfo)
{
  m_channel = channelinfo.number;

  if(!m_session.Open(g_szHostname, g_iPort, g_iConnectTimeout, "XBMC Live stream receiver"))
    return false;

  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELSTREAM_OPEN) ||
      !vrp.add_U32(m_channel) ||
      !m_session.ReadSuccess(&vrp))
  {
    XBMC->Log(LOG_ERROR, "cVNSIDemux::Open - Can't open channel %i - %s", m_channel, channelinfo.name);
    return false;
  }

  m_StatusCount = 0;
  m_startup = true;

  while (m_Streams.nstreams == 0 && m_StatusCount == 0)
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

void cVNSIDemux::Close()
{
  cRequestPacket vrp;
  m_session.Close();
}

bool cVNSIDemux::GetStreamProperties(PVR_STREAMPROPS* props)
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

void cVNSIDemux::Abort()
{
  m_Streams.nstreams = 0;
  m_session.Abort();
}

DemuxPacket* cVNSIDemux::Read()
{
  cResponsePacket *resp = m_session.ReadMessage(15);

  if(resp == NULL)
  {
    return NULL;
  }

  if (resp->getChannelID() != CHANNEL_STREAM)
  {
    delete resp;
    return NULL;
  }

  if (resp->getOpCodeID() == VDR_STREAM_CHANGE)
  {
    StreamChange(resp);
    if (!m_startup)
    {
      DemuxPacket* pkt  = PVR->AllocateDemuxPacket(0);
      pkt->iStreamId    = DMX_SPECIALID_STREAMCHANGE;
      delete resp;
      return pkt;
    }
    else
      m_startup = false;
  }
  else if (resp->getOpCodeID() == VDR_STREAM_STATUS)
  {
    StreamStatus(resp);
  }
  else if (resp->getOpCodeID() == VDR_STREAM_SIGNALINFO)
  {
    StreamSignalInfo(resp);
  }
  else if (resp->getOpCodeID() == VDR_STREAM_CONTENTINFO)
  {
    StreamContentInfo(resp);
    DemuxPacket* pkt = PVR->AllocateDemuxPacket(sizeof(PVR_STREAMPROPS));
    memcpy(pkt->pData, &m_Streams, sizeof(PVR_STREAMPROPS));
    pkt->iStreamId  = DMX_SPECIALID_STREAMINFO;
    pkt->iSize      = sizeof(PVR_STREAMPROPS);
    delete resp;
    return pkt;
  }
  else if (resp->getOpCodeID() == VDR_STREAM_MUXPKT)
  {
    DemuxPacket* p = (DemuxPacket*)resp->getUserData();

    p->iSize      = resp->getUserDataLength();
    p->duration   = (double)resp->getDuration() * DVD_TIME_BASE / 1000000;
    p->dts        = (double)resp->getDTS() * DVD_TIME_BASE / 1000000;
    p->pts        = (double)resp->getPTS() * DVD_TIME_BASE / 1000000;
    p->iStreamId  = -1;
    for(int i = 0; i < m_Streams.nstreams; i++)
    {
      if(m_Streams.stream[i].physid == (int)resp->getStreamID())
      {
            p->iStreamId = i;
            break;
      }
    }
    delete resp;
    return p;
  }

  delete resp;
  return PVR->AllocateDemuxPacket(0);
}

bool cVNSIDemux::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "changing to channel %d", channelinfo.number);

  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELSTREAM_OPEN) || !vrp.add_U32(channelinfo.number) || !m_session.ReadSuccess(&vrp))
  {
    XBMC->Log(LOG_ERROR, "cVNSIDemux::SetChannel - failed to set channel");
  }
  else
  {
    m_channel           = channelinfo.number;
    m_Streams.nstreams  = 0;
    m_startup           = true;
    while (m_Streams.nstreams == 0 && m_StatusCount == 0)
    {
      DemuxPacket* pkg = Read();
      if(!pkg)
        return false;
      PVR->FreeDemuxPacket(pkg);
    }
    return true;
  }
  return false;
}

bool cVNSIDemux::GetSignalStatus(PVR_SIGNALQUALITY &qualityinfo)
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

void cVNSIDemux::StreamChange(cResponsePacket *resp)
{
  m_Streams.nstreams = 0;

  while (!resp->end())
  {
    uint32_t    index = resp->extract_U32();
    const char* type  = resp->extract_String();

    DEVDBG("cVNSIDemux::StreamChange - id: %d, type: %s", index, type);

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
      const char *language = resp->extract_String();

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_AC3;
      m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
      m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
      m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      const char *language = resp->extract_String();

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_MP2;
      m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
      m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
      m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "AAC"))
    {
      const char *language = resp->extract_String();

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_AAC;
      m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
      m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
      m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "DTS"))
    {
      const char *language = resp->extract_String();

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_DTS;
      m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
      m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
      m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "EAC3"))
    {
      const char *language = resp->extract_String();

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_AUDIO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_EAC3;
      m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
      m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
      m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_VIDEO;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_MPEG2VIDEO;
      m_Streams.stream[m_Streams.nstreams].fpsscale   = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].fpsrate    = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].height     = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].width      = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].aspect     = resp->extract_Double();
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
      m_Streams.stream[m_Streams.nstreams].fpsscale   = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].fpsrate    = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].height     = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].width      = resp->extract_U32();
      m_Streams.stream[m_Streams.nstreams].aspect     = resp->extract_Double();
      m_Streams.stream[m_Streams.nstreams].language[0]= 0;
      m_Streams.stream[m_Streams.nstreams].language[1]= 0;
      m_Streams.stream[m_Streams.nstreams].language[2]= 0;
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = -1;
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "DVBSUB"))
    {
      const char *language    = resp->extract_String();
      uint32_t composition_id = resp->extract_U32();
      uint32_t ancillary_id   = resp->extract_U32();

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_DVB_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
      m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
      m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
      m_Streams.stream[m_Streams.nstreams].language[3]= 0;
      m_Streams.stream[m_Streams.nstreams].identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
      m_Streams.nstreams++;
    }
    else if(!strcmp(type, "TEXTSUB"))
    {
      const char *language = resp->extract_String();

      m_Streams.stream[m_Streams.nstreams].id         = m_Streams.nstreams;
      m_Streams.stream[m_Streams.nstreams].physid     = index;
      m_Streams.stream[m_Streams.nstreams].codec_type = CODEC_TYPE_SUBTITLE;
      m_Streams.stream[m_Streams.nstreams].codec_id   = CODEC_ID_TEXT;
      m_Streams.stream[m_Streams.nstreams].language[0]= language[0];
      m_Streams.stream[m_Streams.nstreams].language[1]= language[1];
      m_Streams.stream[m_Streams.nstreams].language[2]= language[2];
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
      XBMC->Log(LOG_ERROR, "cVNSIDemux::StreamChange - max amount of streams reached");
      break;
    }
  }
}

void cVNSIDemux::StreamStatus(cResponsePacket *resp)
{
  const char* status = resp->extract_String();
  if(status == NULL)
    m_Status = "";
  else
  {
    m_StatusCount++;
    m_Status = status;
    XBMC->Log(LOG_DEBUG, "cVNSIDemux::StreamStatus - %s", status);
    XBMC->QueueNotification(QUEUE_INFO, status);
  }
}

void cVNSIDemux::StreamSignalInfo(cResponsePacket *resp)
{
  m_Quality.fe_name   = resp->extract_String();
  m_Quality.fe_status = resp->extract_String();
  m_Quality.fe_snr    = resp->extract_U32();
  m_Quality.fe_signal = resp->extract_U32();
  m_Quality.fe_ber    = resp->extract_U32();
  m_Quality.fe_unc    = resp->extract_U32();
}

void cVNSIDemux::StreamContentInfo(cResponsePacket *resp)
{
  for (int i = 0; i < m_Streams.nstreams && !resp->end(); i++)
  {
    uint32_t index = resp->extract_U32();
    if (index == m_Streams.stream[i].physid)
    {
      if (m_Streams.stream[i].codec_type == CODEC_TYPE_AUDIO)
      {
        const char *language = resp->extract_String();
        m_Streams.stream[i].channels          = resp->extract_U32();
        m_Streams.stream[i].samplerate        = resp->extract_U32();
        m_Streams.stream[i].blockalign        = resp->extract_U32();
        m_Streams.stream[i].bitrate           = resp->extract_U32();
        m_Streams.stream[i].bits_per_sample   = resp->extract_U32();
        m_Streams.stream[i].language[0]       = language[0];
        m_Streams.stream[i].language[1]       = language[1];
        m_Streams.stream[i].language[2]       = language[2];
        m_Streams.stream[i].language[3]       = 0;
      }
      else if (m_Streams.stream[i].codec_type == CODEC_TYPE_VIDEO)
      {
        m_Streams.stream[i].fpsscale         = resp->extract_U32();
        m_Streams.stream[i].fpsrate          = resp->extract_U32();
        m_Streams.stream[i].height           = resp->extract_U32();
        m_Streams.stream[i].width            = resp->extract_U32();
        m_Streams.stream[i].aspect           = resp->extract_Double();
      }
      else if (m_Streams.stream[i].codec_type == CODEC_TYPE_SUBTITLE)
      {
        const char *language    = resp->extract_String();
        uint32_t composition_id = resp->extract_U32();
        uint32_t ancillary_id   = resp->extract_U32();
        m_Streams.stream[i].identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
        m_Streams.stream[i].language[0]= language[0];
        m_Streams.stream[i].language[1]= language[1];
        m_Streams.stream[i].language[2]= language[2];
        m_Streams.stream[i].language[3]= 0;
      }
    }
  }
}
