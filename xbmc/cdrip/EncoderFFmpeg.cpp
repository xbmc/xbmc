/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#ifdef _LINUX
#include "stdint.h"
#else
#define INT64_C __int64
#endif

#include "EncoderFFmpeg.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "settings/GUISettings.h"
#include "utils/URIUtils.h"

/* AV_PKT_FLAG_KEY was named PKT_FLAG_KEY in older versions of libavcodec */
#ifndef AV_PKT_FLAG_KEY
#define AV_PKT_FLAG_KEY PKT_FLAG_KEY
#endif

CEncoderFFmpeg::CEncoderFFmpeg():
  m_Format    (NULL),
  m_CodecCtx  (NULL),
  m_Stream    (NULL),
  m_Buffer    (NULL),
  m_BufferSize(0)
{
}

bool CEncoderFFmpeg::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load()) return false;
  m_dllAvFormat.av_register_all();
  m_dllAvCodec.avcodec_register_all();

  CStdString filename = URIUtils::GetFileName(strFile);
  AVOutputFormat *fmt = NULL;
  fmt = m_dllAvFormat.av_guess_format(NULL, filename.c_str(), NULL);
  if (!fmt)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Unable to guess the output format for the file %s", filename.c_str());
    return false;
  }

  AVCodec *codec;
  codec = m_dllAvCodec.avcodec_find_encoder(
    strcmp(fmt->name, "ogg") == 0 ? CODEC_ID_VORBIS : fmt->audio_codec
  );

  if (!codec)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Unable to find a suitable FFmpeg encoder");
    return false;
  }

  m_Format     = m_dllAvFormat.avformat_alloc_context();
  m_Format->pb = m_dllAvFormat.avio_alloc_context(m_BCBuffer, sizeof(m_BCBuffer), AVIO_FLAG_READ, this,  NULL, MuxerReadPacket, NULL);
  if (!m_Format->pb)
  {
    m_dllAvUtil.av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate ByteIOContext");
    return false;
  }

  m_Format->oformat  = fmt;
  m_Format->bit_rate = g_guiSettings.GetInt("audiocds.bitrate") * 1000;

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
  m_CodecCtx->time_base      = (AVRational){1, iInRate};

  if(fmt->flags & AVFMT_GLOBALHEADER)
  {
    m_CodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_Format->flags   |= CODEC_FLAG_GLOBAL_HEADER;
  }

  m_dllAvCodec.av_init_packet(&m_Pkt);
  m_Pkt.stream_index = m_Stream->index;
  m_Pkt.flags       |= AV_PKT_FLAG_KEY;

  switch(iInBits)
  {
    case  8: m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_U8 ; break;
    case 16: m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S16; break;
    case 32: m_CodecCtx->sample_fmt = AV_SAMPLE_FMT_S32; break;
    default:
      m_dllAvUtil.av_freep(&m_Stream);
      m_dllAvUtil.av_freep(&m_Format->pb);
      m_dllAvUtil.av_freep(&m_Format);
      return false;
  }

  if (m_dllAvCodec.avcodec_open2(m_CodecCtx, codec, NULL))
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to open the codec");
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  /* calculate how many bytes we need per frame */
  m_NeededFrames = m_CodecCtx->frame_size;
  m_NeededBytes  = m_NeededFrames * iInChannels * (iInBits / 8);
  m_Buffer       = new uint8_t[m_NeededBytes];
  m_BufferSize   = 0;

  /* set input stream information and open the file */
  if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits))
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to call CEncoder::Init");
    delete[] m_Buffer;
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
    delete[] m_Buffer;
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  return true;
}

void CEncoderFFmpeg::SetTag(const CStdString tag, const CStdString value)
{
  m_dllAvUtil.av_dict_set(&m_Format->metadata, tag.c_str(), value.c_str(), 0);
}

int CEncoderFFmpeg::MuxerReadPacket(void *opaque, uint8_t *buf, int buf_size)
{
  CEncoderFFmpeg *enc = (CEncoderFFmpeg*)opaque;
  if(enc->WriteStream(buf, buf_size) != buf_size)
  {
    CLog::Log(LOGERROR, "Error writing FFmpeg buffer to file");
    return -1;
  }
  return buf_size;
}

int CEncoderFFmpeg::Encode(int nNumBytesRead, BYTE* pbtStream)
{
  while(nNumBytesRead > 0)
  {
    unsigned int space = m_NeededBytes - m_BufferSize;
    unsigned int copy  = (unsigned int)nNumBytesRead > space ? space : nNumBytesRead;

    memcpy(&m_Buffer[m_BufferSize], pbtStream, space);
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
  uint8_t outbuf[FF_MIN_BUFFER_SIZE];
  int encoded;

  encoded = m_dllAvCodec.avcodec_encode_audio(m_CodecCtx, outbuf, FF_MIN_BUFFER_SIZE, (short*)m_Buffer);
  m_BufferSize = 0;
  if (encoded < 0) {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::WriteFrame - Error encoding audio");
    return false;
  }

  m_Pkt.data = outbuf;
  m_Pkt.size = encoded;

  if (m_CodecCtx->coded_frame && (uint64_t)m_CodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
    m_Pkt.pts = m_dllAvUtil.av_rescale_q(m_CodecCtx->coded_frame->pts, m_Stream->time_base, m_CodecCtx->time_base);

  if (m_dllAvFormat.av_write_frame(m_Format, &m_Pkt) < 0) {
    CLog::Log(LOGERROR, "CEncoderFFMmpeg::WriteFrame - Failed to write the frame data");
    return false;
  }

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

    /* write the eof flag */
    delete[] m_Buffer;
    m_Buffer = NULL;
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
  return true;
}

