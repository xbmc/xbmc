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
#define DTS_ENCODE_BITRATE 1411200

#include "AEEncoderFFmpeg.h"
#include "AEUtil.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include <string.h>

CAEEncoderFFmpeg::CAEEncoderFFmpeg():
  m_CodecCtx(NULL)
{
}

CAEEncoderFFmpeg::~CAEEncoderFFmpeg()
{
  Reset();
  m_dllAvUtil.av_freep(&m_CodecCtx);
}

bool CAEEncoderFFmpeg::IsCompatible(const AEAudioFormat format)
{
  if (!m_CodecCtx)
    return false;

  bool match = (
    format.m_dataFormat == m_CurrentFormat.m_dataFormat &&
    format.m_sampleRate == m_CurrentFormat.m_sampleRate
  );

  if (match)
  {
    enum AEChannel layout[AE_CH_MAX+1];
    unsigned int channels = BuildChannelLayout(AV_CH_LAYOUT_5POINT1_BACK, layout); /* hard coded for AC3 & DTS currently */
    if (channels != m_CurrentFormat.m_channelCount)
    {
      match = false;
    }
    else
    {
      for(unsigned int i = 0; layout[i] != AE_CH_NULL; ++i)
        if (format.m_channelLayout[i] != layout[i])
        {
          match = false;
          break;
        }
    }
  }

  return match;
}

unsigned int CAEEncoderFFmpeg::BuildChannelLayout(const int64_t ffmap, AEChLayout layout)
{
  /* build the channel layout and count the channels */
  unsigned int channels = 0;
  if (ffmap & AV_CH_FRONT_LEFT           ) layout[channels++] = AE_CH_FL  ;
  if (ffmap & AV_CH_FRONT_RIGHT          ) layout[channels++] = AE_CH_FR  ;
  if (ffmap & AV_CH_FRONT_CENTER         ) layout[channels++] = AE_CH_FC  ;
  if (ffmap & AV_CH_LOW_FREQUENCY        ) layout[channels++] = AE_CH_LFE ;
  if (ffmap & AV_CH_BACK_LEFT            ) layout[channels++] = AE_CH_BL  ;
  if (ffmap & AV_CH_BACK_RIGHT           ) layout[channels++] = AE_CH_BR  ;
  if (ffmap & AV_CH_FRONT_LEFT_OF_CENTER ) layout[channels++] = AE_CH_FLOC;
  if (ffmap & AV_CH_FRONT_RIGHT_OF_CENTER) layout[channels++] = AE_CH_FROC;
  if (ffmap & AV_CH_BACK_CENTER          ) layout[channels++] = AE_CH_BC  ;
  if (ffmap & AV_CH_SIDE_LEFT            ) layout[channels++] = AE_CH_SL  ;
  if (ffmap & AV_CH_SIDE_RIGHT           ) layout[channels++] = AE_CH_SR  ;
  if (ffmap & AV_CH_TOP_CENTER           ) layout[channels++] = AE_CH_TC  ;
  if (ffmap & AV_CH_TOP_FRONT_LEFT       ) layout[channels++] = AE_CH_TFL ;
  if (ffmap & AV_CH_TOP_FRONT_CENTER     ) layout[channels++] = AE_CH_TFC ;
  if (ffmap & AV_CH_TOP_FRONT_RIGHT      ) layout[channels++] = AE_CH_TFR ;
  if (ffmap & AV_CH_TOP_BACK_LEFT        ) layout[channels++] = AE_CH_TBL ;
  if (ffmap & AV_CH_TOP_BACK_CENTER      ) layout[channels++] = AE_CH_TBC ;
  if (ffmap & AV_CH_TOP_BACK_RIGHT       ) layout[channels++] = AE_CH_TBR ;
  layout[channels] = AE_CH_NULL;

  return channels;
}

bool CAEEncoderFFmpeg::Initialize(AEAudioFormat &format)
{
  Reset();

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load())
    return false;

  m_dllAvCodec.avcodec_register_all();

  bool ac3 = g_guiSettings.GetBool("audiooutput.ac3passthrough");

  AVCodec *codec = NULL;
