/*
 *      Copyright (C) 2012 Team XBMC
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

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxPVRClient.h"
#include "DVDDemuxUtils.h"
#include "utils/log.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - FF_INPUT_BUFFER_PADDING_SIZE)

using namespace PVR;

CDemuxStreamVideoPVRClient::~CDemuxStreamVideoPVRClient()
{
  if (m_pParser)
  {
    m_parent->m_dllAvCodec.av_parser_close(m_pParser);
    m_pParser = NULL;
  }
}

void CDemuxStreamVideoPVRClient::GetStreamInfo(std::string& strInfo)
{
  switch (codec)
  {
    case CODEC_ID_MPEG2VIDEO:
      strInfo = "mpeg2video";
      break;
    case CODEC_ID_H264:
      strInfo = "h264";
      break;
    default:
      break;
  }
}

CDemuxStreamAudioPVRClient::~CDemuxStreamAudioPVRClient()
{
  if (m_pParser)
  {
    m_parent->m_dllAvCodec.av_parser_close(m_pParser);
    m_pParser = NULL;
  }
}

void CDemuxStreamAudioPVRClient::GetStreamInfo(std::string& strInfo)
{
  switch (codec)
  {
    case CODEC_ID_AC3:
      strInfo = "ac3";
      break;
    case CODEC_ID_EAC3:
      strInfo = "eac3";
      break;
    case CODEC_ID_MP2:
      strInfo = "mpeg2audio";
      break;
    case CODEC_ID_AAC:
      strInfo = "aac";
      break;
    case CODEC_ID_DTS:
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
  for (int i = 0; i < MAX_STREAMS; i++) m_streamsToParse[i] = NULL;
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

  RequestStreams();

  if (!m_dllAvCodec.Load())
  {
    CLog::Log(LOGWARNING, "%s could not load ffmpeg", __FUNCTION__);
    return false;
  }

  // register codecs
  m_dllAvCodec.avcodec_register_all();
  return true;
}

void CDVDDemuxPVRClient::Dispose()
{
  for (int i = 0; i < MAX_STREAMS; i++)
  {
    if (m_streams[i])
    {
      if (m_streams[i]->ExtraData)
        delete[] (BYTE*)(m_streams[i]->ExtraData);
      delete m_streams[i];
    }
    m_streams[i] = NULL;

    if (m_streamsToParse[i])
    {
      if (m_streamsToParse[i]->ExtraData)
        delete[] (BYTE*)(m_streamsToParse[i]->ExtraData);
      delete m_streamsToParse[i];
    }
    m_streamsToParse[i] = NULL;
  }

  m_pInput = NULL;

  m_dllAvCodec.Unload();
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

bool CDVDDemuxPVRClient::ParsePacket(DemuxPacket* pPacket)
{
  bool bReturn(false);

  if (pPacket && pPacket->iSize)
  {
    CDemuxStream* st = m_streamsToParse[pPacket->iStreamId];
    AVCodecParserContext* pParser = NULL;
    if (st && st->type == STREAM_VIDEO)
      pParser = ((CDemuxStreamVideoPVRClient*)st)->m_pParser;
    else if (st && st->type == STREAM_AUDIO)
      pParser = ((CDemuxStreamAudioPVRClient*)st)->m_pParser;

    if (st && pParser)
    {
      // use split function of parser to get SPS
      if (pParser->parser->split)
      {
        AVCodec *codec;
        AVCodecContext *pCodecContext = NULL;
        codec = m_dllAvCodec.avcodec_find_decoder(st->codec);
        if (!codec)
        {
          CLog::Log(LOGERROR, "%s - Error, can't find decoder", __FUNCTION__);
        }
        else
        {
          pCodecContext = m_dllAvCodec.avcodec_alloc_context3(codec);
          int i = pParser->parser->split(pCodecContext, pPacket->pData, pPacket->iSize);
          if (i > 0 && i < FF_MAX_EXTRADATA_SIZE)
          {
            if (st->ExtraData)
              delete[] (uint8_t*)(st->ExtraData);
            st->ExtraSize = i;
            st->ExtraData = new uint8_t[st->ExtraSize+FF_INPUT_BUFFER_PADDING_SIZE];
            memcpy(st->ExtraData, pPacket->pData, st->ExtraSize);
            memset((uint8_t*)st->ExtraData + st->ExtraSize, 0 , FF_INPUT_BUFFER_PADDING_SIZE);
            bReturn = true;
          }
          else
          {
            CLog::Log(LOGERROR, "%s - Error, could not split extra data", __FUNCTION__);
          }
        }
        m_dllAvCodec.avcodec_close(pCodecContext);
      }
      else
      {
        // steam has no extradata to split
        bReturn = true;
      }
    }
  }

  return bReturn;
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
    UpdateStreams((PVR_STREAM_PROPERTIES*)pPacket->pData);
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    Reset();
  }

  // check if streams needs parsing
  int streamId = pPacket->iStreamId;
  CDemuxStream *stream = NULL;
  if (streamId >= 0 && streamId < MAX_STREAMS)
    stream = m_streamsToParse[streamId];
  if (stream)
  {
    if (!ParsePacket(pPacket))
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      return CDVDDemuxUtils::AllocateDemuxPacket(0);
    }
    else
    {
      m_streams[streamId] = m_streamsToParse[streamId];
      m_streamsToParse[streamId] = NULL;
    }
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

  PVR_STREAM_PROPERTIES props;
  m_pvrClient->GetStreamProperties(&props);

  for (unsigned int i = 0; i < props.iStreamCount; ++i)
  {
    CDemuxStream* (*streams)[MAX_STREAMS] = &m_streams;

    if (props.stream[i].iCodecType == AVMEDIA_TYPE_AUDIO)
    {
      CDemuxStreamAudioPVRClient* st = new CDemuxStreamAudioPVRClient(this);
      st->iChannels       = props.stream[i].iChannels;
      st->iSampleRate     = props.stream[i].iSampleRate;
      st->iBlockAlign     = props.stream[i].iBlockAlign;
      st->iBitRate        = props.stream[i].iBitRate;
      st->iBitsPerSample  = props.stream[i].iBitsPerSample;
      st->m_pParser = m_dllAvCodec.av_parser_init(props.stream[i].iCodecId);
      if (st->m_pParser)
      {
        m_streamsToParse[props.stream[i].iStreamIndex] = st;
        streams = &m_streamsToParse;
      }
      else
        m_streams[props.stream[i].iStreamIndex] = st;
    }
    else if (props.stream[i].iCodecType == AVMEDIA_TYPE_VIDEO)
    {
      CDemuxStreamVideoPVRClient* st = new CDemuxStreamVideoPVRClient(this);
      st->iFpsScale       = props.stream[i].iFPSScale;
      st->iFpsRate        = props.stream[i].iFPSRate;
      st->iHeight         = props.stream[i].iHeight;
      st->iWidth          = props.stream[i].iWidth;
      st->fAspect         = props.stream[i].fAspect;
      st->m_pParser = m_dllAvCodec.av_parser_init(props.stream[i].iCodecId);
      if (st->m_pParser)
      {
        m_streamsToParse[props.stream[i].iStreamIndex] = st;
        streams = &m_streamsToParse;
      }
      else
        m_streams[props.stream[i].iStreamIndex] = st;
    }
    else if (props.stream[i].iCodecId == CODEC_ID_DVB_TELETEXT)
    {
      m_streams[props.stream[i].iStreamIndex] = new CDemuxStreamTeletext();
    }
    else if (props.stream[i].iCodecType == AVMEDIA_TYPE_SUBTITLE)
    {
      CDemuxStreamSubtitlePVRClient* st = new CDemuxStreamSubtitlePVRClient(this);
      st->identifier      = props.stream[i].iIdentifier;
      m_streams[props.stream[i].iStreamIndex] = st;
    }
    else
      m_streams[props.stream[i].iStreamIndex] = new CDemuxStream();

    (*streams)[props.stream[i].iStreamIndex]->codec       = (CodecID)props.stream[i].iCodecId;
    (*streams)[props.stream[i].iStreamIndex]->iId         = props.stream[i].iStreamIndex;
    (*streams)[props.stream[i].iStreamIndex]->iPhysicalId = props.stream[i].iPhysicalId;
    (*streams)[props.stream[i].iStreamIndex]->language[0] = props.stream[i].strLanguage[0];
    (*streams)[props.stream[i].iStreamIndex]->language[1] = props.stream[i].strLanguage[1];
    (*streams)[props.stream[i].iStreamIndex]->language[2] = props.stream[i].strLanguage[2];
    (*streams)[props.stream[i].iStreamIndex]->language[3] = props.stream[i].strLanguage[3];

    CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::RequestStreams(): added stream %d:%d with codec_id %d",
        (*streams)[props.stream[i].iStreamIndex]->iId,
        (*streams)[props.stream[i].iStreamIndex]->iPhysicalId,
        (*streams)[props.stream[i].iStreamIndex]->codec);
  }
}

void CDVDDemuxPVRClient::UpdateStreams(PVR_STREAM_PROPERTIES *props)
{
  bool bGotVideoStream(false);

  for (unsigned int i = 0; i < props->iStreamCount; ++i)
  {
    CDemuxStream* (*streams)[MAX_STREAMS] = &m_streams;

    if (m_streams[props->stream[i].iStreamIndex] != NULL &&
        m_streams[props->stream[i].iStreamIndex]->codec == (CodecID)props->stream[i].iCodecId)
    {
      streams = &m_streams;
    }
    else if (m_streamsToParse[props->stream[i].iStreamIndex] != NULL &&
             m_streamsToParse[props->stream[i].iStreamIndex]->codec == (CodecID)props->stream[i].iCodecId)
    {
      streams = &m_streamsToParse;
    }
    else
    {
      CLog::Log(LOGERROR,"Invalid stream inside UpdateStreams");
      continue;
    }

    if ((*streams)[props->stream[i].iStreamIndex]->type == STREAM_AUDIO)
    {
      CDemuxStreamAudioPVRClient* st = (CDemuxStreamAudioPVRClient*) (*streams)[props->stream[i].iStreamIndex];
      st->iChannels       = props->stream[i].iChannels;
      st->iSampleRate     = props->stream[i].iSampleRate;
      st->iBlockAlign     = props->stream[i].iBlockAlign;
      st->iBitRate        = props->stream[i].iBitRate;
      st->iBitsPerSample  = props->stream[i].iBitsPerSample;
    }
    else if ((*streams)[props->stream[i].iStreamIndex]->type == STREAM_VIDEO)
    {
      if (bGotVideoStream)
      {
        CLog::Log(LOGDEBUG, "CDVDDemuxPVRClient - %s - skip video stream", __FUNCTION__);
        continue;
      }

      CDemuxStreamVideoPVRClient* st = (CDemuxStreamVideoPVRClient*) (*streams)[props->stream[i].iStreamIndex];
      if (st->iWidth <= 0 || st->iHeight <= 0)
      {
        CLog::Log(LOGWARNING, "CDVDDemuxPVRClient - %s - invalid stream data", __FUNCTION__);
        continue;
      }

      st->iFpsScale       = props->stream[i].iFPSScale;
      st->iFpsRate        = props->stream[i].iFPSRate;
      st->iHeight         = props->stream[i].iHeight;
      st->iWidth          = props->stream[i].iWidth;
      st->fAspect         = props->stream[i].fAspect;
      bGotVideoStream = true;
    }
    else if ((*streams)[props->stream[i].iStreamIndex]->type == STREAM_SUBTITLE)
    {
      CDemuxStreamSubtitlePVRClient* st = (CDemuxStreamSubtitlePVRClient*) (*streams)[props->stream[i].iStreamIndex];
      st->identifier      = props->stream[i].iIdentifier;
    }

    (*streams)[props->stream[i].iStreamIndex]->language[0] = props->stream[i].strLanguage[0];
    (*streams)[props->stream[i].iStreamIndex]->language[1] = props->stream[i].strLanguage[1];
    (*streams)[props->stream[i].iStreamIndex]->language[2] = props->stream[i].strLanguage[2];
    (*streams)[props->stream[i].iStreamIndex]->language[3] = props->stream[i].strLanguage[3];

    CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::UpdateStreams(): update stream %d:%d with codec_id %d",
        (*streams)[props->stream[i].iStreamIndex]->iId,
        (*streams)[props->stream[i].iStreamIndex]->iPhysicalId,
        (*streams)[props->stream[i].iStreamIndex]->codec);
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

void CDVDDemuxPVRClient::GetStreamCodecName(int iStreamId, CStdString &strName)
{
  CDemuxStream *stream = GetStream(iStreamId);
  if (stream)
  {
    if (stream->codec == CODEC_ID_AC3)
      strName = "ac3";
    else if (stream->codec == CODEC_ID_MP2)
      strName = "mp2";
    else if (stream->codec == CODEC_ID_AAC)
      strName = "aac";
    else if (stream->codec == CODEC_ID_DTS)
      strName = "dca";
    else if (stream->codec == CODEC_ID_MPEG2VIDEO)
      strName = "mpeg2video";
    else if (stream->codec == CODEC_ID_H264)
      strName = "h264";
    else if (stream->codec == CODEC_ID_EAC3)
      strName = "eac3";
  }
}
