/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h"

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - AV_INPUT_BUFFER_PADDING_SIZE)


class CDemuxStreamClientInternal
{
public:
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
      avcodec_free_context(&m_context);
      m_context = nullptr;
    }
  }

  AVCodecParserContext *m_parser = nullptr;
  AVCodecContext *m_context = nullptr;
  bool m_parser_split = false;
};

template <class T>
class CDemuxStreamClientInternalTpl : public CDemuxStreamClientInternal, public T
{
};

CDVDDemuxClient::CDVDDemuxClient() : CDVDDemux()
{
  m_pInput = nullptr;
  m_IDemux = nullptr;
  m_streams.clear();
}

CDVDDemuxClient::~CDVDDemuxClient()
{
  Dispose();
}

bool CDVDDemuxClient::Open(std::shared_ptr<CDVDInputStream> pInput)
{
  Abort();

  m_pInput = pInput;
  m_IDemux = std::dynamic_pointer_cast<CDVDInputStream::IDemux>(m_pInput);
  if (!m_IDemux)
    return false;

  if (!m_IDemux->OpenDemux())
    return false;

  RequestStreams();

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
  return true;
}

void CDVDDemuxClient::Dispose()
{
  DisposeStreams();

  m_pInput = nullptr;
  m_IDemux = nullptr;
}

void CDVDDemuxClient::DisposeStreams()
{
  m_streams.clear();
}

bool CDVDDemuxClient::Reset()
{
  std::shared_ptr<CDVDInputStream> pInputStream = m_pInput;
  Dispose();
  return Open(pInputStream);
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

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
}

bool CDVDDemuxClient::ParsePacket(DemuxPacket* pkt)
{
  bool change = false;

  CDemuxStream* st = GetStream(pkt->iStreamId);
  if (st == nullptr)
    return change;

  if (st->ExtraSize)
    return change;

  CDemuxStreamClientInternal* stream = dynamic_cast<CDemuxStreamClientInternal*>(st);

  if (stream == nullptr ||
     stream->m_parser == nullptr)
    return change;

  if (stream->m_context == nullptr)
  {
    AVCodec *codec = avcodec_find_decoder(st->codec);
    if (codec == nullptr)
    {
      CLog::Log(LOGERROR, "%s - can't find decoder", __FUNCTION__);
      stream->DisposeParser();
      return change;
    }

    stream->m_context = avcodec_alloc_context3(codec);
    if (stream->m_context == nullptr)
    {
      CLog::Log(LOGERROR, "%s - can't allocate context", __FUNCTION__);
      stream->DisposeParser();
      return change;
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
      st->ExtraData = new uint8_t[len+AV_INPUT_BUFFER_PADDING_SIZE];
      memcpy(st->ExtraData, pkt->pData, len);
      memset((uint8_t*)st->ExtraData + len, 0 , AV_INPUT_BUFFER_PADDING_SIZE);
      stream->m_parser_split = false;
      change = true;
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - split extradata");
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
  if (len >= 0)
  {
    if (stream->m_context->profile != st->profile &&
        stream->m_context->profile != FF_PROFILE_UNKNOWN)
    {
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) profile changed from {} to {}", st->uniqueId, st->profile, stream->m_context->profile);
      st->profile = stream->m_context->profile;
      st->changes++;
      st->disabled = false;
    }

    if (stream->m_context->level != st->level &&
        stream->m_context->level != FF_LEVEL_UNKNOWN)
    {
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) level changed from {} to {}", st->uniqueId, st->level, stream->m_context->level);
      st->level = stream->m_context->level;
      st->changes++;
      st->disabled = false;
    }

    switch (st->type)
    {
      case STREAM_AUDIO:
      {
        CDemuxStreamClientInternalTpl<CDemuxStreamAudio>* sta = static_cast<CDemuxStreamClientInternalTpl<CDemuxStreamAudio>*>(st);
        if (stream->m_context->channels != sta->iChannels &&
            stream->m_context->channels != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) channels changed from {} to {}", st->uniqueId, sta->iChannels, stream->m_context->channels);
          sta->iChannels = stream->m_context->channels;
          sta->changes++;
          sta->disabled = false;
        }
        if (stream->m_context->sample_rate != sta->iSampleRate &&
            stream->m_context->sample_rate != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) samplerate changed from {} to {}", st->uniqueId, sta->iSampleRate, stream->m_context->sample_rate);
          sta->iSampleRate = stream->m_context->sample_rate;
          sta->changes++;
          sta->disabled = false;
        }
        break;
      }
      case STREAM_VIDEO:
      {
        CDemuxStreamClientInternalTpl<CDemuxStreamVideo>* stv = static_cast<CDemuxStreamClientInternalTpl<CDemuxStreamVideo>*>(st);
        if (stream->m_context->width != stv->iWidth &&
            stream->m_context->width != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) width changed from {} to {}", st->uniqueId, stv->iWidth, stream->m_context->width);
          stv->iWidth = stream->m_context->width;
          stv->changes++;
          stv->disabled = false;
        }
        if (stream->m_context->height != stv->iHeight &&
            stream->m_context->height != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) height changed from {} to {}", st->uniqueId, stv->iHeight, stream->m_context->height);
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

  return change;
}

