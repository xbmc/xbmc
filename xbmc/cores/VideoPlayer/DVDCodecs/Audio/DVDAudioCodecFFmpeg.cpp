/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDAudioCodecFFmpeg.h"

#include "../../DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/FFmpeg.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

extern "C" {
#include <libavutil/opt.h>
}

CDVDAudioCodecFFmpeg::CDVDAudioCodecFFmpeg(CProcessInfo &processInfo) : CDVDAudioCodec(processInfo)
{
  m_pCodecContext = NULL;

  m_channels = 0;
  m_layout = 0;

  m_pFrame = nullptr;
  m_eof = false;
}

CDVDAudioCodecFFmpeg::~CDVDAudioCodecFFmpeg()
{
  Dispose();
}

bool CDVDAudioCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (hints.cryptoSession)
  {
    CLog::Log(LOGERROR,"CDVDAudioCodecFFmpeg::Open() CryptoSessions unsupported!");
    return false;
  }

  const AVCodec* pCodec = nullptr;
  bool allowdtshddecode = true;

  // set any special options
  for(std::vector<CDVDCodecOption>::iterator it = options.m_keys.begin(); it != options.m_keys.end(); ++it)
    if (it->m_name == "allowdtshddecode")
      allowdtshddecode = atoi(it->m_value.c_str()) != 0;

  if (hints.codec == AV_CODEC_ID_DTS && allowdtshddecode)
    pCodec = avcodec_find_decoder_by_name("dcadec");

  if (!pCodec)
    pCodec = avcodec_find_decoder(hints.codec);

  if (!pCodec)
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecFFmpeg::Open() Unable to find codec {}", hints.codec);
    return false;
  }

  m_pCodecContext = avcodec_alloc_context3(pCodec);
  if (!m_pCodecContext)
    return false;

  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = 1;

#if LIBAVCODEC_VERSION_MAJOR < 60
  if (pCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
    m_pCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;
#endif

  m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
  m_channels = 0;
  av_channel_layout_uninit(&m_pCodecContext->ch_layout);

  if (hints.channels > 0 && hints.channellayout > 0)
  {
    m_pCodecContext->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
    m_pCodecContext->ch_layout.nb_channels = hints.channels;
    m_pCodecContext->ch_layout.u.mask = hints.channellayout;
  }
  else if (hints.channels > 0)
  {
    av_channel_layout_default(&m_pCodecContext->ch_layout, hints.channels);
  }

  m_hint_layout = m_pCodecContext->ch_layout.u.mask;

  m_pCodecContext->sample_rate = hints.samplerate;
  m_pCodecContext->block_align = hints.blockalign;
  m_pCodecContext->bit_rate = hints.bitrate;
  m_pCodecContext->bits_per_coded_sample = hints.bitspersample;

  if(m_pCodecContext->bits_per_coded_sample == 0)
    m_pCodecContext->bits_per_coded_sample = 16;

  if (hints.extradata)
  {
    m_pCodecContext->extradata =
        (uint8_t*)av_mallocz(hints.extradata.GetSize() + AV_INPUT_BUFFER_PADDING_SIZE);
    if(m_pCodecContext->extradata)
    {
      m_pCodecContext->extradata_size = hints.extradata.GetSize();
      memcpy(m_pCodecContext->extradata, hints.extradata.GetData(), hints.extradata.GetSize());
    }
  }

  float applyDrc = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_audioApplyDrc;
  if (applyDrc >= 0.0f)
    av_opt_set_double(m_pCodecContext, "drc_scale", static_cast<double>(applyDrc),
                      AV_OPT_SEARCH_CHILDREN);

  if (avcodec_open2(m_pCodecContext, pCodec, NULL) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDAudioCodecFFmpeg::Open() Unable to open codec");
    Dispose();
    return false;
  }

  m_pFrame = av_frame_alloc();
  if (!m_pFrame)
  {
    Dispose();
    return false;
  }

  m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
  m_hasDownmix = false;

  m_codecName = "ff-" + std::string(m_pCodecContext->codec->name);

  CLog::Log(LOGINFO, "CDVDAudioCodecFFmpeg::Open() Successful opened audio decoder {}",
            m_pCodecContext->codec->name);

  return true;
}

void CDVDAudioCodecFFmpeg::Dispose()
{
  av_frame_free(&m_pFrame);
  avcodec_free_context(&m_pCodecContext);
}

