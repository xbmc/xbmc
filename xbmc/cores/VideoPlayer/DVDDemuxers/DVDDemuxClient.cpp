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
#include "DVDDemuxClient.h"
#include "DVDDemuxUtils.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "../DVDClock.h"

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - FF_INPUT_BUFFER_PADDING_SIZE)


class CDemuxStreamClientInternal
{
public:
  CDemuxStreamClientInternal()
  : m_parser(nullptr)
  , m_context(nullptr)
  , m_parser_split(false)
  {
  }

  ~CDemuxStreamClientInternal()
  {
    DisposeParser();
  }

  void DisposeParser()
  {
    if (m_parser)
    {
      av_parser_close(m_parser);
      m_parser = nullptr;
    }
    if (m_context)
    {
      avcodec_close(m_context);
      m_context = nullptr;
    }
  }

  AVCodecParserContext *m_parser;
  AVCodecContext *m_context;
  bool m_parser_split;
};

class CDemuxStreamVideoClient
: public CDemuxStreamVideo
, public CDemuxStreamClientInternal
{
public:
  CDemuxStreamVideoClient() {}
  virtual std::string GetStreamInfo() override;
};

class CDemuxStreamAudioClient
: public CDemuxStreamAudio
, public CDemuxStreamClientInternal
{
public:
  CDemuxStreamAudioClient() {}
  virtual std::string GetStreamInfo() override;
};

class CDemuxStreamSubtitleClient
: public CDemuxStreamSubtitle
, public CDemuxStreamClientInternal
{
public:
  CDemuxStreamSubtitleClient() {}
  virtual std::string GetStreamInfo() override;
};

std::string CDemuxStreamVideoClient::GetStreamInfo()
{
  std::string strInfo;
  switch (codec)
  {
    case AV_CODEC_ID_MPEG2VIDEO:
      strInfo = "mpeg2video";
      break;
    case AV_CODEC_ID_H264:
      strInfo = "h264";
      break;
    case AV_CODEC_ID_HEVC:
      strInfo = "hevc";
      break;
    default:
      break;
  }

  return strInfo;
}

std::string CDemuxStreamAudioClient::GetStreamInfo()
{
  std::string strInfo;
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

  return strInfo;
}

std::string CDemuxStreamSubtitleClient::GetStreamInfo()
{
  return "";
}

CDVDDemuxClient::CDVDDemuxClient() : CDVDDemux()
{
  m_pInput = nullptr;
  m_IDemux = nullptr;
  for (int i = 0; i < MAX_STREAMS; i++)
    m_streams[i] = nullptr;
}

CDVDDemuxClient::~CDVDDemuxClient()
{
  Dispose();
}

bool CDVDDemuxClient::Open(CDVDInputStream* pInput)
{
  Abort();

  m_pInput = pInput;
  m_IDemux = dynamic_cast<CDVDInputStream::IDemux*>(m_pInput);
  if (!m_IDemux)
    return false;

  if (!m_IDemux->OpenDemux())
    return false;

  RequestStreams();
  return true;
}

void CDVDDemuxClient::Dispose()
{
  DisposeStreams();

  m_pInput = nullptr;
  m_IDemux = nullptr;
}

void CDVDDemuxClient::DisposeStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS)
    return;
  delete m_streams[iStreamId];
  m_streams[iStreamId] = nullptr;
}

void CDVDDemuxClient::DisposeStreams()
{
  for (int i=0; i< MAX_STREAMS; i++)
    DisposeStream(i);
}

