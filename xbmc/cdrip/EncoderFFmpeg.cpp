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

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef TARGET_POSIX
#include "stdint.h"
#else
#define INT64_C __int64
#endif

#include "EncoderFFmpeg.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"

/* AV_PKT_FLAG_KEY was named PKT_FLAG_KEY in older versions of libavcodec */
#ifndef AV_PKT_FLAG_KEY
#define AV_PKT_FLAG_KEY PKT_FLAG_KEY
#endif

CEncoderFFmpeg::CEncoderFFmpeg():
  m_Format    (NULL),
  m_CodecCtx  (NULL),
  m_SwrCtx    (NULL),
  m_Stream    (NULL),
  m_Buffer    (NULL),
  m_BufferSize(0),
  m_BufferFrame(NULL),
  m_ResampledBuffer(NULL),
  m_ResampledBufferSize(0),
  m_ResampledFrame(NULL),
  m_NeedConversion(false)
{
}

bool CEncoderFFmpeg::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load() || !m_dllSwResample.Load()) return false;
  m_dllAvFormat.av_register_all();
  m_dllAvCodec.avcodec_register_all();

  CStdString filename = URIUtils::GetFileName(strFile);
  if(m_dllAvFormat.avformat_alloc_output_context2(&m_Format,NULL,NULL,filename.c_str()))
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Unable to guess the output format for the file %s", filename.c_str());
    return false;
  }

  AVCodec *codec;
  codec = m_dllAvCodec.avcodec_find_encoder(m_Format->oformat->audio_codec);

  if (!codec)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Unable to find a suitable FFmpeg encoder");
    return false;
  }

  m_Format->pb = m_dllAvFormat.avio_alloc_context(m_BCBuffer, sizeof(m_BCBuffer), AVIO_FLAG_WRITE, this,  NULL, avio_write_callback, avio_seek_callback);
  if (!m_Format->pb)
  {
    m_dllAvUtil.av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate ByteIOContext");
    return false;
  }

  m_Format->bit_rate = CSettings::Get().GetInt("audiocds.bitrate") * 1000;

  /* add a stream to it */
  m_Stream = m_dllAvFormat.avformat_new_stream(m_Format, codec);
  if (!m_Stream)
  {
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate AVStream context");
    return false;
  }

  /* set the stream's parameters */
  m_CodecCtx                 = m_Stream->codec;
  m_CodecCtx->codec_id       = codec->id;
  m_CodecCtx->codec_type     = AVMEDIA_TYPE_AUDIO;
  m_CodecCtx->bit_rate       = m_Format->bit_rate;
  m_CodecCtx->sample_rate    = iInRate;
  m_CodecCtx->channels       = iInChannels;
  m_CodecCtx->channel_layout = m_dllAvUtil.av_get_default_channel_layout(iInChannels);
  m_CodecCtx->time_base.num  = 1;
  m_CodecCtx->time_base.den  = iInRate;
  /* Allow experimental encoders (like FFmpeg builtin AAC encoder) */
  m_CodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

  if(m_Format->oformat->flags & AVFMT_GLOBALHEADER)
  {
    m_CodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_Format->flags   |= CODEC_FLAG_GLOBAL_HEADER;
  }

  switch(iInBits)
  {
    case  8: m_InFormat = AV_SAMPLE_FMT_U8 ; break;
    case 16: m_InFormat = AV_SAMPLE_FMT_S16; break;
    case 32: m_InFormat = AV_SAMPLE_FMT_S32; break;
    default:
      m_dllAvUtil.av_freep(&m_Stream);
      m_dllAvUtil.av_freep(&m_Format->pb);
      m_dllAvUtil.av_freep(&m_Format);
      return false;
  }

  m_OutFormat = codec->sample_fmts[0];
  m_CodecCtx->sample_fmt = m_OutFormat;

  m_NeedConversion = (m_OutFormat != m_InFormat);

  if (m_OutFormat <= AV_SAMPLE_FMT_NONE || m_dllAvCodec.avcodec_open2(m_CodecCtx, codec, NULL))
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to open the codec %s", codec->long_name ? codec->long_name : codec->name);
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  /* calculate how many bytes we need per frame */
  m_NeededFrames = m_CodecCtx->frame_size;
  m_NeededBytes  = m_dllAvUtil.av_samples_get_buffer_size(NULL, iInChannels, m_NeededFrames, m_InFormat, 0);
  m_Buffer       = (uint8_t*)m_dllAvUtil.av_malloc(m_NeededBytes);
  m_BufferSize   = 0;

  m_BufferFrame = m_dllAvCodec.avcodec_alloc_frame();
  if(!m_BufferFrame || !m_Buffer)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate necessary buffers");
    if(m_BufferFrame) m_dllAvCodec.avcodec_free_frame(&m_BufferFrame);
    if(m_Buffer) m_dllAvUtil.av_freep(&m_Buffer);
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  m_BufferFrame->nb_samples     = m_CodecCtx->frame_size;
  m_BufferFrame->format         = m_InFormat;
  m_BufferFrame->channel_layout = m_CodecCtx->channel_layout;

  m_dllAvCodec.avcodec_fill_audio_frame(m_BufferFrame, iInChannels, m_InFormat, m_Buffer, m_NeededBytes, 0);

  if(m_NeedConversion)
  {
    m_SwrCtx = m_dllSwResample.swr_alloc_set_opts(NULL,
                    m_CodecCtx->channel_layout, m_OutFormat, m_CodecCtx->sample_rate,
                    m_CodecCtx->channel_layout, m_InFormat, m_CodecCtx->sample_rate,
                    0, NULL);
    if(!m_SwrCtx || m_dllSwResample.swr_init(m_SwrCtx) < 0)
    {
      CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to initialize the resampler");
      m_dllAvCodec.avcodec_free_frame(&m_BufferFrame);
      m_dllAvUtil.av_freep(&m_Buffer);
      m_dllAvUtil.av_freep(&m_Stream);
      m_dllAvUtil.av_freep(&m_Format->pb);
      m_dllAvUtil.av_freep(&m_Format);
      return false;
    }

    m_ResampledBufferSize = m_dllAvUtil.av_samples_get_buffer_size(NULL, iInChannels, m_NeededFrames, m_OutFormat, 0);
    m_ResampledBuffer = (uint8_t*)m_dllAvUtil.av_malloc(m_ResampledBufferSize);
    m_ResampledFrame = m_dllAvCodec.avcodec_alloc_frame();
    if(!m_ResampledBuffer || !m_ResampledFrame)
    {
      CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate a frame for resampling");
      if (m_ResampledFrame)  m_dllAvCodec.avcodec_free_frame(&m_ResampledFrame);
      if (m_ResampledBuffer) m_dllAvUtil.av_freep(&m_ResampledBuffer);
      if (m_SwrCtx)          m_dllSwResample.swr_free(&m_SwrCtx);
      m_dllAvCodec.avcodec_free_frame(&m_BufferFrame);
      m_dllAvUtil.av_freep(&m_Buffer);
      m_dllAvUtil.av_freep(&m_Stream);
      m_dllAvUtil.av_freep(&m_Format->pb);
      m_dllAvUtil.av_freep(&m_Format);
      return false;
    }
    m_ResampledFrame->nb_samples     = m_NeededFrames;
    m_ResampledFrame->format         = m_OutFormat;
    m_ResampledFrame->channel_layout = m_CodecCtx->channel_layout;
    m_dllAvCodec.avcodec_fill_audio_frame(m_ResampledFrame, iInChannels, m_OutFormat, m_ResampledBuffer, m_ResampledBufferSize, 0);
  }

  /* set input stream information and open the file */
  if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits))
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to call CEncoder::Init");
    if (m_ResampledFrame ) m_dllAvCodec.avcodec_free_frame(&m_ResampledFrame);
    if (m_ResampledBuffer) m_dllAvUtil.av_freep(&m_ResampledBuffer);
    if (m_SwrCtx)          m_dllSwResample.swr_free(&m_SwrCtx);
    m_dllAvCodec.avcodec_free_frame(&m_BufferFrame);
    m_dllAvUtil.av_freep(&m_Buffer);
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  /* set the tags */
  SetTag("album"       , m_strAlbum);
  SetTag("album_artist", m_strArtist);
  SetTag("genre"       , m_strGenre);
  SetTag("title"       , m_strTitle);
  SetTag("track"       , m_strTrack);
  SetTag("encoder"     , "XBMC FFmpeg Encoder");

  /* write the header */
  if (m_dllAvFormat.avformat_write_header(m_Format, NULL) != 0)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to write the header");
    if (m_ResampledFrame ) m_dllAvCodec.avcodec_free_frame(&m_ResampledFrame);
    if (m_ResampledBuffer) m_dllAvUtil.av_freep(&m_ResampledBuffer);
    if (m_SwrCtx)          m_dllSwResample.swr_free(&m_SwrCtx);
    m_dllAvCodec.avcodec_free_frame(&m_BufferFrame);
    m_dllAvUtil.av_freep(&m_Buffer);
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  CLog::Log(LOGDEBUG, "CEncoderFFmpeg::Init - Successfully initialized with muxer %s and codec %s", m_Format->oformat->long_name? m_Format->oformat->long_name : m_Format->oformat->name, codec->long_name? codec->long_name : codec->name);

  return true;
}

