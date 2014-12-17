/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxPVRClient.h"
#include "DVDDemuxUtils.h"
#include "utils/log.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "../DVDClock.h"

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - FF_INPUT_BUFFER_PADDING_SIZE)

using namespace PVR;

CDemuxStreamPVRInternal::CDemuxStreamPVRInternal(CDVDDemuxPVRClient *parent)
 : m_parent(parent)
 , m_parser(NULL)
 , m_context(NULL)
 , m_parser_split(false)
{
}

CDemuxStreamPVRInternal::~CDemuxStreamPVRInternal()
{
  DisposeParser();
}

void CDemuxStreamPVRInternal::DisposeParser()
{
  if (m_parser)
  {
    av_parser_close(m_parser);
    m_parser = NULL;
  }
  if (m_context)
  {
    avcodec_close(m_context);
    m_context = NULL;
  }
}

void CDemuxStreamVideoPVRClient::GetStreamInfo(std::string& strInfo)
{
  switch (codec)
  {
    case AV_CODEC_ID_MPEG2VIDEO:
      strInfo = "mpeg2video";
      break;
    case AV_CODEC_ID_H264:
      strInfo = "h264";
      break;
    default:
      break;
  }
}

void CDemuxStreamAudioPVRClient::GetStreamInfo(std::string& strInfo)
{
  switch (codec)
  {
    case AV_CODEC_ID_AC3:
      strInfo = "ac3";
      break;
    case AV_CODEC_ID_EAC3:
      strInfo = "eac3";
      break;
    case AV_CODEC_ID_MP2:
      strInfo = "mpeg2audio";
      break;
    case AV_CODEC_ID_AAC:
      strInfo = "aac";
      break;
    case AV_CODEC_ID_DTS:
      strInfo = "dts";
      break;
    default:
      break;
  }
}

void CDemuxStreamSubtitlePVRClient::GetStreamInfo(std::string& strInfo)
{
}

CDVDDemuxPVRClient::CDVDDemuxPVRClient() : CDVDDemux()
{
  m_pInput = NULL;
  for (int i = 0; i < MAX_STREAMS; i++) m_streams[i] = NULL;
}

CDVDDemuxPVRClient::~CDVDDemuxPVRClient()
{
  Dispose();
}

bool CDVDDemuxPVRClient::Open(CDVDInputStream* pInput)
{
  Abort();

  m_pInput = pInput;
  if (!g_PVRClients->GetPlayingClient(m_pvrClient))
    return false;

  return true;
}

void CDVDDemuxPVRClient::Dispose()
{
  for (int i = 0; i < MAX_STREAMS; i++)
  {
    delete m_streams[i];
    m_streams[i] = NULL;
  }

  m_pInput = NULL;
}

void CDVDDemuxPVRClient::DisposeStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS)
    return;
  delete m_streams[iStreamId];
  m_streams[iStreamId] = NULL;
}