void CDVDDemuxClient::Reset()
{
  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxClient::Abort()
{
  if (m_IDemux)
    m_IDemux->AbortDemux();
}

void CDVDDemuxClient::Flush()
{
  if (m_IDemux)
    m_IDemux->FlushDemux();
}

void CDVDDemuxClient::ParsePacket(DemuxPacket* pkt)
{
  CDemuxStream* st = m_streams[pkt->iStreamId];
  if (st == nullptr)
    return;

  if (st->ExtraSize)
    return;

  CDemuxStreamClientInternal* stream = dynamic_cast<CDemuxStreamClientInternal*>(st);

  if (stream == nullptr ||
     stream->m_parser == nullptr)
    return;

  if (stream->m_context == nullptr)
  {
    AVCodec *codec = avcodec_find_decoder(st->codec);
    if (codec == nullptr)
    {
      CLog::Log(LOGERROR, "%s - can't find decoder", __FUNCTION__);
      stream->DisposeParser();
      return;
    }

    stream->m_context = avcodec_alloc_context3(codec);
    if (stream->m_context == nullptr)
    {
      CLog::Log(LOGERROR, "%s - can't allocate context", __FUNCTION__);
      stream->DisposeParser();
      return;
    }
    stream->m_context->time_base.num = 1;
    stream->m_context->time_base.den = DVD_TIME_BASE;
  }

  if (stream->m_parser_split && stream->m_parser->parser->split)
  {
    int len = stream->m_parser->parser->split(stream->m_context, pkt->pData, pkt->iSize);
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
      stream->m_parser_split = false;
    }
  }


  uint8_t *outbuf = nullptr;
  int outbuf_size = 0;
  int len = av_parser_parse2(stream->m_parser,
                             stream->m_context, &outbuf, &outbuf_size,
                             pkt->pData, pkt->iSize,
                             (int64_t)(pkt->pts * DVD_TIME_BASE),
                             (int64_t)(pkt->dts * DVD_TIME_BASE),
                             0);
  // our parse is setup to parse complete frames, so we don't care about outbufs
  if(len >= 0)
  {
    if (stream->m_context->profile != st->profile &&
        stream->m_context->profile != FF_PROFILE_UNKNOWN)
    {
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - {%d} profile changed from %d to %d", st->iId, st->profile, stream->m_context->profile);
      st->profile = stream->m_context->profile;
      st->changes++;
      st->disabled = false;
    }

    if (stream->m_context->level != st->level &&
        stream->m_context->level != FF_LEVEL_UNKNOWN)
    {
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - {%d} level changed from %d to %d", st->iId, st->level, stream->m_context->level);
      st->level = stream->m_context->level;
      st->changes++;
      st->disabled = false;
    }

    switch (st->type)
    {
      case STREAM_AUDIO:
      {
        CDemuxStreamAudioClient* sta = static_cast<CDemuxStreamAudioClient*>(st);
        if (stream->m_context->channels != sta->iChannels &&
            stream->m_context->channels != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - {%d} channels changed from %d to %d", st->iId, sta->iChannels, stream->m_context->channels);
          sta->iChannels = stream->m_context->channels;
          sta->changes++;
          sta->disabled = false;
        }
        if (stream->m_context->sample_rate != sta->iSampleRate &&
            stream->m_context->sample_rate != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - {%d} samplerate changed from %d to %d", st->iId, sta->iSampleRate, stream->m_context->sample_rate);
          sta->iSampleRate = stream->m_context->sample_rate;
          sta->changes++;
          sta->disabled = false;
        }
        break;
      }
      case STREAM_VIDEO:
      {
        CDemuxStreamVideoClient* stv = static_cast<CDemuxStreamVideoClient*>(st);
        if (stream->m_context->width != stv->iWidth &&
            stream->m_context->width != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - {%d} width changed from %d to %d", st->iId, stv->iWidth, stream->m_context->width);
          stv->iWidth = stream->m_context->width;
          stv->changes++;
          stv->disabled = false;
        }
        if (stream->m_context->height != stv->iHeight &&
            stream->m_context->height != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - {%d} height changed from %d to %d", st->iId, stv->iHeight, stream->m_context->height);
          stv->iHeight = stream->m_context->height;
          stv->changes++;
          stv->disabled = false;
        }
        break;
      }

      default:
        break;
    }
  }
  else
    CLog::Log(LOGDEBUG, "%s - parser returned error %d", __FUNCTION__, len);

  return;
}

DemuxPacket* CDVDDemuxClient::Read()
{
  if (!m_IDemux)
    return nullptr;

  DemuxPacket* pPacket = m_IDemux->ReadDemux();
  if (!pPacket)
  {
    return nullptr;
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
  else if (pPacket->iStreamId >= 0 &&
           pPacket->iStreamId < MAX_STREAMS &&
           m_streams[pPacket->iStreamId])
  {
    ParsePacket(pPacket);
  }

  return pPacket;
}

CDemuxStream* CDVDDemuxClient::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS)
    return nullptr;
  return m_streams[iStreamId];
}

