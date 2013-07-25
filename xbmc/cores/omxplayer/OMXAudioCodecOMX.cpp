/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "OMXAudioCodecOMX.h"
#ifdef TARGET_LINUX
#include "XMemUtils.h"
#endif
#include "utils/log.h"

#include "cores/AudioEngine/Utils/AEUtil.h"

#define MAX_AUDIO_FRAME_SIZE (AVCODEC_MAX_AUDIO_FRAME_SIZE*2)

COMXAudioCodecOMX::COMXAudioCodecOMX()
{
  m_iBufferSize2 = 0;
  m_pBuffer2     = (BYTE*)_aligned_malloc(MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE, 16);
  memset(m_pBuffer2, 0, MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);

  m_iBufferUpmixSize = 0;
  m_pBufferUpmix = (BYTE*)_aligned_malloc(MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE, 16);
  memset(m_pBufferUpmix, 0, MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);

  m_iBuffered = 0;
  m_pCodecContext = NULL;
  m_pConvert = NULL;
  m_bOpenedCodec = false;

  m_channels = 0;
  m_layout = 0;
  m_pFrame1 = NULL;
  m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  m_desiredSampleFormat = AV_SAMPLE_FMT_NONE;
  m_iBufferSize1 = 0;
}

COMXAudioCodecOMX::~COMXAudioCodecOMX()
{
  _aligned_free(m_pBuffer2);
  _aligned_free(m_pBufferUpmix);
  Dispose();
}

