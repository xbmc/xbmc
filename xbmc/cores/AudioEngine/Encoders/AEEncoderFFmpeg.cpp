/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#define AC3_ENCODE_BITRATE 640000
#define DTS_ENCODE_BITRATE 1411200

#include "cores/AudioEngine/Encoders/AEEncoderFFmpeg.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/FFmpeg.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

extern "C"
{
#include <libavutil/channel_layout.h>
}

#include <cassert>
#include <string.h>

using FFMPEG_HELP_TOOLS::FFMpegErrorToString;
using FFMPEG_HELP_TOOLS::FFMpegException;

CAEEncoderFFmpeg::CAEEncoderFFmpeg() : m_CodecCtx(NULL), m_SwrCtx(NULL)
{
}

CAEEncoderFFmpeg::~CAEEncoderFFmpeg()
{
  Reset();
  swr_free(&m_SwrCtx);
  av_channel_layout_uninit(&m_CodecCtx->ch_layout);
  avcodec_free_context(&m_CodecCtx);
}

bool CAEEncoderFFmpeg::IsCompatible(const AEAudioFormat& format)
{
  if (!m_CodecCtx)
    return false;

  bool match = (
    format.m_dataFormat == m_CurrentFormat.m_dataFormat &&
    format.m_sampleRate == m_CurrentFormat.m_sampleRate
  );

  if (match)
  {
    CAEChannelInfo layout;
    BuildChannelLayout(AV_CH_LAYOUT_5POINT1_BACK, layout); /* hard coded for AC3 & DTS currently */
    match = (m_CurrentFormat.m_channelLayout == layout);
  }

  return match;
}

unsigned int CAEEncoderFFmpeg::BuildChannelLayout(const int64_t ffmap, CAEChannelInfo& layout)
{
  /* build the channel layout and count the channels */
  layout.Reset();
  if (ffmap & AV_CH_FRONT_LEFT           ) layout += AE_CH_FL  ;
  if (ffmap & AV_CH_FRONT_RIGHT          ) layout += AE_CH_FR  ;
  if (ffmap & AV_CH_FRONT_CENTER         ) layout += AE_CH_FC  ;
  if (ffmap & AV_CH_LOW_FREQUENCY        ) layout += AE_CH_LFE ;
  if (ffmap & AV_CH_BACK_LEFT            ) layout += AE_CH_BL  ;
  if (ffmap & AV_CH_BACK_RIGHT           ) layout += AE_CH_BR  ;
  if (ffmap & AV_CH_FRONT_LEFT_OF_CENTER ) layout += AE_CH_FLOC;
  if (ffmap & AV_CH_FRONT_RIGHT_OF_CENTER) layout += AE_CH_FROC;
  if (ffmap & AV_CH_BACK_CENTER          ) layout += AE_CH_BC  ;
  if (ffmap & AV_CH_SIDE_LEFT            ) layout += AE_CH_SL  ;
  if (ffmap & AV_CH_SIDE_RIGHT           ) layout += AE_CH_SR  ;
  if (ffmap & AV_CH_TOP_CENTER           ) layout += AE_CH_TC  ;
  if (ffmap & AV_CH_TOP_FRONT_LEFT       ) layout += AE_CH_TFL ;
  if (ffmap & AV_CH_TOP_FRONT_CENTER     ) layout += AE_CH_TFC ;
  if (ffmap & AV_CH_TOP_FRONT_RIGHT      ) layout += AE_CH_TFR ;
  if (ffmap & AV_CH_TOP_BACK_LEFT        ) layout += AE_CH_TBL ;
  if (ffmap & AV_CH_TOP_BACK_CENTER      ) layout += AE_CH_TBC ;
  if (ffmap & AV_CH_TOP_BACK_RIGHT       ) layout += AE_CH_TBR ;

  return layout.Count();
}