void CDVDDemuxPVRClient::Reset()
{
  if(m_pInput && g_PVRManager.IsStarted())
    m_pvrClient->DemuxReset();

  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxPVRClient::Abort()
{
  if(m_pInput)
    m_pvrClient->DemuxAbort();
}

void CDVDDemuxPVRClient::Flush()
{
  if(m_pInput && g_PVRManager.IsStarted())
    m_pvrClient->DemuxFlush();
}

void CDVDDemuxPVRClient::ParsePacket(DemuxPacket* pkt)
{
  CDemuxStream* st = m_streams[pkt->iStreamId];
  if (st == NULL)
    return;

  if (st->ExtraSize)
    return;

  CDemuxStreamPVRInternal* pvr = dynamic_cast<CDemuxStreamPVRInternal*>(st);

  if(pvr == NULL
  || pvr->m_parser == NULL)
    return;

  if(pvr->m_context == NULL)
  {
    AVCodec *codec = avcodec_find_decoder(st->codec);
    if (codec == NULL)
    {
      CLog::Log(LOGERROR, "%s - can't find decoder", __FUNCTION__);
      pvr->DisposeParser();
      return;
    }

    pvr->m_context = avcodec_alloc_context3(codec);
    if(pvr->m_context == NULL)
    {
      CLog::Log(LOGERROR, "%s - can't allocate context", __FUNCTION__);
      pvr->DisposeParser();
      return;
    }
    pvr->m_context->time_base.num = 1;
    pvr->m_context->time_base.den = DVD_TIME_BASE;
  }

  if(pvr->m_parser_split && pvr->m_parser->parser->split)
  {
    int len = pvr->m_parser->parser->split(pvr->m_context, pkt->pData, pkt->iSize);
    if (len > 0 && len < FF_MAX_EXTRADATA_SIZE)
    {
      if (st->ExtraData)
        delete[] (uint8_t*)st->ExtraData;
      st->changes++;
      st->disabled = false;
      st->ExtraSize = len;
      st->ExtraData = new uint8_t[len+FF_INPUT_BUFFER_PADDING_SIZE];
      memcpy(st->ExtraData, pkt->pData, len);
      memset((uint8_t*)st->ExtraData + len, 0 , FF_INPUT_BUFFER_PADDING_SIZE);
      pvr->m_parser_split = false;
    }
  }


  uint8_t *outbuf = NULL;
  int      outbuf_size = 0;
  int len = av_parser_parse2(pvr->m_parser
                                        , pvr->m_context, &outbuf, &outbuf_size
                                        , pkt->pData, pkt->iSize
                                        , (int64_t)(pkt->pts * DVD_TIME_BASE)
                                        , (int64_t)(pkt->dts * DVD_TIME_BASE)
                                        , 0);
  /* our parse is setup to parse complete frames, so we don't care about outbufs */
  if(len >= 0)
  {
#define CHECK_UPDATE(st, trg, src, invalid) do { \
      if(src != invalid \
      && src != st->trg) { \
        CLog::Log(LOGDEBUG, "%s - {%d} " #trg " changed from %d to %d",  __FUNCTION__, st->iId, st->trg, src); \
        st->trg = src; \
        st->changes++; \
        st->disabled = false; \
      } \
    } while(0)


    CHECK_UPDATE(st, profile, pvr->m_context->profile , FF_PROFILE_UNKNOWN);
    CHECK_UPDATE(st, level  , pvr->m_context->level   , FF_LEVEL_UNKNOWN);

    switch (st->type)
    {
      case STREAM_AUDIO: {
        CDemuxStreamAudioPVRClient* sta = static_cast<CDemuxStreamAudioPVRClient*>(st);
        CHECK_UPDATE(sta, iChannels     , pvr->m_context->channels   , 0);
        CHECK_UPDATE(sta, iSampleRate   , pvr->m_context->sample_rate, 0);
        break;
      }
      case STREAM_VIDEO: {
        CDemuxStreamVideoPVRClient* stv = static_cast<CDemuxStreamVideoPVRClient*>(st);
        CHECK_UPDATE(stv, iWidth        , pvr->m_context->width , 0);
        CHECK_UPDATE(stv, iHeight       , pvr->m_context->height, 0);
        break;
      }

      default:
        break;
    }

#undef CHECK_UPDATE
  }
  else
    CLog::Log(LOGDEBUG, "%s - parser returned error %d", __FUNCTION__, len);

  return;
}

DemuxPacket* CDVDDemuxPVRClient::Read()
{
  if (!g_PVRManager.IsStarted())
    return CDVDDemuxUtils::AllocateDemuxPacket(0);

  DemuxPacket* pPacket = m_pvrClient->DemuxRead();
  if (!pPacket)
  {
    if (m_pInput)
      m_pInput->Close();
    return NULL;
  }

  if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    RequestStreams();
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    RequestStreams();
  }
  else if (pPacket->iStreamId >= 0
        && pPacket->iStreamId < MAX_STREAMS
        && m_streams[pPacket->iStreamId])
  {
    ParsePacket(pPacket);
  }

  return pPacket;
}

CDemuxStream* CDVDDemuxPVRClient::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS) return NULL;
    return m_streams[iStreamId];
}

