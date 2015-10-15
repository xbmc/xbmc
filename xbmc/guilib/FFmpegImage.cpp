/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
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
#include "FFmpegImage.h"
#include "utils/log.h"
#include "cores/FFmpeg.h"
#include "guilib/Texture.h"

#include <algorithm>

extern "C"
{
#include <libavutil/imgutils.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

struct MemBuffer
{
  uint8_t* data;
  int64_t size;
  int64_t pos;
};

static int mem_file_read(void *h, uint8_t* buf, int size)
{
  MemBuffer* mbuf = static_cast<MemBuffer*>(h);
  int tocopy = (int) std::min((int64_t)size, mbuf->size-mbuf->pos);
  memcpy(buf, mbuf->data+mbuf->pos, tocopy);
  mbuf->pos += tocopy;
  return tocopy;
}

static int64_t mem_file_seek(void *h, int64_t pos, int whence)
{
  MemBuffer* mbuf = static_cast<MemBuffer*>(h);
  if(whence == AVSEEK_SIZE)
    return mbuf->size;

  whence &= ~AVSEEK_FORCE;
  if (whence == SEEK_SET)
    mbuf->pos = pos;
  else if (whence == SEEK_CUR)
    mbuf->pos = std::min(mbuf->pos+pos, mbuf->size-1);
  else
    mbuf->pos = mbuf->size+pos;

  return mbuf->pos;
}

CFFmpegImage::CFFmpegImage()
  : m_pitch(0)
{
  m_hasAlpha = false;
  m_pPictureRGB = static_cast<AVPicture*>(av_mallocz(sizeof(AVPicture)));
}

CFFmpegImage::~CFFmpegImage()
{
  av_freep(&m_pPictureRGB);
}

bool CFFmpegImage::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                                      unsigned int width, unsigned int height)
{
  if (!m_pPictureRGB)
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "AVPicture member not allocated");
    return false;
  }
  
  uint8_t* fbuffer = (uint8_t*)av_malloc(FFMPEG_FILE_BUFFER_SIZE);
  MemBuffer buf;
  buf.data = buffer;
  buf.size = bufSize;
  buf.pos = 0;

  AVIOContext* ioctx = avio_alloc_context(fbuffer, FFMPEG_FILE_BUFFER_SIZE, 0, &buf,
                                          mem_file_read, NULL, mem_file_seek);

  if (!ioctx)
  {
    av_free(fbuffer);
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate AVIOContext");
    return false;
  }

  AVFormatContext* fctx = avformat_alloc_context();
  if (!fctx)
  {
    av_free(ioctx->buffer);
    av_free(ioctx);
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate AVFormatContext");
    return false;
  }

  fctx->pb = ioctx;
  ioctx->max_packet_size = FFMPEG_FILE_BUFFER_SIZE;

  if (avformat_open_input(&fctx, "", NULL, NULL) < 0)
  {
    avformat_close_input(&fctx);
    FreeIOCtx(ioctx);
    return false;
  }

  AVCodecContext* codec_ctx = fctx->streams[0]->codec;
  AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
  if (avcodec_open2(codec_ctx, codec, NULL) < 0)
  {
    avformat_close_input(&fctx);
    FreeIOCtx(ioctx);
    return false;
  }

  AVFrame* frame = av_frame_alloc();
  if (!frame)
  {
    avformat_close_input(&fctx);
    FreeIOCtx(ioctx);
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not not allocated AVFrame");
    return false;
  }

  AVPacket pkt;
  av_read_frame(fctx, &pkt);
  int frame_decoded;
  int ret = avcodec_decode_video2(codec_ctx, frame, &frame_decoded, &pkt);
  if (ret < 0)
    CLog::Log(LOGDEBUG, "Error [%d] while decoding frame: %s\n", ret, strerror(AVERROR(ret)));

  if (frame_decoded != 0)
  {
    m_height = frame->height;
    m_width = frame->width;

    if (avpicture_alloc(m_pPictureRGB, PIX_FMT_RGB32, m_width, m_height) < 0)
    {
      CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate AVPicture member with %i x %i pixes", m_width, m_height);
      return false;
    }

    AVPixelFormat pixFormat = ConvertFormats(codec_ctx);

    struct SwsContext * context = sws_getContext(m_width, m_height, pixFormat,
      m_width, m_height, PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

    sws_scale(context, frame->data, frame->linesize, 0, m_height,
      m_pPictureRGB->data, m_pPictureRGB->linesize);
    sws_freeContext(context);

    m_pitch = m_pPictureRGB->linesize[0];

  }
  else
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not decode a frame");

  av_free_packet(&pkt);
  avcodec_close(codec_ctx);
  avformat_close_input(&fctx);
  FreeIOCtx(ioctx);

  return (frame_decoded != 0);
}

AVPixelFormat CFFmpegImage::ConvertFormats(AVCodecContext* codec_ctx)
{
  switch (codec_ctx->pix_fmt) {
  case AV_PIX_FMT_YUVJ420P:
    return AV_PIX_FMT_YUV420P;
    break;
  case AV_PIX_FMT_YUVJ422P:
    return AV_PIX_FMT_YUV422P;
    break;
  case AV_PIX_FMT_YUVJ444P:
    return AV_PIX_FMT_YUV444P;
    break;
  case AV_PIX_FMT_YUVJ440P:
    return AV_PIX_FMT_YUV440P;
  default:
    return codec_ctx->pix_fmt;
    break;
  }
}

void CFFmpegImage::FreeIOCtx(AVIOContext* ioctx)
{
  av_free(ioctx->buffer);
  av_free(ioctx);
}

bool CFFmpegImage::Decode(unsigned char * const pixels, unsigned int pitch,
                          unsigned int format)
{
  if (m_width == 0 || m_height == 0 || format != XB_FMT_A8R8G8B8)
    return false;

  const unsigned char *src = m_pPictureRGB->data[0];
  unsigned char* dst = pixels;

  if (pitch == m_pitch)
    memcpy(dst, src, m_height * m_pitch);
  else
  {
    for (unsigned int y = 0; y < m_height; y++)
    {
      memcpy(dst, src, m_pitch);
      src += m_pitch;
      dst += pitch;
    }
  }
  return true;
}

bool CFFmpegImage::CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width,
                                             unsigned int height, unsigned int format,
                                             unsigned int pitch,
                                             const std::string& destFile,
                                             unsigned char* &bufferout,
                                             unsigned int &bufferoutSize)
{
  return false;
}

void CFFmpegImage::ReleaseThumbnailBuffer()
{
  av_freep(&m_pPictureRGB);
}