bool CAEEncoderFFmpeg::Initialize(AEAudioFormat &format, bool allow_planar_input)
{
  Reset();

  bool ac3 = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH);

  const AVCodec* codec = nullptr;

  /* fallback to ac3 if we support it, we might not have DTS support */
  if (ac3)
  {
    m_CodecName = "AC3";
    m_CodecID = AV_CODEC_ID_AC3;
    m_BitRate = AC3_ENCODE_BITRATE;
    codec = avcodec_find_encoder(m_CodecID);
  }

  /* check we got the codec */
  if (!codec)
    return false;

  m_CodecCtx = avcodec_alloc_context3(codec);
  if (!m_CodecCtx)
    return false;

  m_CodecCtx->bit_rate = m_BitRate;
  m_CodecCtx->sample_rate = format.m_sampleRate;
  av_channel_layout_uninit(&m_CodecCtx->ch_layout);
  av_channel_layout_from_mask(&m_CodecCtx->ch_layout, AV_CH_LAYOUT_5POINT1_BACK);

  /* select a suitable data format */
  if (codec->sample_fmts)
  {
    bool hasFloat  = false;
    bool hasDouble = false;
    bool hasS32 = false;
    bool hasS16 = false;
    bool hasU8 = false;
    bool hasFloatP = false;
    bool hasUnknownFormat = false;

    for(int i = 0; codec->sample_fmts[i] != AV_SAMPLE_FMT_NONE; ++i)
    {
      switch (codec->sample_fmts[i])
      {
        case AV_SAMPLE_FMT_FLT: hasFloat  = true; break;
        case AV_SAMPLE_FMT_DBL: hasDouble = true; break;
        case AV_SAMPLE_FMT_S32: hasS32    = true; break;
        case AV_SAMPLE_FMT_S16: hasS16    = true; break;
        case AV_SAMPLE_FMT_U8 : hasU8     = true; break;
        case AV_SAMPLE_FMT_FLTP:
          if (allow_planar_input)
            hasFloatP  = true;
          else
            hasUnknownFormat = true;
          break;
        case AV_SAMPLE_FMT_NONE: return false;
        default: hasUnknownFormat = true; break;
      }
    }

    if (hasFloat)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_FLT;
      format.m_dataFormat = AE_FMT_FLOAT;
    }
    else if (hasFloatP)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
      format.m_dataFormat = AE_FMT_FLOATP;
    }
    else if (hasDouble)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_DBL;
      format.m_dataFormat = AE_FMT_DOUBLE;
    }
    else if (hasS32)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S32;
      format.m_dataFormat = AE_FMT_S32NE;
    }
    else if (hasS16)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
      format.m_dataFormat = AE_FMT_S16NE;
    }
    else if (hasU8)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_U8;
      format.m_dataFormat = AE_FMT_U8;
    }
    else if (hasUnknownFormat)
    {
      m_CodecCtx->sample_fmt = codec->sample_fmts[0];
      format.m_dataFormat = AE_FMT_FLOAT;
      m_NeedConversion = true;
      CLog::Log(LOGINFO,
                "CAEEncoderFFmpeg::Initialize - Unknown audio format, it will be resampled.");
    }
    else
    {
      CLog::Log(
          LOGERROR,
          "CAEEncoderFFmpeg::Initialize - Unable to find a suitable data format for the codec ({})",
          m_CodecName);
      av_channel_layout_uninit(&m_CodecCtx->ch_layout);
      avcodec_free_context(&m_CodecCtx);
      return false;
    }
  }

  uint64_t mask = m_CodecCtx->ch_layout.u.mask;
  av_channel_layout_uninit(&m_CodecCtx->ch_layout);
  av_channel_layout_from_mask(&m_CodecCtx->ch_layout, mask);
  m_CodecCtx->ch_layout.nb_channels = BuildChannelLayout(mask, m_Layout);

  /* open the codec */
  if (avcodec_open2(m_CodecCtx, codec, NULL))
  {
    av_channel_layout_uninit(&m_CodecCtx->ch_layout);
    avcodec_free_context(&m_CodecCtx);
    return false;
  }

  format.m_frames = m_CodecCtx->frame_size;
  int channels = m_CodecCtx->ch_layout.nb_channels;
  format.m_frameSize = channels * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  format.m_channelLayout = m_Layout;

  m_CurrentFormat = format;
  m_NeededFrames = format.m_frames;
  m_OutputRatio   = (double)m_NeededFrames / m_OutputSize;
  m_SampleRateMul = 1.0 / (double)m_CodecCtx->sample_rate;

  if (m_NeedConversion)
  {
    int ret = swr_alloc_set_opts2(&m_SwrCtx, &m_CodecCtx->ch_layout, m_CodecCtx->sample_fmt,
                                  m_CodecCtx->sample_rate, &m_CodecCtx->ch_layout,
                                  AV_SAMPLE_FMT_FLT, m_CodecCtx->sample_rate, 0, NULL);
    if (ret || swr_init(m_SwrCtx) < 0)
    {
      CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Initialize - Failed to initialise resampler.");
      swr_free(&m_SwrCtx);
      av_channel_layout_uninit(&m_CodecCtx->ch_layout);
      avcodec_free_context(&m_CodecCtx);
      return false;
    }
  }
  CLog::Log(LOGINFO, "CAEEncoderFFmpeg::Initialize - {} encoder ready", m_CodecName);
  return true;
}

