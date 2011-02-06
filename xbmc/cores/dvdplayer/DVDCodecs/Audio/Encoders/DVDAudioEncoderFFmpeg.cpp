/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#define AC3_ENCODE_BITRATE 640000

#include "DVDAudioEncoderFFmpeg.h"
#include "utils/log.h"
#include <string.h>

CDVDAudioEncoderFFmpeg::CDVDAudioEncoderFFmpeg():
  m_CodecCtx(NULL),
  m_AudioConvert(NULL),
  m_Buffer(NULL),
  m_TmpBuffer(NULL),
  m_TmpBuffer2(NULL)
{
}

CDVDAudioEncoderFFmpeg::~CDVDAudioEncoderFFmpeg()
{
  Reset();
  m_dllAvUtil.av_freep(&m_CodecCtx);
  if (m_AudioConvert)
    m_dllAvCodec.av_audio_convert_free(m_AudioConvert);
  delete m_Buffer;
  delete m_TmpBuffer;
  delete m_TmpBuffer2;
}

bool CDVDAudioEncoderFFmpeg::Initialize(unsigned int channels, enum PCMChannels *channelMap, unsigned int bitsPerSample, unsigned int sampleRate)
{
  Reset();
  if (!channelMap || !m_dllAvUtil.Load() || !m_dllAvCore.Load() || !m_dllAvCodec.Load())
    return false;

  m_dllAvCodec.avcodec_register_all();
  AVCodec *codec;
  codec = m_dllAvCodec.avcodec_find_encoder(CODEC_ID_AC3);
  if (!codec)
    return false;

  /* always assume 6 channels, 5.1... m_remap will give us what we want */
  m_CodecCtx = m_dllAvCodec.avcodec_alloc_context();
  m_CodecCtx->bit_rate       = AC3_ENCODE_BITRATE;
  m_CodecCtx->sample_rate    = sampleRate;
  m_CodecCtx->channels       = 6;
  m_CodecCtx->channel_layout = AV_CH_LAYOUT_5POINT1_BACK;

  switch(bitsPerSample)
  {
    case  8: m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_U8 ; break;
    case 16: m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S16; break;
    case 32: m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S32; break;
    default:
      m_dllAvUtil.av_freep(&m_CodecCtx);
      return false;
  }

  /* check if the sample format is supported by the encoder */
  if (codec->sample_fmts)
  {
    int i;
    for (i = 0; codec->sample_fmts[i] != AV_SAMPLE_FMT_NONE; i++)
      if (m_CodecCtx->sample_fmt == codec->sample_fmts[i])
        break;
    if (codec->sample_fmts[i] == AV_SAMPLE_FMT_NONE)
    {
      /* the input format was not supported, initiate conversion to the first
       * supported format */
      CLog::Log(LOGDEBUG, "CDVDAudioEncoderFFmpeg: Initializing audio conversion for encoding");
      m_AudioConvert = m_dllAvCodec.av_audio_convert_alloc(codec->sample_fmts[0], 1,
                                                           m_CodecCtx->sample_fmt, 1, NULL, 0);
      if (!m_AudioConvert)
      {
        m_dllAvUtil.av_freep(&m_CodecCtx);
        return false;
      }
      m_CodecCtx->sample_fmt = codec->sample_fmts[0];
    }
  }

  if (m_dllAvCodec.avcodec_open(m_CodecCtx, codec))
  {
    m_dllAvUtil.av_freep(&m_CodecCtx);
    return false;
  }

  /* remap the channels according to the specified channel layout */
  int index = 0;
  if (m_CodecCtx->channel_layout & AV_CH_FRONT_LEFT           ) m_ChannelMap[index++] = PCM_FRONT_LEFT           ;
  if (m_CodecCtx->channel_layout & AV_CH_FRONT_RIGHT          ) m_ChannelMap[index++] = PCM_FRONT_RIGHT          ;
  if (m_CodecCtx->channel_layout & AV_CH_FRONT_CENTER         ) m_ChannelMap[index++] = PCM_FRONT_CENTER         ;
  if (m_CodecCtx->channel_layout & AV_CH_LOW_FREQUENCY        ) m_ChannelMap[index++] = PCM_LOW_FREQUENCY        ;
  if (m_CodecCtx->channel_layout & AV_CH_BACK_LEFT            ) m_ChannelMap[index++] = PCM_BACK_LEFT            ;
  if (m_CodecCtx->channel_layout & AV_CH_BACK_RIGHT           ) m_ChannelMap[index++] = PCM_BACK_RIGHT           ;
  if (m_CodecCtx->channel_layout & AV_CH_FRONT_LEFT_OF_CENTER ) m_ChannelMap[index++] = PCM_FRONT_LEFT_OF_CENTER ;
  if (m_CodecCtx->channel_layout & AV_CH_FRONT_RIGHT_OF_CENTER) m_ChannelMap[index++] = PCM_FRONT_RIGHT_OF_CENTER;
  if (m_CodecCtx->channel_layout & AV_CH_BACK_CENTER          ) m_ChannelMap[index++] = PCM_BACK_CENTER          ;
  if (m_CodecCtx->channel_layout & AV_CH_SIDE_LEFT            ) m_ChannelMap[index++] = PCM_SIDE_LEFT            ;
  if (m_CodecCtx->channel_layout & AV_CH_SIDE_RIGHT           ) m_ChannelMap[index++] = PCM_SIDE_RIGHT           ;
  if (m_CodecCtx->channel_layout & AV_CH_TOP_CENTER           ) m_ChannelMap[index++] = PCM_TOP_CENTER           ;
  if (m_CodecCtx->channel_layout & AV_CH_TOP_FRONT_LEFT       ) m_ChannelMap[index++] = PCM_TOP_FRONT_LEFT       ;
  if (m_CodecCtx->channel_layout & AV_CH_TOP_FRONT_CENTER     ) m_ChannelMap[index++] = PCM_TOP_FRONT_CENTER     ;
  if (m_CodecCtx->channel_layout & AV_CH_TOP_FRONT_RIGHT      ) m_ChannelMap[index++] = PCM_TOP_FRONT_RIGHT      ;
  if (m_CodecCtx->channel_layout & AV_CH_TOP_BACK_LEFT        ) m_ChannelMap[index++] = PCM_TOP_BACK_LEFT        ;
  if (m_CodecCtx->channel_layout & AV_CH_TOP_BACK_CENTER      ) m_ChannelMap[index++] = PCM_TOP_BACK_CENTER      ;
  if (m_CodecCtx->channel_layout & AV_CH_TOP_BACK_RIGHT       ) m_ChannelMap[index++] = PCM_TOP_BACK_RIGHT       ;

  m_Remap.SetInputFormat (channels, channelMap, bitsPerSample / 8);
  m_Remap.SetOutputFormat(index, m_ChannelMap, true);

  if (!m_Remap.CanRemap())
  {
    m_dllAvUtil.av_freep(&m_CodecCtx);
    return false; 
  }

  m_BitsPerSample = bitsPerSample;
  m_NeededFrames = m_CodecCtx->frame_size;
  m_NeededBytes  = m_Remap.FramesToInBytes (m_NeededFrames);
  m_OutputBytes  = m_Remap.FramesToOutBytes(m_NeededFrames);
  m_Buffer       = new uint8_t[FF_MIN_BUFFER_SIZE];
  m_TmpBuffer    = new uint8_t[m_OutputBytes];

  if (m_AudioConvert)
      m_TmpBuffer2   = new uint8_t[m_NeededFrames * m_CodecCtx->channels *
                                   m_dllAvCore.av_get_bits_per_sample_fmt(m_CodecCtx->sample_fmt) / 8];

  return true;
}

