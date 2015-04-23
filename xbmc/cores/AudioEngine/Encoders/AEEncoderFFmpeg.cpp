/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#define AC3_ENCODE_BITRATE 640000
#define DTS_ENCODE_BITRATE 1411200

#include "cores/AudioEngine/Encoders/AEEncoderFFmpeg.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include <string.h>
#include <cassert>

CAEEncoderFFmpeg::CAEEncoderFFmpeg():
  m_BitRate       (0    ),
  m_CodecCtx      (NULL ),
  m_SwrCtx        (NULL ),
  m_BufferSize    (0    ),
  m_OutputSize    (0    ),
  m_OutputRatio   (0.0  ),
  m_SampleRateMul (0.0  ),
  m_NeededFrames  (0    ),
  m_NeedConversion(false),
  m_ResampBuffer  (NULL ),
  m_ResampBufferSize(0  )
{
}

CAEEncoderFFmpeg::~CAEEncoderFFmpeg()
{
  Reset();
  av_freep(&m_CodecCtx);
  av_freep(&m_ResampBuffer);
  if (m_SwrCtx)
    swr_free(&m_SwrCtx);
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

  bool ac3 = CSettings::Get().GetBool("audiooutput.ac3passthrough");

  AVCodec *codec = NULL;
#if 0
  /* the DCA encoder is currently useless for transcode, it creates a 196 kHz DTS-HD like mongrel which is useless for SPDIF */
  bool dts = CSettings::Get().GetBool("audiooutput.dtspassthrough");
  if (dts && (!ac3 || g_advancedSettings.m_audioTranscodeTo.Equals("dts")))
  {
    m_CodecName = "DTS";
    m_CodecID   = AV_CODEC_ID_DTS;
    m_PackFunc  = &CAEPackIEC61937::PackDTS_1024;
    m_BitRate   = DTS_ENCODE_BITRATE;
    codec = avcodec_find_encoder(m_CodecID);
  }
#endif

  /* fallback to ac3 if we support it, we might not have DTS support */
  if (!codec && ac3)
  {
    m_CodecName = "AC3";
    m_CodecID   = AV_CODEC_ID_AC3;
    m_PackFunc  = &CAEPackIEC61937::PackAC3;
    m_BitRate   = AC3_ENCODE_BITRATE;
    codec = avcodec_find_encoder(m_CodecID);
  }

  /* check we got the codec */
  if (!codec)
    return false;

  m_CodecCtx                 = avcodec_alloc_context3(codec);
  m_CodecCtx->bit_rate       = m_BitRate;
  m_CodecCtx->sample_rate    = format.m_sampleRate;
  m_CodecCtx->channel_layout = AV_CH_LAYOUT_5POINT1_BACK;

  /* select a suitable data format */
  if (codec->sample_fmts)
  {
    bool hasFloat  = false;
    bool hasDouble = false;
    bool hasS32    = false;
    bool hasS16    = false;
    bool hasU8     = false;
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
      format.m_dataFormat    = AE_FMT_FLOAT;
    }
    else if (hasFloatP)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
      format.m_dataFormat    = AE_FMT_FLOATP;
    }
    else if (hasDouble)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_DBL;
      format.m_dataFormat    = AE_FMT_DOUBLE;
    }
    else if (hasS32)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S32;
      format.m_dataFormat    = AE_FMT_S32NE;
    }
    else if (hasS16)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
      format.m_dataFormat    = AE_FMT_S16NE;
    }
    else if (hasU8)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_U8;
      format.m_dataFormat    = AE_FMT_U8;
    }
    else if (hasUnknownFormat)
    {
      m_CodecCtx->sample_fmt = codec->sample_fmts[0];
      format.m_dataFormat    = AE_FMT_FLOAT;
      m_NeedConversion       = true;
      CLog::Log(LOGNOTICE, "CAEEncoderFFmpeg::Initialize - Unknown audio format, it will be resampled.");
    }
    else
    {
      CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Initialize - Unable to find a suitable data format for the codec (%s)", m_CodecName.c_str());
      return false;
    }
  }

  m_CodecCtx->channels = BuildChannelLayout(m_CodecCtx->channel_layout, m_Layout);

  /* open the codec */
  if (avcodec_open2(m_CodecCtx, codec, NULL))
  {
    av_freep(&m_CodecCtx);
    return false;
  }

  format.m_frames        = m_CodecCtx->frame_size;
  format.m_frameSamples  = m_CodecCtx->frame_size * m_CodecCtx->channels;
  format.m_frameSize     = m_CodecCtx->channels * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  format.m_channelLayout = m_Layout;

  m_CurrentFormat = format;
  m_NeededFrames  = format.m_frames;
  m_OutputSize    = m_PackFunc(NULL, 0, m_Buffer);
  m_OutputRatio   = (double)m_NeededFrames / m_OutputSize;
  m_SampleRateMul = 1.0 / (double)m_CodecCtx->sample_rate;

  if (m_NeedConversion)
  {
    m_SwrCtx = swr_alloc_set_opts(NULL,
                      m_CodecCtx->channel_layout, m_CodecCtx->sample_fmt, m_CodecCtx->sample_rate,
                      m_CodecCtx->channel_layout, AV_SAMPLE_FMT_FLT, m_CodecCtx->sample_rate,
                      0, NULL);
    if (!m_SwrCtx || swr_init(m_SwrCtx) < 0)
    {
      CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Initialize - Failed to initialise resampler.");
      return false;
    }
  }
  CLog::Log(LOGNOTICE, "CAEEncoderFFmpeg::Initialize - %s encoder ready", m_CodecName.c_str());
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