void CAEEncoderFFmpeg::Reset()
{
  m_BufferSize = 0;
}

unsigned int CAEEncoderFFmpeg::GetBitRate()
{
  return m_BitRate;
}

AVCodecID CAEEncoderFFmpeg::GetCodecID()
{
  return m_CodecID;
}

unsigned int CAEEncoderFFmpeg::GetFrames()
{
  return m_NeededFrames;
}

int CAEEncoderFFmpeg::Encode(uint8_t *in, int in_size, uint8_t *out, int out_size)
{
  int size = 0;
  int err = AVERROR_UNKNOWN;
  AVFrame* frame = nullptr;
  AVPacket* pkt = nullptr;

  if (!m_CodecCtx)
    return size;

  try
  {
    /* allocate the input frame and output packet
     * sadly, we have to alloc/dealloc it everytime since we have no guarantee the
     * data argument will be constant over iterated calls and the frame needs to
     * setup pointers inside data */
    frame = av_frame_alloc();
    pkt = av_packet_alloc();
    if (!frame || !pkt)
      throw FFMpegException(
          "Failed to allocate \"AVFrame\" or \"AVPacket\" for encoding (error '{}')",
          strerror(errno));

    frame->nb_samples = m_CodecCtx->frame_size;
    frame->format = m_CodecCtx->sample_fmt;
    av_channel_layout_uninit(&frame->ch_layout);
    av_channel_layout_copy(&frame->ch_layout, &m_CodecCtx->ch_layout);
    int channelNum = m_CodecCtx->ch_layout.nb_channels;

    avcodec_fill_audio_frame(frame, channelNum, m_CodecCtx->sample_fmt, in, in_size, 0);

    /* encode it */
    err = avcodec_send_frame(m_CodecCtx, frame);
    if (err < 0)
      throw FFMpegException("Error sending a frame for encoding (error '{}')",
                            FFMpegErrorToString(err));

    err = avcodec_receive_packet(m_CodecCtx, pkt);
    //! @TODO: This is a workaround for our current design. The caller should be made
    // aware of the potential error values to use the ffmpeg API in a proper way, which means
    // copying with EAGAIN and multiple packet output.
    // For the current situation there is a relationship implicitely assumed of:
    // 1 frame in - 1 packet out. This holds true in practice but the API does not guarantee it.
    if (err >= 0)
    {
      if (pkt->size <= out_size)
      {
        memset(out, 0, out_size);
        memcpy(out, pkt->data, pkt->size);
        size = pkt->size;
      }
      else
      {
        CLog::LogF(LOGERROR, "Encoded pkt size ({}) is bigger than buffer ({})", pkt->size,
                   out_size);
      }
      av_packet_unref(pkt);
    }
    else
    {
      CLog::LogF(LOGERROR, "Error receiving encoded paket ({})", err);
    }
  }
  catch (const FFMpegException& caught)
  {
    CLog::Log(LOGERROR, "CAEEncoderFFmpeg::{} - {}", __func__, caught.what());
  }

  av_channel_layout_uninit(&frame->ch_layout);

  /* free temporary data */
  av_frame_free(&frame);

  /* free the packet */
  av_packet_free(&pkt);

  /* return the number of frames used */
  return size;
}

int CAEEncoderFFmpeg::GetData(uint8_t **data)
{
  int size;
  *data = m_Buffer;
  size = m_BufferSize;
  m_BufferSize = 0;
  return size;
}

double CAEEncoderFFmpeg::GetDelay(unsigned int bufferSize)
{
  if (!m_CodecCtx)
    return 0;

  int frames = m_CodecCtx->delay;
  if (m_BufferSize)
    frames += m_NeededFrames;

  return ((double)frames + ((double)bufferSize * m_OutputRatio)) * m_SampleRateMul;
}