void CEncoderFFmpeg::SetTag(const CStdString tag, const CStdString value)
{
  m_dllAvUtil.av_dict_set(&m_Format->metadata, tag.c_str(), value.c_str(), 0);
}

int CEncoderFFmpeg::avio_write_callback(void *opaque, uint8_t *buf, int buf_size)
{
  CEncoderFFmpeg *enc = (CEncoderFFmpeg*)opaque;
  if(enc->FileWrite(buf, buf_size) != buf_size)
  {
    CLog::Log(LOGERROR, "Error writing FFmpeg buffer to file");
    return -1;
  }
  return buf_size;
}

int64_t CEncoderFFmpeg::avio_seek_callback(void *opaque, int64_t offset, int whence)
{
  CEncoderFFmpeg *enc = (CEncoderFFmpeg*)opaque;
  return enc->FileSeek(offset, whence);
}

int CEncoderFFmpeg::Encode(int nNumBytesRead, BYTE* pbtStream)
{
  while(nNumBytesRead > 0)
  {
    unsigned int space = m_NeededBytes - m_BufferSize;
    unsigned int copy  = (unsigned int)nNumBytesRead > space ? space : nNumBytesRead;

    memcpy(&m_Buffer[m_BufferSize], pbtStream, copy);
    m_BufferSize  += copy;
    pbtStream     += copy;
    nNumBytesRead -= copy;

    /* only write full packets */
    if (m_BufferSize == m_NeededBytes)
      if (!WriteFrame()) return 0;
  }

  return 1;
}