int CAEEncoderFFmpeg::Encode(float *data, unsigned int frames)
{
  int got_output;
  AVFrame *frame;
  const uint8_t *input = (const uint8_t*) data;

  if (!m_CodecCtx || frames < m_NeededFrames)
    return 0;

  /* size of the buffer sent to the encoder: either from the input data or after
   * conversion, in all cases it is in the m_CodecCtx->sample_fmt format */
  int buf_size = av_samples_get_buffer_size(NULL, m_CodecCtx->channels, frames, m_CodecCtx->sample_fmt, 0);
  assert(buf_size>0);

  /* allocate the input frame
   * sadly, we have to alloc/dealloc it everytime since we have no guarantee the
   * data argument will be constant over iterated calls and the frame needs to
   * setup pointers inside data */
  frame = av_frame_alloc();
  if (!frame)
    return 0;

  frame->nb_samples     = m_CodecCtx->frame_size;
  frame->format         = m_CodecCtx->sample_fmt;
  frame->channel_layout = m_CodecCtx->channel_layout;

  if (m_NeedConversion)
  {
    if (!m_ResampBuffer || buf_size > m_ResampBufferSize)
    {
      m_ResampBuffer = (uint8_t*)av_realloc(m_ResampBuffer, buf_size);
      if (!m_ResampBuffer)
      {
        CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Encode - Failed to allocate %i bytes buffer for resampling", buf_size);
        av_frame_free(&frame);
        return 0;
      }
      m_ResampBufferSize = buf_size;
    }

    avcodec_fill_audio_frame(frame, m_CodecCtx->channels, m_CodecCtx->sample_fmt, m_ResampBuffer, buf_size, 0);

    /* important note: the '&input' here works because we convert from a packed
     * format (ie, interleaved). If it were to be used to convert from planar
     * formats (ie, non-interleaved, which is not currently supported by AE),
     * we would need to adapt it or it would segfault. */
    if (swr_convert(m_SwrCtx, frame->extended_data, frames, &input, frames) < 0)
    {
      CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Encode - Resampling failed");
      av_frame_free(&frame);
      return 0;
    }
  }
  else
    avcodec_fill_audio_frame(frame, m_CodecCtx->channels, m_CodecCtx->sample_fmt,
                    input, buf_size, 0);

  /* initialize the output packet */
  av_init_packet(&m_Pkt);
  m_Pkt.size      = sizeof(m_Buffer) - IEC61937_DATA_OFFSET;
  m_Pkt.data      = m_Buffer + IEC61937_DATA_OFFSET;

  /* encode it */
  int ret = avcodec_encode_audio2(m_CodecCtx, &m_Pkt, frame, &got_output);

  /* free temporary data */
  av_frame_free(&frame);

  if (ret < 0 || !got_output)
  {
    CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Encode - Encoding failed");
    return 0;
  }

  /* pack it into an IEC958 frame */
  m_BufferSize = m_PackFunc(NULL, m_Pkt.size, m_Buffer);
  if (m_BufferSize != m_OutputSize)
  {
    m_OutputSize  = m_BufferSize;
    m_OutputRatio = (double)m_NeededFrames / m_OutputSize;
  }

  /* free the packet */
  av_free_packet(&m_Pkt);

  /* return the number of frames used */
  return m_NeededFrames;
}

int CAEEncoderFFmpeg::Encode(uint8_t *in, int in_size, uint8_t *out, int out_size)
{
  int got_output;
  AVFrame *frame;

  if (!m_CodecCtx)
    return 0;

  /* allocate the input frame
   * sadly, we have to alloc/dealloc it everytime since we have no guarantee the
   * data argument will be constant over iterated calls and the frame needs to
   * setup pointers inside data */
  frame = av_frame_alloc();
  if (!frame)
    return 0;

  frame->nb_samples     = m_CodecCtx->frame_size;
  frame->format         = m_CodecCtx->sample_fmt;
  frame->channel_layout = m_CodecCtx->channel_layout;

  avcodec_fill_audio_frame(frame, m_CodecCtx->channels, m_CodecCtx->sample_fmt,
                    in, in_size, 0);

  /* initialize the output packet */
  av_init_packet(&m_Pkt);
  m_Pkt.size      = out_size - IEC61937_DATA_OFFSET;
  m_Pkt.data      = out + IEC61937_DATA_OFFSET;

  /* encode it */
  int ret = avcodec_encode_audio2(m_CodecCtx, &m_Pkt, frame, &got_output);

  /* free temporary data */
  av_frame_free(&frame);

  if (ret < 0 || !got_output)
  {
    CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Encode - Encoding failed");
    return 0;
  }

  /* pack it into an IEC958 frame */
  m_PackFunc(NULL, m_Pkt.size, out);

  /* free the packet */
  av_free_packet(&m_Pkt);

  /* return the number of frames used */
  return m_NeededFrames;
}


int CAEEncoderFFmpeg::GetData(uint8_t **data)
{
  int size;
  *data = m_Buffer;
  size  = m_BufferSize;
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