DemuxPacket* CDVDDemuxClient::Read()
{
  if (!m_IDemux)
    return nullptr;

  if (m_packet)
    return m_packet.release();

  m_packet.reset(m_IDemux->ReadDemux());
  if (!m_packet)
  {
    return nullptr;
  }

  if (m_packet->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    RequestStreams();
    CDVDDemuxUtils::FreeDemuxPacket(m_packet.release());
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }
  else if (m_packet->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    RequestStreams();
  }
  else if (m_packet->iStreamId >= 0 && m_streams.count(m_packet->iStreamId) > 0)
  {
    if (ParsePacket(m_packet.get()))
    {
      RequestStreams();
      DemuxPacket *pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);
      pPacket->iStreamId = DMX_SPECIALID_STREAMCHANGE;
      pPacket->demuxerId = m_demuxerId;
      return pPacket;
    }
  }

  CDVDInputStream::IDisplayTime *inputStream = m_pInput->GetIDisplayTime();
  if (inputStream)
  {
    int dispTime = inputStream->GetTime();
    if (m_displayTime != dispTime)
    {
      m_displayTime = dispTime;
      if (m_packet->dts != DVD_NOPTS_VALUE)
      {
        m_dtsAtDisplayTime = m_packet->dts;
      }
    }
    if (m_dtsAtDisplayTime != DVD_NOPTS_VALUE && m_packet->dts != DVD_NOPTS_VALUE)
    {
      m_packet->dispTime = m_displayTime;
      m_packet->dispTime += DVD_TIME_TO_MSEC(m_packet->dts - m_dtsAtDisplayTime);
    }
  }
  return m_packet.release();
}

CDemuxStream* CDVDDemuxClient::GetStream(int iStreamId) const
{
  auto stream = m_streams.find(iStreamId);
  if (stream == m_streams.end())
    return nullptr;

  return stream->second.get();
}

std::vector<CDemuxStream*> CDVDDemuxClient::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  for (auto &st : m_streams)
    streams.push_back(st.second.get());

  return streams;
}

void CDVDDemuxClient::RequestStreams()
{
  std::map<int, std::shared_ptr<CDemuxStream>> newStreamMap;
  for (auto stream : m_IDemux->GetStreams())
    SetStreamProps(stream, newStreamMap, false);
  m_streams = newStreamMap;
}