#if 0
  /* the DCA encoder is currently useless for transcode, it creates a 196 kHz DTS-HD like mongrel which is useless for SPDIF */
  bool dts = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  if (dts && (!ac3 || g_advancedSettings.m_audioTranscodeTo.Equals("dts")))
  {
    m_CodecName = "DTS";
    m_CodecID   = CODEC_ID_DTS;
    m_PackFunc  = &CAEPackIEC958::PackDTS_1024;
    m_BitRate   = DTS_ENCODE_BITRATE;
    codec = m_dllAvCodec.avcodec_find_encoder(m_CodecID);
  }
#endif

  /* fallback to ac3 if we support it, we might not have DTS support */
  if (!codec && ac3)
  {
    m_CodecName = "AC3";
    m_CodecID   = CODEC_ID_AC3;
    m_PackFunc  = &CAEPackIEC958::PackAC3;
    m_BitRate   = AC3_ENCODE_BITRATE;
    codec = m_dllAvCodec.avcodec_find_encoder(m_CodecID);
  }

  /* check we got the codec */
  if (!codec)
    return false;

  m_CodecCtx                 = m_dllAvCodec.avcodec_alloc_context();
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

    for(int i = 0; codec->sample_fmts[i] != AV_SAMPLE_FMT_NONE; ++i)
    {
      switch(codec->sample_fmts[i])
      {
        case AV_SAMPLE_FMT_FLT: hasFloat  = true; break;
        case AV_SAMPLE_FMT_DBL: hasDouble = true; break;
        case AV_SAMPLE_FMT_S32: hasS32    = true; break;
        case AV_SAMPLE_FMT_S16: hasS16    = true; break;
        case AV_SAMPLE_FMT_U8 : hasU8     = true; break;

        default:
          return false;
      }
    }

    if (hasFloat)
    {
      m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_FLT;
      format.m_dataFormat    = AE_FMT_FLOAT;
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
    else
    {
      CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Initialize - Unable to find a suitable data format for the codec (%s)", m_CodecName.c_str());
      return false;
    }
  }

  m_CodecCtx->channels = BuildChannelLayout(m_CodecCtx->channel_layout, m_Layout);

  /* open the codec */
  if (m_dllAvCodec.avcodec_open(m_CodecCtx, codec))
  {
    m_dllAvUtil.av_freep(&m_CodecCtx);
    return false;
  }  

  format.m_dataFormat    = AE_FMT_FLOAT;
  format.m_frames        = m_CodecCtx->frame_size;
  format.m_frameSamples  = m_CodecCtx->frame_size * m_CodecCtx->channels;
  format.m_frameSize     = m_CodecCtx->channels * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  format.m_channelCount  = m_CodecCtx->channels;
  format.m_channelLayout = m_Layout;

  m_CurrentFormat = format;
  m_NeededFrames  = format.m_frames;
  m_OutputSize    = m_PackFunc(NULL, 0, m_Buffer);
  m_OutputRatio   = (float)m_NeededFrames / m_OutputSize;

  CLog::Log(LOGERROR, "CAEEncoderFFmpeg::Initialize - %s encoder ready", m_CodecName.c_str());
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

CodecID CAEEncoderFFmpeg::GetCodecID()
{
  return m_CodecID;
}

unsigned int CAEEncoderFFmpeg::GetFrames()
{
  return m_NeededFrames;
}

int CAEEncoderFFmpeg::Encode(float *data, unsigned int frames)
{
  if (!m_CodecCtx || frames < m_NeededFrames)
    return 0;

  /* encode it */
  int size = m_dllAvCodec.avcodec_encode_audio(m_CodecCtx, m_Buffer + IEC958_DATA_OFFSET, FF_MIN_BUFFER_SIZE, (short*)data);

  /* pack it into an IEC958 frame */
  m_BufferSize = m_PackFunc(NULL, size, m_Buffer);
  if (m_BufferSize != m_OutputSize)
  {
    m_OutputSize  = m_BufferSize;
    m_OutputRatio = (float)m_NeededFrames / m_OutputSize;
  }

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

float CAEEncoderFFmpeg::GetDelay(unsigned int bufferSize)
{
  if (!m_CodecCtx)
    return 0;

  int frames = m_CodecCtx->delay;
  if (m_BufferSize)
    frames += m_NeededFrames;

  return ((float)frames + ((float)bufferSize * m_OutputRatio)) / (float)m_CodecCtx->sample_rate;
}