bool COMXAudioCodecOMX::Open(CDVDStreamInfo &hints)
{
  AVCodec* pCodec;
  m_bOpenedCodec = false;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwResample.Load())
    return false;

  m_dllAvCodec.avcodec_register_all();

  pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);
  if (!pCodec)
  {
    CLog::Log(LOGDEBUG,"COMXAudioCodecOMX::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  m_bFirstFrame = true;
  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context3(pCodec);
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

  if (m_dllAvCodec.avcodec_open2(m_pCodecContext, pCodec, NULL) < 0)
  {
    CLog::Log(LOGDEBUG,"COMXAudioCodecOMX::Open() Unable to open codec");
    Dispose();
    return false;
  }

  m_pFrame1 = m_dllAvCodec.avcodec_alloc_frame();
  m_bOpenedCodec = true;
  m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  m_desiredSampleFormat = m_pCodecContext->sample_fmt == AV_SAMPLE_FMT_S16 ? AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_FLTP;
  return true;
}

void COMXAudioCodecOMX::Dispose()
{
  if (m_pFrame1) m_dllAvUtil.av_free(m_pFrame1);
  m_pFrame1 = NULL;

  if (m_pConvert)
    m_dllSwResample.swr_free(&m_pConvert);

  if (m_pCodecContext)
  {
    if (m_pCodecContext->extradata) m_dllAvUtil.av_free(m_pCodecContext->extradata);
    m_pCodecContext->extradata = NULL;
    if (m_bOpenedCodec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    m_bOpenedCodec = false;
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  m_dllSwResample.Unload();

  m_iBufferSize1 = 0;
  m_iBufferSize2 = 0;
  m_iBuffered = 0;
}

int COMXAudioCodecOMX::Decode(BYTE* pData, int iSize)
{
  int iBytesUsed, got_frame;
  if (!m_pCodecContext) return -1;

  AVPacket avpkt;
  m_dllAvCodec.av_init_packet(&avpkt);
  avpkt.data = pData;
  avpkt.size = iSize;
  iBytesUsed = m_dllAvCodec.avcodec_decode_audio4( m_pCodecContext
                                                 , m_pFrame1
                                                 , &got_frame
                                                 , &avpkt);
  if (iBytesUsed < 0 || !got_frame)
  {
    m_iBufferSize1 = 0;
    m_iBufferSize2 = 0;
    return iBytesUsed;
  }
  int linesize1, linesize2;
  m_iBufferSize1 = m_dllAvUtil.av_samples_get_buffer_size(&linesize1, m_pCodecContext->channels, m_pFrame1->nb_samples, m_pCodecContext->sample_fmt, 1);
  m_iBufferSize2 = m_dllAvUtil.av_samples_get_buffer_size(&linesize2, m_pCodecContext->channels, m_pFrame1->nb_samples, m_desiredSampleFormat, 1);

  /* some codecs will attempt to consume more data than what we gave */
  if (iBytesUsed > iSize)
  {
    CLog::Log(LOGWARNING, "COMXAudioCodecOMX::Decode - decoder attempted to consume more data than given");
    iBytesUsed = iSize;
  }

  if(m_iBufferSize1 == 0 && iBytesUsed >= 0)
    m_iBuffered += iBytesUsed;
  else
    m_iBuffered = 0;

  if (m_bFirstFrame)
  {
    CLog::Log(LOGDEBUG, "COMXAudioCodecOMX::Decode(%p,%d) format=%d(%d) chan=%d samples=%d size=%d/%d,%d/%d,%d data=%p,%p,%p,%p,%p,%p,%p,%p",
             pData, iSize, m_pCodecContext->sample_fmt, m_desiredSampleFormat, m_pCodecContext->channels, m_pFrame1->nb_samples,
             m_iBufferSize1, m_iBufferSize2, linesize1, linesize2, m_pFrame1->linesize[0],
             m_pFrame1->data[0], m_pFrame1->data[1], m_pFrame1->data[2], m_pFrame1->data[3], m_pFrame1->data[4], m_pFrame1->data[5], m_pFrame1->data[6], m_pFrame1->data[7]
             );
  }

  if(m_pCodecContext->sample_fmt != m_desiredSampleFormat && m_iBufferSize1 > 0)
  {
    if(m_pConvert && (m_pCodecContext->sample_fmt != m_iSampleFormat || m_channels != m_pCodecContext->channels))
      m_dllSwResample.swr_free(&m_pConvert);

    if(!m_pConvert)
    {
      m_iSampleFormat = m_pCodecContext->sample_fmt;
      m_pConvert = m_dllSwResample.swr_alloc_set_opts(NULL,
                      m_dllAvUtil.av_get_default_channel_layout(m_pCodecContext->channels), 
                      m_desiredSampleFormat, m_pCodecContext->sample_rate,
                      m_dllAvUtil.av_get_default_channel_layout(m_pCodecContext->channels), 
                      m_pCodecContext->sample_fmt, m_pCodecContext->sample_rate,
                      0, NULL);

      if(!m_pConvert || m_dllSwResample.swr_init(m_pConvert) < 0)
      {
        CLog::Log(LOGERROR, "COMXAudioCodecOMX::Decode - Unable to initialise convert format %d to %d", m_pCodecContext->sample_fmt, m_desiredSampleFormat);
        m_iBufferSize1 = 0;
        m_iBufferSize2 = 0;
        return iBytesUsed;
      }
    }
    m_iBufferSize1 = 0;

    BYTE *out_planes[] = {
            m_pBuffer2 + 0 * linesize2, m_pBuffer2 + 1 * linesize2, m_pBuffer2 + 2 * linesize2, m_pBuffer2 + 3 * linesize2,
            m_pBuffer2 + 4 * linesize2, m_pBuffer2 + 5 * linesize2, m_pBuffer2 + 6 * linesize2, m_pBuffer2 + 7 * linesize2,
    };

    if(m_dllSwResample.swr_convert(m_pConvert, out_planes, m_pFrame1->nb_samples, (const uint8_t **)m_pFrame1->data, m_pFrame1->nb_samples) < 0)
    {
      CLog::Log(LOGERROR, "COMXAudioCodecOMX::Decode - Unable to convert format %d to %d", (int)m_pCodecContext->sample_fmt, m_desiredSampleFormat);
      m_iBufferSize1 = 0;
      m_iBufferSize2 = 0;
      return iBytesUsed;
    }
  }
  else
  {
    m_iBufferSize2 = 0;
  }

  return iBytesUsed;
}

int COMXAudioCodecOMX::GetData(BYTE** dst)
{
  int size = 0;
  bool contiguous = true;

  if(m_iBufferSize1)
  {
    int i;
    int linesize;
    BYTE *next = m_pFrame1->data[0];
    m_dllAvUtil.av_samples_get_buffer_size(&linesize, m_pCodecContext->channels, m_pFrame1->nb_samples, m_pCodecContext->sample_fmt, 1);
    for (i=0; i<m_pCodecContext->channels; i++)
    {
      if (!m_pFrame1->data[i])
        break;
      if (next != m_pFrame1->data[i])
        contiguous = false;
      next += linesize;
      size += linesize;
    }
    if (size != m_iBufferSize1)
      contiguous = false;

    if (contiguous)
    {
      *dst = m_pFrame1->data[0];
      size = m_iBufferSize1;
    }
    else
    {
      m_iBufferUpmixSize = 0;
      for (i=0; i<m_pCodecContext->channels; i++)
      {
        if (m_iBufferUpmixSize + linesize <= MAX_AUDIO_FRAME_SIZE && m_pFrame1->data[i])
        {
          memcpy(m_pBufferUpmix + m_iBufferUpmixSize, m_pFrame1->data[i], linesize);
          m_iBufferUpmixSize += linesize;
        } else assert(0);
      }
      *dst = m_pBufferUpmix;
      size = m_iBufferUpmixSize;
    }
  }
  if(m_iBufferSize2)
  {
    *dst = m_pBuffer2;
    size = m_iBufferSize2;
  }
  if (m_bFirstFrame)
  {
    CLog::Log(LOGDEBUG, "COMXAudioCodecOMX::GetData size=%d/%d/%d cont=%d buf=%p", m_iBufferSize1, m_iBufferSize2, size, contiguous, *dst);
     m_bFirstFrame = false;
  }
  return size;
}

void COMXAudioCodecOMX::Reset()
{
  if (m_pCodecContext) m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);
  m_iBufferSize1 = 0;
  m_iBufferSize2 = 0;
  m_iBuffered = 0;
}

int COMXAudioCodecOMX::GetChannels()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->channels;
}

int COMXAudioCodecOMX::GetSampleRate()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->sample_rate;
}

int COMXAudioCodecOMX::GetBitsPerSample()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->sample_fmt == AV_SAMPLE_FMT_S16 ? 16 : 32;
}

int COMXAudioCodecOMX::GetBitRate()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->bit_rate;
}

static unsigned count_bits(int64_t value)
{
  unsigned bits = 0;
  for(;value;++bits)
    value &= value - 1;
  return bits;
}

void COMXAudioCodecOMX::BuildChannelMap()
{
  if (m_channels == m_pCodecContext->channels && m_layout == m_pCodecContext->channel_layout)
    return; //nothing to do here

  m_channels = m_pCodecContext->channels;
  m_layout   = m_pCodecContext->channel_layout;

  int64_t layout;

  int bits = count_bits(m_pCodecContext->channel_layout);
  if (bits == m_pCodecContext->channels)
    layout = m_pCodecContext->channel_layout;
  else
  {
    CLog::Log(LOGINFO, "COMXAudioCodecOMX::GetChannelMap - FFmpeg reported %d channels, but the layout contains %d ignoring", m_pCodecContext->channels, bits);
    layout = m_dllAvUtil.av_get_default_channel_layout(m_pCodecContext->channels);
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
}

CAEChannelInfo COMXAudioCodecOMX::GetChannelMap()
{
  BuildChannelMap();
  return m_channelLayout;
}
