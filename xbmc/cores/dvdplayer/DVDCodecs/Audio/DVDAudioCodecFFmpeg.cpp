/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "DVDAudioCodecFFmpeg.h"
#ifdef _LINUX
#include "XMemUtils.h"
#endif
#include "../../DVDStreamInfo.h"
#include "utils/log.h"
#include "GUISettings.h"

CDVDAudioCodecFFmpeg::CDVDAudioCodecFFmpeg() : CDVDAudioCodec()
{
  m_iBufferSize1 = 0;
  m_pBuffer1     = (BYTE*)_aligned_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE, 16);
  memset(m_pBuffer1, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);

  m_iBuffered = 0;
  m_pCodecContext = NULL;
  m_pConvert = NULL;
  m_bOpenedCodec = false;

  m_channelLayout[0] = PCM_INVALID;
  m_channels = 0;
  m_layout = 0;
}

CDVDAudioCodecFFmpeg::~CDVDAudioCodecFFmpeg()
{
  _aligned_free(m_pBuffer1);
  Dispose();
}

bool CDVDAudioCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  AVCodec* pCodec;
  m_bOpenedCodec = false;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load())
    return false;

  m_dllAvCodec.avcodec_register_all();
  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();
  m_dllAvCodec.avcodec_get_context_defaults(m_pCodecContext);

  pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);
  if (!pCodec)
  {
    CLog::Log(LOGDEBUG,"CDVDAudioCodecFFmpeg::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = 1;

  if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
    m_pCodecContext->flags |= CODEC_FLAG_TRUNCATED;

  m_channels = 0;
  m_pCodecContext->channels = hints.channels;
  m_pCodecContext->sample_rate = hints.samplerate;
  m_pCodecContext->block_align = hints.blockalign;
  m_pCodecContext->bit_rate = hints.bitrate;
  m_pCodecContext->bits_per_coded_sample = hints.bitspersample;

  if(m_pCodecContext->bits_per_coded_sample == 0)
    m_pCodecContext->bits_per_coded_sample = 16;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  // set acceleration
  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;

  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDAudioCodecFFmpeg::Open() Unable to open codec");
    Dispose();
    return false;
  }

  m_bOpenedCodec = true;
  return true;
}