void CDVDDemuxClient::RequestStreams()
{
  int nbStreams = m_IDemux->GetNrOfStreams();

  int i;
  for (i = 0; i < nbStreams; ++i)
  {
    CDemuxStream *stream = m_IDemux->GetStream(i);
    if (!stream)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStreams - invalid stream at pos %d", i);
      DisposeStreams();
      return;
    }

    if (stream->type == STREAM_AUDIO)
    {
      CDemuxStreamAudio *source = dynamic_cast<CDemuxStreamAudio*>(stream);
      if (!source)
      {
        CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStreams - invalid audio stream at pos %d", i);
        DisposeStreams();
        return;
      }
      CDemuxStreamAudioClient* st = nullptr;
      if (m_streams[i])
      {
        st = dynamic_cast<CDemuxStreamAudioClient*>(m_streams[i]);
        if (!st || (st->codec != source->codec))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamAudioClient();
        st->m_parser = av_parser_init(source->codec);
        if(st->m_parser)
          st->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      }
      st->iChannels       = source->iChannels;
      st->iSampleRate     = source->iSampleRate;
      st->iBlockAlign     = source->iBlockAlign;
      st->iBitRate        = source->iBitRate;
      st->iBitsPerSample  = source->iBitsPerSample;
      if (source->ExtraSize > 0 && source->ExtraData)
      {
        st->ExtraData = new uint8_t[source->ExtraSize];
        st->ExtraSize = source->ExtraSize;
        for (unsigned int j=0; j<source->ExtraSize; j++)
          st->ExtraData[j] = source->ExtraData[j];
      }
      m_streams[i] = st;
      st->m_parser_split = true;
      st->changes++;
    }
    else if (stream->type == STREAM_VIDEO)
    {
      CDemuxStreamVideo *source = dynamic_cast<CDemuxStreamVideo*>(stream);
      if (!source)
      {
        CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStreams - invalid video stream at pos %d", i);
        DisposeStreams();
        return;
      }
      CDemuxStreamVideoClient* st = nullptr;
      if (m_streams[i])
      {
        st = dynamic_cast<CDemuxStreamVideoClient*>(m_streams[i]);
        if (!st
            || (st->codec != source->codec)
            || (st->iWidth != source->iWidth)
            || (st->iHeight != source->iHeight))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamVideoClient();
        st->m_parser = av_parser_init(source->codec);
        if(st->m_parser)
          st->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      }
      st->iFpsScale       = source->irFpsScale;
      st->iFpsRate        = source->irFpsRate;
      st->iHeight         = source->iHeight;
      st->iWidth          = source->iWidth;
      st->fAspect         = source->fAspect;
      st->stereo_mode     = "mono";
      if (source->ExtraSize > 0 && source->ExtraData)
      {
        st->ExtraData = new uint8_t[source->ExtraSize];
        st->ExtraSize = source->ExtraSize;
        for (unsigned int j=0; j<source->ExtraSize; j++)
          st->ExtraData[j] = source->ExtraData[j];
      }
      m_streams[i] = st;
      st->m_parser_split = true;
    }
    else if (stream->type == STREAM_SUBTITLE)
    {
      CDemuxStreamSubtitle *source = dynamic_cast<CDemuxStreamSubtitle*>(stream);
      if (!source)
      {
        CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStreams - invalid subtitle stream at pos %d", i);
        DisposeStreams();
        return;
      }
      CDemuxStreamSubtitleClient* st = nullptr;
      if (m_streams[i])
      {
        st = dynamic_cast<CDemuxStreamSubtitleClient*>(m_streams[i]);
        if (!st || (st->codec != source->codec))
          DisposeStream(i);
      }
      if (!m_streams[i])
      {
        st = new CDemuxStreamSubtitleClient();
      }
      if (source->ExtraSize == 4)
      {
        st->ExtraData = new uint8_t[4];
        st->ExtraSize = 4;
        for (int j=0; j<4; j++)
          st->ExtraData[j] = source->ExtraData[j];
      }
      m_streams[i] = st;
    }
    else if (stream->type == STREAM_TELETEXT)
    {
      if (m_streams[i])
      {
        if (m_streams[i]->codec != stream->codec)
          DisposeStream(i);
      }
      if (!m_streams[i])
        m_streams[i] = new CDemuxStreamTeletext();
    }
    else if (stream->type == STREAM_RADIO_RDS)
    {
      if (m_streams[i])
      {
        if (m_streams[i]->codec != stream->codec)
          DisposeStream(i);
      }
      if (!m_streams[i])
        m_streams[i] = new CDemuxStreamRadioRDS();
    }
    else
    {
      if (m_streams[i])
        DisposeStream(i);
      m_streams[i] = new CDemuxStream();
    }

    m_streams[i]->codec = stream->codec;
    m_streams[i]->iId = i;
    m_streams[i]->iPhysicalId = stream->iPhysicalId;
    for (int j=0; j<4; j++)
      m_streams[i]->language[j] = stream->language[j];

    m_streams[i]->realtime = stream->realtime;

    CLog::Log(LOGDEBUG,"CDVDDemuxClient::RequestStreams(): added/updated stream %d:%d with codec_id %d",
        m_streams[i]->iId,
        m_streams[i]->iPhysicalId,
        m_streams[i]->codec);
  }
  // check if we need to dispose any streams no longer in props
  for (int j = i; j < MAX_STREAMS; j++)
  {
    if (m_streams[j])
    {
      CLog::Log(LOGDEBUG,"CDVDDemuxClient::RequestStreams(): disposed stream %d:%d with codec_id %d",
          m_streams[j]->iId,
          m_streams[j]->iPhysicalId,
          m_streams[j]->codec);
      DisposeStream(j);
    }
  }
}

int CDVDDemuxClient::GetNrOfStreams()
{
  int i = 0;
  while (i < MAX_STREAMS && m_streams[i])
    i++;
  return i;
}

std::string CDVDDemuxClient::GetFileName()
{
  if (m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

std::string CDVDDemuxClient::GetStreamCodecName(int iStreamId)
{
  CDemuxStream *stream = GetStream(iStreamId);
  std::string strName;
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
  return strName;
}

bool CDVDDemuxClient::SeekTime(int timems, bool backwards, double *startpts)
{
  if (m_IDemux)
  {
    return m_IDemux->SeekTime(timems, backwards, startpts);
  }
  return false;
}

void CDVDDemuxClient::SetSpeed (int speed)
{
  if (m_IDemux)
  {
    m_IDemux->SetSpeed(speed);
  }
}
