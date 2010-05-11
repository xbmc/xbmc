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

#include "EncoderFFmpeg.h"
#include "FileSystem/File.h"
#include "utils/log.h"
#include "GUISettings.h"

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

  AVOutputFormat *fmt = NULL;
#if LIBAVFORMAT_VERSION_MAJOR < 52
  fmt = guess_format(NULL, strFile, NULL);
#else
  fmt = av_guess_format(NULL, strFile, NULL);
#endif
  if (!fmt)
    return false;

  AVCodec *codec;
  codec = m_dllAvCodec.avcodec_find_encoder(fmt->audio_codec);
  if (!codec)
    return false;

  m_Format     = m_dllAvFormat.avformat_alloc_context();
  m_Format->pb = m_dllAvFormat.av_alloc_put_byte(m_BCBuffer, sizeof(m_BCBuffer), URL_RDONLY, this,  NULL, MuxerReadPacket, NULL);
  if (!m_Format->pb)
  {
    m_dllAvUtil.av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate ByteIOContext");
    return false;
  }

  /* this is streamed, no file, and ignore the index */
  m_Format->pb->is_streamed   = 1;
  m_Format->flags            |= AVFMT_NOFILE | AVFMT_FLAG_IGNIDX;
  m_Format->bit_rate          = iInRate;

  /* setup the muxer */
  AVFormatParameters params;
  memset(&params, 0, sizeof(params));
  params.channels       = iInChannels;
  params.sample_rate    = iInRate;
  params.audio_codec_id = fmt->audio_codec;
  params.channel        = 0;
  if (m_dllAvFormat.av_set_parameters(m_Format, &params) != 0)
  {
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to set the muxer parameters");
    return false;
  }

  /* add a stream to it */
  m_Stream = m_dllAvFormat.av_new_stream(m_Format, 1);
  if (!m_Stream)
  {
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate AVStream context");
    return false;
  }

  /* set the stream's parameters */
  m_Stream->stream_copy = 1;

  m_CodecCtx                 = m_Stream->codec;
  m_CodecCtx->bit_rate       = g_guiSettings.GetInt("audiocds.bitrate");
  m_CodecCtx->sample_rate    = iInRate;
  m_CodecCtx->channels       = iInChannels;
  m_CodecCtx->channel_layout = m_dllAvCodec.avcodec_guess_channel_layout(iInChannels, codec->id, NULL);

  switch(iInBits)
  {
    case  8: m_CodecCtx->sample_fmt = SAMPLE_FMT_U8 ; break;
    case 16: m_CodecCtx->sample_fmt = SAMPLE_FMT_S16; break;
    case 32: m_CodecCtx->sample_fmt = SAMPLE_FMT_S32; break;
    default:
      m_dllAvUtil.av_freep(&m_Stream);
      m_dllAvUtil.av_freep(&m_Format->pb);
      m_dllAvUtil.av_freep(&m_Format);
      return false;
  }

  if (m_dllAvCodec.avcodec_open(m_CodecCtx, codec))
  {
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
    delete[] m_Buffer;
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  /* write the header */
  if (m_dllAvFormat.av_write_header(m_Format) != 0)
  {
    delete[] m_Buffer;
    m_dllAvUtil.av_freep(&m_Stream);
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format);
    return false;
  }

  return true;
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
  int encoded = m_dllAvCodec.avcodec_encode_audio(m_CodecCtx, outbuf, FF_MIN_BUFFER_SIZE, (short*)m_Buffer);
  m_BufferSize = 0;
  if (encoded < 0) {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::WriteFrame - Error encoding audio");
    return false;
  }

  AVPacket pkt;
  m_dllAvCodec.av_init_packet(&pkt);
  pkt.data = outbuf;
  pkt.size = encoded;

  if (m_dllAvFormat.av_write_frame(m_Format, &pkt) < 0) {
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
      /* write the last frame */
      WriteFrame();
    }

    /* write the trailer */
    m_dllAvFormat.av_write_trailer(m_Format);
  }

  delete[] m_Buffer;
  m_BufferSize = 0;

  m_dllAvUtil.av_freep(&m_Stream);
  if (m_Format)
  {
    m_dllAvUtil.av_freep(&m_Format->pb);
    m_dllAvUtil.av_freep(&m_Format    );
  }

  m_dllAvFormat.Unload();
  m_dllAvUtil  .Unload();
  m_dllAvCodec .Unload();
  return true;
}