bool CEncoderFFmpeg::WriteFrame()
{
  int encoded, got_output;
  AVFrame* frame;

  m_dllAvCodec.av_init_packet(&m_Pkt);
  m_Pkt.data = NULL;
  m_Pkt.size = 0;

  if(m_NeedConversion)
  {
    if (m_dllSwResample.swr_convert(m_SwrCtx, m_ResampledFrame->extended_data, m_NeededFrames, (const uint8_t**)m_BufferFrame->extended_data, m_NeededFrames) < 0)
    {
      CLog::Log(LOGERROR, "CEncoderFFmpeg::WriteFrame - Error resampling audio");
      return false;
    }
    frame = m_ResampledFrame;
  }
  else frame = m_BufferFrame;

  encoded = m_dllAvCodec.avcodec_encode_audio2(m_CodecCtx, &m_Pkt, frame, &got_output);

  m_BufferSize = 0;

  if (encoded < 0) {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::WriteFrame - Error encoding audio: %i", encoded);
    return false;
  }

  if (got_output)
  {
    if (m_CodecCtx->coded_frame && m_CodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
      m_Pkt.pts = m_dllAvUtil.av_rescale_q(m_CodecCtx->coded_frame->pts, m_Stream->time_base, m_CodecCtx->time_base);

    if (m_dllAvFormat.av_write_frame(m_Format, &m_Pkt) < 0) {
      CLog::Log(LOGERROR, "CEncoderFFMmpeg::WriteFrame - Failed to write the frame data");
      return false;
    }
  }

  m_dllAvCodec.av_free_packet(&m_Pkt);

  return true;
}

bool CEncoderFFmpeg::Close()
{
  if (m_Format) {
    /* if there is anything still in the buffer */
    if (m_BufferSize > 0) {
      /* zero the unused space so we dont encode random junk */
      memset(&m_Buffer[m_BufferSize], 0, m_NeededBytes - m_BufferSize);
      /* write any remaining data */
      WriteFrame();
    }

    /* Flush if needed */
    m_dllAvUtil.av_freep(&m_Buffer);
    m_dllAvCodec.avcodec_free_frame(&m_BufferFrame);

    if (m_SwrCtx)          m_dllSwResample.swr_free(&m_SwrCtx);
    if (m_ResampledFrame ) m_dllAvCodec.avcodec_free_frame(&m_ResampledFrame);
    if (m_ResampledBuffer) m_dllAvUtil.av_freep(&m_ResampledBuffer);
    m_NeedConversion = false;

    WriteFrame();

    /* write the trailer */
    m_dllAvFormat.av_write_trailer(m_Format);
    FlushStream();
    FileClose();

    /* cleanup */
    m_dllAvCodec.avcodec_close(m_CodecCtx);
    m_dllAvUtil.av_freep(&m_Stream    );
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format    );
  }

  m_BufferSize = 0;

  m_dllAvFormat.Unload();
  m_dllAvUtil  .Unload();
  m_dllAvCodec .Unload();
  m_dllSwResample.Unload();
  return true;
}

