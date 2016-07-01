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

#include "DVDAudioCodecFFmpeg.h"
#ifdef TARGET_POSIX
#include "XMemUtils.h"
#endif
#include "../../DVDStreamInfo.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "DVDCodecs/DVDCodecs.h"
extern "C" {
#include "libavutil/opt.h"
}

#include "settings/Settings.h"
#if defined(TARGET_DARWIN)
#include "cores/AudioEngine/Utils/AEUtil.h"
#endif

CDVDAudioCodecFFmpeg::CDVDAudioCodecFFmpeg(CProcessInfo &processInfo) : CDVDAudioCodec(processInfo)
{
  m_pCodecContext = NULL;

  m_channels = 0;
  m_layout = 0;
  
  m_pFrame1 = NULL;
  m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  m_gotFrame = 0;
}

CDVDAudioCodecFFmpeg::~CDVDAudioCodecFFmpeg()
{
  Dispose();
}

bool CDVDAudioCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  AVCodec* pCodec = NULL;
  bool allowdtshddecode = true;

  // set any special options
  for(std::vector<CDVDCodecOption>::iterator it = options.m_keys.begin(); it != options.m_keys.end(); ++it)
    if (it->m_name == "allowdtshddecode")
      allowdtshddecode = atoi(it->m_value.c_str());

  if (hints.codec == AV_CODEC_ID_DTS && allowdtshddecode)
    pCodec = avcodec_find_decoder_by_name("dcadec");

  if (!pCodec)
    pCodec = avcodec_find_decoder(hints.codec);

  if (!pCodec)
  {
    CLog::Log(LOGDEBUG,"CDVDAudioCodecFFmpeg::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  m_pCodecContext = avcodec_alloc_context3(pCodec);
  if (!m_pCodecContext)
    return false;

  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = 1;

  if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
    m_pCodecContext->flags |= CODEC_FLAG_TRUNCATED;

  m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
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
    m_pCodecContext->extradata = (uint8_t*)av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    if(m_pCodecContext->extradata)
    {
      m_pCodecContext->extradata_size = hints.extrasize;
      memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
    }
  }

  if (g_advancedSettings.m_audioApplyDrc >= 0.0)
    av_opt_set_double(m_pCodecContext, "drc_scale", g_advancedSettings.m_audioApplyDrc, AV_OPT_SEARCH_CHILDREN);

  if (avcodec_open2(m_pCodecContext, pCodec, NULL) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDAudioCodecFFmpeg::Open() Unable to open codec");
    Dispose();
    return false;
  }

  m_pFrame1 = av_frame_alloc();
  if (!m_pFrame1)
  {
    Dispose();
    return false;
  }

  m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  m_matrixEncoding = AV_MATRIX_ENCODING_NONE;

  m_processInfo.SetAudioDecoderName(m_pCodecContext->codec->name);
  return true;
}

void CDVDAudioCodecFFmpeg::Dispose()
{
  av_frame_free(&m_pFrame1);
  avcodec_free_context(&m_pCodecContext);
}

int CDVDAudioCodecFFmpeg::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  int iBytesUsed;
  if (!m_pCodecContext)
    return -1;

  AVPacket avpkt;
  av_init_packet(&avpkt);
  avpkt.data = pData;
  avpkt.size = iSize;
  avpkt.dts = (dts == DVD_NOPTS_VALUE) ? AV_NOPTS_VALUE : dts / DVD_TIME_BASE * AV_TIME_BASE;
  avpkt.pts = (pts == DVD_NOPTS_VALUE) ? AV_NOPTS_VALUE : pts / DVD_TIME_BASE * AV_TIME_BASE;
  iBytesUsed = avcodec_decode_audio4( m_pCodecContext
                                                 , m_pFrame1
                                                 , &m_gotFrame
                                                 , &avpkt);
  if (iBytesUsed < 0 || !m_gotFrame)
  {
    return iBytesUsed;
  }

  /* some codecs will attempt to consume more data than what we gave */
  if (iBytesUsed > iSize)
  {
    CLog::Log(LOGWARNING, "CDVDAudioCodecFFmpeg::Decode - decoder attempted to consume more data than given");
    iBytesUsed = iSize;
  }

  if (m_pFrame1->nb_side_data)
  {
    for (int i = 0; i < m_pFrame1->nb_side_data; i++)
    {
      AVFrameSideData *sd = m_pFrame1->side_data[i];
      if (sd->data)
      {
        if (sd->type == AV_FRAME_DATA_MATRIXENCODING)
        {
          m_matrixEncoding = *(enum AVMatrixEncoding*)sd->data;
        }
      }
    }
  }

  m_format.m_dataFormat = GetDataFormat();
  m_format.m_channelLayout = GetChannelMap();
  m_format.m_sampleRate = GetSampleRate();
  m_format.m_frameSize = m_format.m_channelLayout.Count() * CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3;
  return iBytesUsed;
}

void CDVDAudioCodecFFmpeg::GetData(DVDAudioFrame &frame)
{
  frame.passthrough = false;
  frame.nb_frames = 0;
  frame.format.m_dataFormat = m_format.m_dataFormat;
  frame.format.m_channelLayout = m_format.m_channelLayout;
  frame.framesize = (CAEUtil::DataFormatToBits(frame.format.m_dataFormat) >> 3) * frame.format.m_channelLayout.Count();
  if(frame.framesize == 0)
    return;
  frame.nb_frames = GetData(frame.data)/frame.framesize;
  frame.planes = AE_IS_PLANAR(frame.format.m_dataFormat) ? frame.format.m_channelLayout.Count() : 1;
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

  int64_t bpts = av_frame_get_best_effort_timestamp(m_pFrame1);
  if(bpts != AV_NOPTS_VALUE)
    frame.pts = (double)bpts * DVD_TIME_BASE / AV_TIME_BASE;
  else
    frame.pts = DVD_NOPTS_VALUE;
}

int CDVDAudioCodecFFmpeg::GetData(uint8_t** dst)
{
  if(m_gotFrame)
  {
    int planes = av_sample_fmt_is_planar(m_pCodecContext->sample_fmt) ? m_pFrame1->channels : 1;
    for (int i=0; i<planes; i++)
      dst[i] = m_pFrame1->extended_data[i];
    m_gotFrame = 0;
    return m_pFrame1->nb_samples * m_pFrame1->channels * av_get_bytes_per_sample(m_pCodecContext->sample_fmt);
  }

  return 0;
}

void CDVDAudioCodecFFmpeg::Reset()
{
  if (m_pCodecContext) avcodec_flush_buffers(m_pCodecContext);
  m_gotFrame = 0;
}

int CDVDAudioCodecFFmpeg::GetChannels()
{
  return m_pCodecContext->channels;
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
    return m_pCodecContext->bit_rate;
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
    CLog::Log(LOGINFO, "CDVDAudioCodecFFmpeg::GetChannelMap - FFmpeg reported %d channels, but the layout contains %d ignoring", m_pCodecContext->channels, bits);
    layout = av_get_default_channel_layout(m_pCodecContext->channels);
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

  m_channels = m_pCodecContext->channels;
}

CAEChannelInfo CDVDAudioCodecFFmpeg::GetChannelMap()
{
  BuildChannelMap();
  return m_channelLayout;
}