void CDVDAudioCodecFFmpeg::Dispose()
{
  if (m_pCodecContext)
  {
    if (m_bOpenedCodec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    m_bOpenedCodec = false;
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();

  m_iBufferSize1 = 0;
  m_iBuffered = 0;
}

int CDVDAudioCodecFFmpeg::Decode(BYTE* pData, int iSize)
{
  int iBytesUsed;
  if (!m_pCodecContext) return -1;

  m_iBufferSize1 = AVCODEC_MAX_AUDIO_FRAME_SIZE ;
  iBytesUsed = m_dllAvCodec.avcodec_decode_audio2( m_pCodecContext
                                                 , (int16_t*)m_pBuffer1
                                                 , &m_iBufferSize1
                                                 , pData
                                                 , iSize);

  /* some codecs will attempt to consume more data than what we gave */
  if (iBytesUsed > iSize)
  {
    CLog::Log(LOGWARNING, "CDVDAudioCodecFFmpeg::Decode - decoder attempted to consume more data than given");
    iBytesUsed = iSize;
  }

  if(m_iBufferSize1 == 0 && iBytesUsed >= 0)
    m_iBuffered += iBytesUsed;
  else
    m_iBuffered = 0;

  return iBytesUsed;
}

int CDVDAudioCodecFFmpeg::GetData(BYTE** dst)
{
  if(m_iBufferSize1)
  {
    *dst = m_pBuffer1;
    return m_iBufferSize1;
  }
  return 0;
}

void CDVDAudioCodecFFmpeg::Reset()
{
  if (m_pCodecContext) m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);
  m_iBufferSize1 = 0;
  m_iBuffered = 0;
}

int CDVDAudioCodecFFmpeg::GetChannels()
{
  return m_pCodecContext->channels;
}

int CDVDAudioCodecFFmpeg::GetSampleRate()
{
  if (m_pCodecContext) return m_pCodecContext->sample_rate;
  return 0;
}

enum AEDataFormat CDVDAudioCodecFFmpeg::GetDataFormat()
{
  switch(m_pCodecContext->sample_fmt)
  {
    case SAMPLE_FMT_U8 : return AE_FMT_U8;
    case SAMPLE_FMT_S16: return AE_FMT_S16NE;
    case SAMPLE_FMT_S32: return AE_FMT_S32NE;
    case SAMPLE_FMT_FLT: return AE_FMT_FLOAT;
    case SAMPLE_FMT_DBL: return AE_FMT_DOUBLE;
    default:
      assert(false);
  }
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
  int64_t layout;

  int bits = count_bits(m_pCodecContext->channel_layout);
  if (bits == m_pCodecContext->channels)
    layout = m_pCodecContext->channel_layout;
  else
  {
    CLog::Log(LOGINFO, "CDVDAudioCodecFFmpeg::GetChannelMap - FFmpeg reported %d channels, but the layout contains %d ignoring", m_pCodecContext->channels, bits);
    layout = m_dllAvCodec.avcodec_guess_channel_layout(m_pCodecContext->channels, m_pCodecContext->codec_id, NULL);
  }

  index = 0;
  if (layout & CH_FRONT_LEFT           ) m_channelLayout[index++] = AE_CH_FL  ;
  if (layout & CH_FRONT_RIGHT          ) m_channelLayout[index++] = AE_CH_FR  ;
  if (layout & CH_FRONT_CENTER         ) m_channelLayout[index++] = AE_CH_FC  ;
  if (layout & CH_LOW_FREQUENCY        ) m_channelLayout[index++] = AE_CH_LFE ;
  if (layout & CH_BACK_LEFT            ) m_channelLayout[index++] = AE_CH_BL  ;
  if (layout & CH_BACK_RIGHT           ) m_channelLayout[index++] = AE_CH_BR  ;
  if (layout & CH_FRONT_LEFT_OF_CENTER ) m_channelLayout[index++] = AE_CH_FLOC;
  if (layout & CH_FRONT_RIGHT_OF_CENTER) m_channelLayout[index++] = AE_CH_FROC;
  if (layout & CH_BACK_CENTER          ) m_channelLayout[index++] = AE_CH_BC  ;
  if (layout & CH_SIDE_LEFT            ) m_channelLayout[index++] = AE_CH_SL  ;
  if (layout & CH_SIDE_RIGHT           ) m_channelLayout[index++] = AE_CH_SR  ;
  if (layout & CH_TOP_CENTER           ) m_channelLayout[index++] = AE_CH_TC  ;
  if (layout & CH_TOP_FRONT_LEFT       ) m_channelLayout[index++] = AE_CH_TFL ;
  if (layout & CH_TOP_FRONT_CENTER     ) m_channelLayout[index++] = AE_CH_TFC ;
  if (layout & CH_TOP_FRONT_RIGHT      ) m_channelLayout[index++] = AE_CH_TFR ;
  if (layout & CH_TOP_BACK_LEFT        ) m_channelLayout[index++] = AE_CH_BL  ;
  if (layout & CH_TOP_BACK_CENTER      ) m_channelLayout[index++] = AE_CH_BC  ;
  if (layout & CH_TOP_BACK_RIGHT       ) m_channelLayout[index++] = AE_CH_BR  ;
  m_channelLayout[index] = AE_CH_NULL;

  m_channels = m_pCodecContext->channels;
}

AEChLayout CDVDAudioCodecFFmpeg::GetChannelMap()
{
  BuildChannelMap();
  if (m_channelLayout[0] == AE_CH_NULL)
    return NULL;

  return m_channelLayout;
}
