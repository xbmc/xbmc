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

#include "AEEncoderFFmpeg.h"
#include "AEUtil.h"
#include "AEPackIEC958.h"
#include <string.h>

CAEEncoderFFmpeg::CAEEncoderFFmpeg():
  m_CodecCtx(NULL),
  m_Buffer  (NULL)
{
}

CAEEncoderFFmpeg::~CAEEncoderFFmpeg()
{
  Reset();

  m_dllAvUtil.av_freep(&m_CodecCtx);
}

bool CAEEncoderFFmpeg::Initialize(AEAudioFormat &format)
{
  Reset();

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load())
    return false;

  m_dllAvCodec.avcodec_register_all();

  AVCodec *codec;
  codec = m_dllAvCodec.avcodec_find_encoder(CODEC_ID_AC3);
  if (!codec)
    return false;

  /* always assume 6 channels, 5.1... m_remap will give us what we want */
  m_CodecCtx = m_dllAvCodec.avcodec_alloc_context();
  m_CodecCtx->bit_rate       = AC3_ENCODE_BITRATE;
  m_CodecCtx->sample_rate    = format.m_sampleRate;
  m_CodecCtx->channels       = 6;
  m_CodecCtx->channel_layout = CH_LAYOUT_5POINT1_BACK;
  m_CodecCtx->sample_fmt     = SAMPLE_FMT_FLT;

  if (m_dllAvCodec.avcodec_open(m_CodecCtx, codec))
  {
    m_dllAvUtil.av_freep(&m_CodecCtx);
    return false;
  }

  /* remap the channels according to the specified channel layout */
  /* this never changes for AC3, but keep it anyway for if/when DCA encode support comes along */
  int index = 0;
  if (m_CodecCtx->channel_layout & CH_FRONT_LEFT           ) m_Layout[index++] = AE_CH_FL  ;
  if (m_CodecCtx->channel_layout & CH_FRONT_RIGHT          ) m_Layout[index++] = AE_CH_FR  ;
  if (m_CodecCtx->channel_layout & CH_FRONT_CENTER         ) m_Layout[index++] = AE_CH_FC  ;
  if (m_CodecCtx->channel_layout & CH_LOW_FREQUENCY        ) m_Layout[index++] = AE_CH_LFE ;
  if (m_CodecCtx->channel_layout & CH_BACK_LEFT            ) m_Layout[index++] = AE_CH_BL  ;
  if (m_CodecCtx->channel_layout & CH_BACK_RIGHT           ) m_Layout[index++] = AE_CH_BR  ;
  if (m_CodecCtx->channel_layout & CH_FRONT_LEFT_OF_CENTER ) m_Layout[index++] = AE_CH_FLOC;
  if (m_CodecCtx->channel_layout & CH_FRONT_RIGHT_OF_CENTER) m_Layout[index++] = AE_CH_FROC;
  if (m_CodecCtx->channel_layout & CH_BACK_CENTER          ) m_Layout[index++] = AE_CH_BC  ;
  if (m_CodecCtx->channel_layout & CH_SIDE_LEFT            ) m_Layout[index++] = AE_CH_SL  ;
  if (m_CodecCtx->channel_layout & CH_SIDE_RIGHT           ) m_Layout[index++] = AE_CH_SR  ;
  if (m_CodecCtx->channel_layout & CH_TOP_CENTER           ) m_Layout[index++] = AE_CH_TC  ;
  if (m_CodecCtx->channel_layout & CH_TOP_FRONT_LEFT       ) m_Layout[index++] = AE_CH_TFL ;
  if (m_CodecCtx->channel_layout & CH_TOP_FRONT_CENTER     ) m_Layout[index++] = AE_CH_TFC ;
  if (m_CodecCtx->channel_layout & CH_TOP_FRONT_RIGHT      ) m_Layout[index++] = AE_CH_TFR ;
  if (m_CodecCtx->channel_layout & CH_TOP_BACK_LEFT        ) m_Layout[index++] = AE_CH_TBL ;
  if (m_CodecCtx->channel_layout & CH_TOP_BACK_CENTER      ) m_Layout[index++] = AE_CH_TBC ;
  if (m_CodecCtx->channel_layout & CH_TOP_BACK_RIGHT       ) m_Layout[index++] = AE_CH_TBR ;
  m_Layout[index] = AE_CH_NULL;

  format.m_dataFormat    = AE_FMT_FLOAT;
  format.m_frames        = m_CodecCtx->frame_size / index;
  format.m_frameSamples  = index;
  format.m_frameSize     = (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3) * format.m_frameSamples;
  format.m_channelCount  = index;
  format.m_channelLayout = m_Layout;

  m_NeededFrames = format.m_frames;
  m_Buffer       = new uint8_t[std::max(FF_MIN_BUFFER_SIZE, MAX_IEC958_PACKET)];

  return true;
}

void CAEEncoderFFmpeg::Reset()
{
  m_BufferSize = 0;
}

unsigned int CAEEncoderFFmpeg::GetBitRate()
{
  return AC3_ENCODE_BITRATE;
}

CodecID CAEEncoderFFmpeg::GetCodecID()
{
  return CODEC_ID_AC3;
}

unsigned int CAEEncoderFFmpeg::GetFrames()
{
  return m_NeededFrames;
}

int CAEEncoderFFmpeg::Encode(float *data, unsigned int frames)
{
  if (frames < m_NeededFrames)
    return 0;

  /* encode it */
  int size = m_dllAvCodec.avcodec_encode_audio(m_CodecCtx, m_Buffer + IEC958_DATA_OFFSET, FF_MIN_BUFFER_SIZE, (short*)data);

  /* pack it into an IEC958 frame */
  m_BufferSize = CAEPackIEC958::PackAC3(NULL, size, m_Buffer);

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