bool CDVDAudioCodecFFmpeg::AddData(const DemuxPacket &packet)
{
  if (!m_pCodecContext)
    return false;

  if (m_eof)
  {
    Reset();
  }

  AVPacket* avpkt = av_packet_alloc();
  if (!avpkt)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecFFmpeg::{} - av_packet_alloc failed: {}", __FUNCTION__,
              strerror(errno));
    return false;
  }

  avpkt->data = packet.pData;
  avpkt->size = packet.iSize;
  avpkt->dts = (packet.dts == DVD_NOPTS_VALUE)
                   ? AV_NOPTS_VALUE
                   : static_cast<int64_t>(packet.dts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt->pts = (packet.pts == DVD_NOPTS_VALUE)
                   ? AV_NOPTS_VALUE
                   : static_cast<int64_t>(packet.pts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt->side_data = static_cast<AVPacketSideData*>(packet.pSideData);
  avpkt->side_data_elems = packet.iSideDataElems;

  int ret = avcodec_send_packet(m_pCodecContext, avpkt);

  //! @todo: properly handle avpkt side_data. this works around our improper use of the side_data
  // as we pass pointers to ffmpeg allocated memory for the side_data. we should really be allocating
  // and storing our own AVPacket. This will require some extensive changes.
  av_buffer_unref(&avpkt->buf);
  av_free(avpkt);

  // try again
  if (ret == AVERROR(EAGAIN))
  {
    return false;
  }

  return true;
}

void CDVDAudioCodecFFmpeg::GetData(DVDAudioFrame &frame)
{
  frame.nb_frames = 0;

  uint8_t* data[16]{};
  int bytes = GetData(data);
  if (!bytes)
  {
    return;
  }

  frame.passthrough = false;
  frame.format.m_dataFormat = m_format.m_dataFormat;
  frame.format.m_channelLayout = m_format.m_channelLayout;
  frame.framesize = (CAEUtil::DataFormatToBits(frame.format.m_dataFormat) >> 3) * frame.format.m_channelLayout.Count();
  if(frame.framesize == 0)
    return;

  frame.nb_frames = bytes/frame.framesize;
  frame.framesOut = 0;
  frame.planes = AE_IS_PLANAR(frame.format.m_dataFormat) ? frame.format.m_channelLayout.Count() : 1;

  for (unsigned int i=0; i<frame.planes; i++)
    frame.data[i] = data[i];

  frame.bits_per_sample = CAEUtil::DataFormatToBits(frame.format.m_dataFormat);
  frame.format.m_sampleRate = m_format.m_sampleRate;
  frame.matrix_encoding = GetMatrixEncoding();
  frame.audio_service_type = GetAudioServiceType();
  frame.profile = GetProfile();
  // compute duration.
  if (frame.format.m_sampleRate)
    frame.duration = ((double)frame.nb_frames * DVD_TIME_BASE) / frame.format.m_sampleRate;
  else
    frame.duration = 0.0;

  int64_t bpts = m_pFrame->best_effort_timestamp;
  if(bpts != AV_NOPTS_VALUE)
    frame.pts = (double)bpts * DVD_TIME_BASE / AV_TIME_BASE;
  else
    frame.pts = DVD_NOPTS_VALUE;

  frame.hasDownmix = m_hasDownmix;
  if (frame.hasDownmix)
  {
    frame.centerMixLevel = m_downmixInfo.center_mix_level;
  }
}

int CDVDAudioCodecFFmpeg::GetData(uint8_t** dst)
{
  int ret = avcodec_receive_frame(m_pCodecContext, m_pFrame);
  if (!ret)
  {
    if (m_pFrame->nb_side_data)
    {
      for (int i = 0; i < m_pFrame->nb_side_data; i++)
      {
        AVFrameSideData *sd = m_pFrame->side_data[i];
        if (sd->data)
        {
          if (sd->type == AV_FRAME_DATA_MATRIXENCODING)
          {
            m_matrixEncoding = *(enum AVMatrixEncoding*)sd->data;
          }
          else if (sd->type == AV_FRAME_DATA_DOWNMIX_INFO)
          {
            m_downmixInfo = *(AVDownmixInfo*)sd->data;
            m_hasDownmix = true;
          }
        }
      }
    }

    m_format.m_dataFormat = GetDataFormat();
    m_format.m_channelLayout = GetChannelMap();
    m_format.m_sampleRate = GetSampleRate();
    m_format.m_frameSize = m_format.m_channelLayout.Count() *
                           CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3;

    int channels = m_pFrame->ch_layout.nb_channels;
    int planes = av_sample_fmt_is_planar(m_pCodecContext->sample_fmt) ? channels : 1;

    for (int i=0; i<planes; i++)
      dst[i] = m_pFrame->extended_data[i];

    return m_pFrame->nb_samples * channels * av_get_bytes_per_sample(m_pCodecContext->sample_fmt);
  }

  return 0;
}

void CDVDAudioCodecFFmpeg::Reset()
{
  if (m_pCodecContext) avcodec_flush_buffers(m_pCodecContext);
  m_eof = false;
}

int CDVDAudioCodecFFmpeg::GetChannels()
{
  return m_pCodecContext->ch_layout.nb_channels;
}

int CDVDAudioCodecFFmpeg::GetSampleRate()
{
  if (m_pCodecContext)
    return m_pCodecContext->sample_rate;
  return 0;
}

enum AEDataFormat CDVDAudioCodecFFmpeg::GetDataFormat()
{
  switch(m_pCodecContext->sample_fmt)
  {
    case AV_SAMPLE_FMT_U8 : return AE_FMT_U8;
    case AV_SAMPLE_FMT_U8P : return AE_FMT_U8P;
    case AV_SAMPLE_FMT_S16: return AE_FMT_S16NE;
    case AV_SAMPLE_FMT_S16P: return AE_FMT_S16NEP;
    case AV_SAMPLE_FMT_S32: return AE_FMT_S32NE;
    case AV_SAMPLE_FMT_S32P: return AE_FMT_S32NEP;
    case AV_SAMPLE_FMT_FLT: return AE_FMT_FLOAT;
    case AV_SAMPLE_FMT_FLTP: return AE_FMT_FLOATP;
    case AV_SAMPLE_FMT_DBL: return AE_FMT_DOUBLE;
    case AV_SAMPLE_FMT_DBLP: return AE_FMT_DOUBLEP;
    case AV_SAMPLE_FMT_NONE:
    default:
      CLog::Log(LOGERROR, "CDVDAudioCodecFFmpeg::GetDataFormat - invalid data format");
      return AE_FMT_INVALID;
  }
}

int CDVDAudioCodecFFmpeg::GetBitRate()
{
  if (m_pCodecContext)
    return static_cast<int>(m_pCodecContext->bit_rate);
  return 0;
}

enum AVMatrixEncoding CDVDAudioCodecFFmpeg::GetMatrixEncoding()
{
  return m_matrixEncoding;
}

enum AVAudioServiceType CDVDAudioCodecFFmpeg::GetAudioServiceType()
{
  if (m_pCodecContext)
    return m_pCodecContext->audio_service_type;
  return AV_AUDIO_SERVICE_TYPE_MAIN;
}

int CDVDAudioCodecFFmpeg::GetProfile()
{
  if (m_pCodecContext)
    return m_pCodecContext->profile;
  return 0;
}

static unsigned count_bits(int64_t value)
{
  unsigned bits = 0;
  for(;value;++bits)
    value &= value - 1;
  return bits;
}

void CDVDAudioCodecFFmpeg::BuildChannelMap()
{
  int codecChannels = m_pCodecContext->ch_layout.nb_channels;
  uint64_t codecChannelLayout = m_pCodecContext->ch_layout.u.mask;
  if (m_channels == codecChannels && m_layout == codecChannelLayout)
    return; //nothing to do here

  m_channels = codecChannels;
  m_layout = codecChannelLayout;

  int64_t layout;

  int bits = count_bits(codecChannelLayout);
  if (bits == codecChannels)
    layout = codecChannelLayout;
  else
  {
    CLog::Log(LOGINFO,
              "CDVDAudioCodecFFmpeg::GetChannelMap - FFmpeg reported {} channels, but the layout "
              "contains {} - trying hints",
              codecChannels, bits);
    if (static_cast<int>(count_bits(m_hint_layout)) == codecChannels)
      layout = m_hint_layout;
    else
    {
      AVChannelLayout def_layout = {};
      av_channel_layout_default(&def_layout, codecChannels);
      layout = def_layout.u.mask;
      av_channel_layout_uninit(&def_layout);
      CLog::Log(LOGINFO, "Using default layout...");
    }
  }

  m_channelLayout.Reset();

  if (layout & AV_CH_FRONT_LEFT           ) m_channelLayout += AE_CH_FL  ;
  if (layout & AV_CH_FRONT_RIGHT          ) m_channelLayout += AE_CH_FR  ;
  if (layout & AV_CH_FRONT_CENTER         ) m_channelLayout += AE_CH_FC  ;
  if (layout & AV_CH_LOW_FREQUENCY        ) m_channelLayout += AE_CH_LFE ;
  if (layout & AV_CH_BACK_LEFT            ) m_channelLayout += AE_CH_BL  ;
  if (layout & AV_CH_BACK_RIGHT           ) m_channelLayout += AE_CH_BR  ;
  if (layout & AV_CH_FRONT_LEFT_OF_CENTER ) m_channelLayout += AE_CH_FLOC;
  if (layout & AV_CH_FRONT_RIGHT_OF_CENTER) m_channelLayout += AE_CH_FROC;
  if (layout & AV_CH_BACK_CENTER          ) m_channelLayout += AE_CH_BC  ;
  if (layout & AV_CH_SIDE_LEFT            ) m_channelLayout += AE_CH_SL  ;
  if (layout & AV_CH_SIDE_RIGHT           ) m_channelLayout += AE_CH_SR  ;
  if (layout & AV_CH_TOP_CENTER           ) m_channelLayout += AE_CH_TC  ;
  if (layout & AV_CH_TOP_FRONT_LEFT       ) m_channelLayout += AE_CH_TFL ;
  if (layout & AV_CH_TOP_FRONT_CENTER     ) m_channelLayout += AE_CH_TFC ;
  if (layout & AV_CH_TOP_FRONT_RIGHT      ) m_channelLayout += AE_CH_TFR ;
  if (layout & AV_CH_TOP_BACK_LEFT        ) m_channelLayout += AE_CH_BL  ;
  if (layout & AV_CH_TOP_BACK_CENTER      ) m_channelLayout += AE_CH_BC  ;
  if (layout & AV_CH_TOP_BACK_RIGHT       ) m_channelLayout += AE_CH_BR  ;

  m_channels = codecChannels;
}

CAEChannelInfo CDVDAudioCodecFFmpeg::GetChannelMap()
{
  BuildChannelMap();
  return m_channelLayout;
}