void CDVDAudioEncoderFFmpeg::Reset()
{
  m_BufferSize = 0;
}

unsigned int CDVDAudioEncoderFFmpeg::GetBitRate()
{
  return AC3_ENCODE_BITRATE;
}

CodecID CDVDAudioEncoderFFmpeg::GetCodecID()
{
  return CODEC_ID_AC3;
}

unsigned int CDVDAudioEncoderFFmpeg::GetPacketSize()
{
  return m_NeededBytes;
}

int CDVDAudioEncoderFFmpeg::Encode(uint8_t *data, int size)
{
  /* remap the data and encode it in blocks */
  if (size < (int)m_NeededBytes)
    return 0;

  m_Remap.Remap(data, m_TmpBuffer, m_NeededFrames);

  if (m_AudioConvert) {
    void *convInBuf[] = { m_TmpBuffer };
    int convInStr[] = { m_BitsPerSample / 8 };
    void *convOutBuf[] = { m_TmpBuffer2 };
    int convOutStr[] = { m_dllAvCore.av_get_bits_per_sample_fmt(m_CodecCtx->sample_fmt) / 8 };
    if (m_dllAvCodec.av_audio_convert(m_AudioConvert, convOutBuf, convOutStr,
                                      convInBuf, convInStr, m_NeededFrames * m_CodecCtx->channels) < 0) {
      CLog::Log(LOGERROR, "CDVDAudioEncoderFFmpeg: Audio conversion failed");
      m_BufferSize = 0;
      return m_NeededBytes;
    }
  }

  short *inBuf = (short *)(m_AudioConvert ? m_TmpBuffer2 : m_TmpBuffer);
  m_BufferSize = m_dllAvCodec.avcodec_encode_audio(m_CodecCtx, m_Buffer, FF_MIN_BUFFER_SIZE, inBuf);
  return m_NeededBytes;
}

int CDVDAudioEncoderFFmpeg::GetData(uint8_t **data)
{
  int size;
  *data = m_Buffer;
  size  = m_BufferSize;
  m_BufferSize = 0;
  return size;
}