void CDVDDemuxClient::SetStreamProps(CDemuxStream *stream, std::map<int, std::shared_ptr<CDemuxStream>> &map, bool forceInit)
{
  if (!stream)
  {
    CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid stream");
    DisposeStreams();
    return;
  }

  std::shared_ptr<CDemuxStream> currentStream(GetStreamInternal(stream->uniqueId));
  std::shared_ptr<CDemuxStream> toStream;

  if (stream->type == STREAM_AUDIO)
  {
    CDemuxStreamAudio *source = dynamic_cast<CDemuxStreamAudio*>(stream);
    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid audio stream with id %d", stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamAudio>> streamAudio;
    if (currentStream)
      streamAudio = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamAudio>>(currentStream);
    if (forceInit || !streamAudio || streamAudio->codec != source->codec)
    {
      streamAudio.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamAudio>());
      streamAudio->m_parser = av_parser_init(source->codec);
      if (streamAudio->m_parser)
        streamAudio->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      streamAudio->iSampleRate = source->iSampleRate;
      streamAudio->iChannels = source->iChannels;
    }

    streamAudio->iBlockAlign     = source->iBlockAlign;
    streamAudio->iBitRate        = source->iBitRate;
    streamAudio->iBitsPerSample  = source->iBitsPerSample;
    if (source->ExtraSize > 0 && source->ExtraData)
    {
      delete[] streamAudio->ExtraData;
      streamAudio->ExtraData = new uint8_t[source->ExtraSize];
      streamAudio->ExtraSize = source->ExtraSize;
      for (unsigned int j=0; j<source->ExtraSize; j++)
        streamAudio->ExtraData[j] = source->ExtraData[j];
    }
    streamAudio->m_parser_split = true;
    streamAudio->changes++;
    map[stream->uniqueId] = streamAudio;
    toStream = streamAudio;
  }
  else if (stream->type == STREAM_VIDEO)
  {
    CDemuxStreamVideo *source = dynamic_cast<CDemuxStreamVideo*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid video stream with id %d", stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamVideo>> streamVideo;
    if (currentStream)
      streamVideo = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamVideo>>(currentStream);
    if (forceInit || !streamVideo || streamVideo->codec != source->codec)
    {
      streamVideo.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamVideo>());
      streamVideo->m_parser = av_parser_init(source->codec);
      if (streamVideo->m_parser)
        streamVideo->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      streamVideo->iHeight = source->iHeight;
      streamVideo->iWidth = source->iWidth;
    }

    streamVideo->iFpsScale       = source->iFpsScale;
    streamVideo->iFpsRate        = source->iFpsRate;
    streamVideo->fAspect         = source->fAspect;
    streamVideo->iBitRate = source->iBitRate;
    if (source->ExtraSize > 0 && source->ExtraData)
    {
      delete[] streamVideo->ExtraData;
      streamVideo->ExtraData = new uint8_t[source->ExtraSize];
      streamVideo->ExtraSize = source->ExtraSize;
      for (unsigned int j=0; j<source->ExtraSize; j++)
        streamVideo->ExtraData[j] = source->ExtraData[j];
    }
    streamVideo->m_parser_split = true;
    streamVideo->changes++;
    map[stream->uniqueId] = streamVideo;
    toStream = streamVideo;
  }
  else if (stream->type == STREAM_SUBTITLE)
  {
    CDemuxStreamSubtitle *source = dynamic_cast<CDemuxStreamSubtitle*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid subtitle stream with id %d", stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamSubtitle>> streamSubtitle;
    if (currentStream)
      streamSubtitle = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamSubtitle>>(currentStream);
    if (!streamSubtitle || streamSubtitle->codec != source->codec)
    {
      streamSubtitle.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamSubtitle>());
      streamSubtitle->m_parser = av_parser_init(source->codec);
      if (streamSubtitle->m_parser)
        streamSubtitle->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
    }

    if (source->ExtraSize == 4)
    {
      delete[] streamSubtitle->ExtraData;
      streamSubtitle->ExtraData = new uint8_t[4];
      streamSubtitle->ExtraSize = 4;
      for (int j=0; j<4; j++)
        streamSubtitle->ExtraData[j] = source->ExtraData[j];
    }
    map[stream->uniqueId] = streamSubtitle;
    toStream = streamSubtitle;
  }
  else if (stream->type == STREAM_TELETEXT)
  {
    CDemuxStreamTeletext *source = dynamic_cast<CDemuxStreamTeletext*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid teletext stream with id %d", stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamTeletext>> streamTeletext;
    if (currentStream)
       streamTeletext = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamTeletext>>(currentStream);
    if (!streamTeletext || streamTeletext->codec != source->codec)
    {
      streamTeletext.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamTeletext>());
    }

    map[stream->uniqueId] = streamTeletext;
    toStream = streamTeletext;
  }
  else if (stream->type == STREAM_RADIO_RDS)
  {
    CDemuxStreamRadioRDS *source = dynamic_cast<CDemuxStreamRadioRDS*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid radio-rds stream with id %d", stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamRadioRDS>> streamRDS;
    if (currentStream)
      streamRDS = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamRadioRDS>>(currentStream);
    if (!streamRDS || streamRDS->codec != source->codec)
    {
      streamRDS.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamRadioRDS>());
    }

    map[stream->uniqueId] = streamRDS;
    toStream = streamRDS;
  }
  else
  {
    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStream>> streamGen;
    streamGen = std::make_shared<CDemuxStreamClientInternalTpl<CDemuxStream>>();
    map[stream->uniqueId] = streamGen;
    toStream = streamGen;
  }

  // only update profile / level if we create a new stream
  // existing streams may be corrected by ParsePacket
  if (!currentStream)
  {
    toStream->profile = stream->profile;
    toStream->level = stream->level;
  }

  toStream->uniqueId = stream->uniqueId;
  toStream->codec = stream->codec;
  toStream->codecName = stream->codecName;
  toStream->flags = stream->flags;
  toStream->cryptoSession = stream->cryptoSession;
  toStream->externalInterfaces = stream->externalInterfaces;
  toStream->language = stream->language;

  CLog::Log(LOGDEBUG,"CDVDDemuxClient::RequestStream(): added/updated stream %d with codec_id %d",
      toStream->uniqueId,
      toStream->codec);
}

std::shared_ptr<CDemuxStream> CDVDDemuxClient::GetStreamInternal(int iStreamId)
{
  auto stream = m_streams.find(iStreamId);
  if (stream != m_streams.end())
  {
    return stream->second;
  }
  else
    return nullptr;
}

int CDVDDemuxClient::GetNrOfStreams() const
{
  return m_streams.size();
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

bool CDVDDemuxClient::SeekTime(double timems, bool backwards, double *startpts)
{
  if (m_IDemux)
  {
    m_displayTime = 0;
    m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
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

void CDVDDemuxClient::EnableStream(int id, bool enable)
{
  if (m_IDemux)
  {
    m_IDemux->EnableStream(id, enable);
  }
}

void CDVDDemuxClient::OpenStream(int id)
{
  // OpenStream may change some parameters
  // in this case we need to reset our stream properties
  if (m_IDemux && m_IDemux->OpenStream(id))
  {
    SetStreamProps(m_IDemux->GetStream(id), m_streams, true);
  }
}

void CDVDDemuxClient::SetVideoResolution(int width, int height)
{
  if (m_IDemux)
  {
    m_IDemux->SetVideoResolution(width, height);
  }
}