void CDVDDemuxPVRClient::RequestStreams()
{
  if (!g_PVRManager.IsStarted())
    return;

  PVR_STREAM_PROPERTIES props = {};
  m_pvrClient->GetStreamProperties(&props);
  unsigned int i;

  for (i = 0; i < props.iStreamCount; ++i)
  {
    CDemuxStream *stm = m_streams[i];

    if (props.stream[i].iCodecType == XBMC_CODEC_TYPE_AUDIO)
    {
      CDemuxStreamAudioPVRClient* st = NULL;
      if (stm)
      {
        st = dynamic_cast<CDemuxStreamAudioPVRClient*>(stm);
        if (!st || (st->codec != (AVCodecID)props.stream[i].iCodecId))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamAudioPVRClient(this);
        st->m_parser = av_parser_init(props.stream[i].iCodecId);
        if(st->m_parser)
          st->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      }
      st->iChannels       = props.stream[i].iChannels;
      st->iSampleRate     = props.stream[i].iSampleRate;
      st->iBlockAlign     = props.stream[i].iBlockAlign;
      st->iBitRate        = props.stream[i].iBitRate;
      st->iBitsPerSample  = props.stream[i].iBitsPerSample;
      m_streams[i] = st;
      st->m_parser_split = true;
      st->changes++;
    }
    else if (props.stream[i].iCodecType == XBMC_CODEC_TYPE_VIDEO)
    {
      CDemuxStreamVideoPVRClient* st = NULL;
      if (stm)
      {
        st = dynamic_cast<CDemuxStreamVideoPVRClient*>(stm);
        if (!st
            || (st->codec != (AVCodecID)props.stream[i].iCodecId)
            || (st->iWidth != props.stream[i].iWidth)
            || (st->iHeight != props.stream[i].iHeight))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamVideoPVRClient(this);
        st->m_parser = av_parser_init(props.stream[i].iCodecId);
        if(st->m_parser)
          st->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      }
      st->iFpsScale       = props.stream[i].iFPSScale;
      st->iFpsRate        = props.stream[i].iFPSRate;
      st->iHeight         = props.stream[i].iHeight;
      st->iWidth          = props.stream[i].iWidth;
      st->fAspect         = props.stream[i].fAspect;
      st->stereo_mode     = "mono";
      m_streams[i] = st;
      st->m_parser_split = true;
    }
    else if (props.stream[i].iCodecId == AV_CODEC_ID_DVB_TELETEXT)
    {
      if (stm)
      {
        if (stm->codec != (AVCodecID)props.stream[i].iCodecId)
          DisposeStream(i);
      }
      if (!m_streams[i])
        m_streams[i] = new CDemuxStreamTeletext();
    }
    else if (props.stream[i].iCodecType == XBMC_CODEC_TYPE_SUBTITLE)
    {
      CDemuxStreamSubtitlePVRClient* st = NULL;
      if (stm)
      {
        st = dynamic_cast<CDemuxStreamSubtitlePVRClient*>(stm);
        if (!st || (st->codec != (AVCodecID)props.stream[i].iCodecId))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamSubtitlePVRClient(this);
      }
      if(props.stream[i].iIdentifier)
      {
        st->ExtraData = new uint8_t[4];
        st->ExtraSize = 4;
        st->ExtraData[0] = (props.stream[i].iIdentifier >> 8) & 0xff;
        st->ExtraData[1] = (props.stream[i].iIdentifier >> 0) & 0xff;
        st->ExtraData[2] = (props.stream[i].iIdentifier >> 24) & 0xff;
        st->ExtraData[3] = (props.stream[i].iIdentifier >> 16) & 0xff;
      }
      m_streams[i] = st;
    }
    else
    {
      if (m_streams[i])
        DisposeStream(i);
      m_streams[i] = new CDemuxStream();
    }

    m_streams[i]->codec       = (AVCodecID)props.stream[i].iCodecId;
    m_streams[i]->iId         = i;
    m_streams[i]->iPhysicalId = props.stream[i].iPhysicalId;
    m_streams[i]->language[0] = props.stream[i].strLanguage[0];
    m_streams[i]->language[1] = props.stream[i].strLanguage[1];
    m_streams[i]->language[2] = props.stream[i].strLanguage[2];
    m_streams[i]->language[3] = props.stream[i].strLanguage[3];

    CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::RequestStreams(): added/updated stream %d:%d with codec_id %d",
        m_streams[i]->iId,
        m_streams[i]->iPhysicalId,
        m_streams[i]->codec);
  }
  // check if we need to dispose any streams no longer in props
  for (unsigned int j = i; j < MAX_STREAMS; j++)
  {
    if (m_streams[j])
    {
      CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::RequestStreams(): disposed stream %d:%d with codec_id %d",
          m_streams[j]->iId,
          m_streams[j]->iPhysicalId,
          m_streams[j]->codec);
      DisposeStream(j);
    }
  }
}

int CDVDDemuxPVRClient::GetNrOfStreams()
{
  int i = 0;
  while (i < MAX_STREAMS && m_streams[i]) i++;
  return i;
}

std::string CDVDDemuxPVRClient::GetFileName()
{
  if(m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

void CDVDDemuxPVRClient::GetStreamCodecName(int iStreamId, std::string &strName)
{
  CDemuxStream *stream = GetStream(iStreamId);
  if (stream)
  {
    if (stream->codec == AV_CODEC_ID_AC3)
      strName = "ac3";
    else if (stream->codec == AV_CODEC_ID_MP2)
      strName = "mp2";
    else if (stream->codec == AV_CODEC_ID_AAC)
      strName = "aac";
    else if (stream->codec == AV_CODEC_ID_DTS)
      strName = "dca";
    else if (stream->codec == AV_CODEC_ID_MPEG2VIDEO)
      strName = "mpeg2video";
    else if (stream->codec == AV_CODEC_ID_H264)
      strName = "h264";
    else if (stream->codec == AV_CODEC_ID_EAC3)
      strName = "eac3";
  }
}

bool CDVDDemuxPVRClient::SeekTime(int timems, bool backwards, double *startpts)
{
  if (m_pInput)
    return m_pvrClient->SeekTime(timems, backwards, startpts);
  return false;
}

void CDVDDemuxPVRClient::SetSpeed ( int speed )
{
  if (m_pInput)
    m_pvrClient->SetSpeed(speed);
}
