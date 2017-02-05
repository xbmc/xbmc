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
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}

Frame::Frame() :
  m_pImage(nullptr),
  m_delay(0),
  m_imageSize(0),
  m_height(0),
  m_width(0)
{}


Frame::Frame(const Frame& src) :
  m_pImage(nullptr),
  m_delay(src.m_delay),
  m_imageSize(src.m_imageSize),
  m_height(src.m_height),
  m_width(src.m_width)
{
  if (src.m_pImage)
  {
    m_pImage = new unsigned char[m_imageSize];
    memcpy(m_pImage, src.m_pImage, m_imageSize);
  }
}

Frame::~Frame()
{
  delete[] m_pImage;
  m_pImage = nullptr;
}

struct ThumbDataManagement
{
  uint8_t* intermediateBuffer = nullptr; // gets av_alloced
  AVFrame* frame_input = nullptr;
  AVFrame* frame_temporary = nullptr;
  SwsContext* sws = nullptr;
  AVCodecContext* avOutctx = nullptr;
  AVCodec* codec = nullptr;
  ~ThumbDataManagement()
  {
    av_free(intermediateBuffer);
    intermediateBuffer = nullptr;
    av_frame_free(&frame_input);
    frame_input = nullptr;
    av_frame_free(&frame_temporary);
    frame_temporary = nullptr;
    avcodec_close(avOutctx);
    avcodec_free_context(&avOutctx);
    avOutctx = nullptr;
    sws_freeContext(sws);
    sws = nullptr;
  }
};

// valid positions are including 0 (start of buffer)
// and bufferSize -1 last data point
static inline size_t Clamp(int64_t newPosition, size_t bufferSize)
{
  return std::min(std::max((int64_t) 0, newPosition), (int64_t) (bufferSize -1));
}

static int mem_file_read(void *h, uint8_t* buf, int size)
{
  if (size < 0)
    return -1;

  MemBuffer* mbuf = static_cast<MemBuffer*>(h);
  int64_t unread = mbuf->size - mbuf->pos;
  if (unread <= 0)
    return 0;
  
  size_t tocopy = std::min((size_t)size, (size_t)unread);
  memcpy(buf, mbuf->data + mbuf->pos, tocopy);
  mbuf->pos += tocopy;
  return tocopy;
}

static int64_t mem_file_seek(void *h, int64_t pos, int whence)
{
  MemBuffer* mbuf = static_cast<MemBuffer*>(h);
  if (whence == AVSEEK_SIZE)
    return mbuf->size;

  // we want to ignore the AVSEEK_FORCE flag and therefore mask it away
  whence &= ~AVSEEK_FORCE;

  if (whence == SEEK_SET)
  {
    mbuf->pos = Clamp(pos, mbuf->size);
  }
  else if (whence == SEEK_CUR)
  {
    mbuf->pos = Clamp(((int64_t)mbuf->pos) + pos, mbuf->size);
  }
  else
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Unknown seek mode: %i", whence);

  return mbuf->pos;
}

CFFmpegImage::CFFmpegImage(const std::string& strMimeType) : m_strMimeType(strMimeType)
{
  m_hasAlpha = false;
  m_pFrame = nullptr;
  m_outputBuffer = nullptr;
}

CFFmpegImage::~CFFmpegImage()
{
  av_frame_free(&m_pFrame);
  // someone could have forgotten to call us
  CleanupLocalOutputBuffer();
  if (m_fctx)
  {
    avcodec_free_context(&m_codec_ctx);
    avformat_close_input(&m_fctx);
  }
  if (m_ioctx)
    FreeIOCtx(&m_ioctx);

  m_buf.data = nullptr;
  m_buf.pos = 0;
  m_buf.size = 0;
}

bool CFFmpegImage::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                                      unsigned int width, unsigned int height)
{
    
  if (!Initialize(buffer, bufSize))
  {
    //log
    return false;
  }

  av_frame_free(&m_pFrame);
  m_pFrame = ExtractFrame();

  return !(m_pFrame == nullptr);
}

bool CFFmpegImage::Initialize(unsigned char* buffer, unsigned int bufSize)
{
  uint8_t* fbuffer = (uint8_t*)av_malloc(FFMPEG_FILE_BUFFER_SIZE);
  if (!fbuffer)
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate FFMPEG_FILE_BUFFER_SIZE");
    return false;
  }
  m_buf.data = buffer;
  m_buf.size = bufSize;
  m_buf.pos = 0;

  m_ioctx = avio_alloc_context(fbuffer, FFMPEG_FILE_BUFFER_SIZE, 0, &m_buf,
    mem_file_read, NULL, mem_file_seek);

  if (!m_ioctx)
  {
    av_free(fbuffer);
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate AVIOContext");
    return false;
  }

  m_fctx = avformat_alloc_context();
  if (!m_fctx)
  {
    FreeIOCtx(&m_ioctx);
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate AVFormatContext");
    return false;
  }

  m_fctx->pb = m_ioctx;
  m_ioctx->max_packet_size = FFMPEG_FILE_BUFFER_SIZE;

  // Some clients have pngs saved as jpeg or ask us for png but are jpeg
  // mythv throws all mimetypes away and asks us with application/octet-stream
  // this is poor man's fallback to at least identify png / jpeg
  bool is_jpeg = (bufSize > 2 && buffer[0] == 0xFF && buffer[1] == 0xD8 && buffer[2] == 0xFF);
  bool is_png = (bufSize > 3 && buffer[1] == 'P' && buffer[2] == 'N' && buffer[3] == 'G');
  bool is_tiff = (bufSize > 2 && buffer[0] == 'I' && buffer[1] == 'I' && buffer[2] == '*');

  AVInputFormat* inp = nullptr;
  if (is_jpeg)
    inp = av_find_input_format("jpeg_pipe");
  else if (m_strMimeType == "image/apng")
    inp = av_find_input_format("apng");
  else if (is_png)
    inp = av_find_input_format("png_pipe");
  else if (is_tiff)
    inp = av_find_input_format("tiff_pipe");
  else if (m_strMimeType == "image/jp2")
    inp = av_find_input_format("j2k_pipe");
  else if (m_strMimeType == "image/webp")
    inp = av_find_input_format("webp_pipe");
  // brute force parse if above check already failed
  else if (m_strMimeType == "image/jpeg" || m_strMimeType == "image/jpg")
    inp = av_find_input_format("jpeg_pipe");
  else if (m_strMimeType == "image/png")
    inp = av_find_input_format("png_pipe");
  else if (m_strMimeType == "image/tiff")
    inp = av_find_input_format("tiff_pipe");
  else if (m_strMimeType == "image/gif")
    inp = av_find_input_format("gif");

  if (avformat_open_input(&m_fctx, NULL, inp, NULL) < 0)
  {
    CLog::Log(LOGERROR, "Could not find suitable input format: %s", m_strMimeType.c_str());
    avformat_close_input(&m_fctx);
    FreeIOCtx(&m_ioctx);
    return false;
  }

  if (m_fctx->nb_streams <= 0)
  {
    avformat_close_input(&m_fctx);
    FreeIOCtx(&m_ioctx);
    return false;
  }
  AVCodecParameters* codec_params = m_fctx->streams[0]->codecpar;
  AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
  m_codec_ctx = avcodec_alloc_context3(codec);
  if (!m_codec_ctx)
  {
    avformat_close_input(&m_fctx);
    FreeIOCtx(&m_ioctx);
    return false;
  }

  if (avcodec_parameters_to_context(m_codec_ctx, codec_params) < 0)
  {
    avformat_close_input(&m_fctx);
    avcodec_free_context(&m_codec_ctx);
    FreeIOCtx(&m_ioctx);
    return false;
  }

  if (avcodec_open2(m_codec_ctx, codec, NULL) < 0)
  {
    avformat_close_input(&m_fctx);
    avcodec_free_context(&m_codec_ctx);
    FreeIOCtx(&m_ioctx);
    return false;
  }

  return true;
}

AVFrame* CFFmpegImage::ExtractFrame()
{
  if (!m_fctx || !m_fctx->streams[0])
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "No valid format context or stream");
    return nullptr;
  }

  AVPacket pkt;
  AVFrame* frame = av_frame_alloc();
  int frame_decoded = 0;
  int ret = 0;
  ret = av_read_frame(m_fctx, &pkt);
  if (ret < 0)
  {
    CLog::Log(LOGDEBUG, "Error [%d] while reading frame: %s\n", ret, strerror(AVERROR(ret)));
    av_frame_free(&frame);
    av_packet_unref(&pkt);
    return nullptr;
  }

  ret = DecodeFFmpegFrame(m_codec_ctx, frame, &frame_decoded, &pkt);
  if (ret < 0 || frame_decoded == 0 || !frame)
  {
    CLog::Log(LOGDEBUG, "Error [%d] while decoding frame: %s\n", ret, strerror(AVERROR(ret)));
    av_frame_free(&frame);
    av_packet_unref(&pkt);
    return nullptr;
  }
  //we need milliseconds
  av_frame_set_pkt_duration(frame, av_rescale_q(frame->pkt_duration, m_fctx->streams[0]->time_base, AVRational{ 1, 1000 }));
  m_height = frame->height;
  m_width = frame->width;
  m_originalWidth = m_width;
  m_originalHeight = m_height;

  const AVPixFmtDescriptor* pixDescriptor = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(frame->format));
  if (pixDescriptor && ((pixDescriptor->flags & (AV_PIX_FMT_FLAG_ALPHA | AV_PIX_FMT_FLAG_PAL)) != 0))
    m_hasAlpha = true;

  AVDictionary* dic = av_frame_get_metadata(frame);
  AVDictionaryEntry* entry = NULL;
  if (dic)
  {
    entry = av_dict_get(dic, "Orientation", NULL, AV_DICT_MATCH_CASE);
    if (entry && entry->value)
    {
      int orientation = atoi(entry->value);
      // only values between including 0 and including 8
      // http://sylvana.net/jpegcrop/exif_orientation.html
      if (orientation >= 0 && orientation <= 8)
        m_orientation = (unsigned int)orientation;
    }
  }
  av_packet_unref(&pkt);

  return frame;
}

AVPixelFormat CFFmpegImage::ConvertFormats(AVFrame* frame)
{
  switch (frame->format) {
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
    return static_cast<AVPixelFormat>(frame->format);
    break;
  }
}

void CFFmpegImage::FreeIOCtx(AVIOContext** ioctx)
{
  av_freep(&((*ioctx)->buffer));
  av_freep(ioctx);
}

bool CFFmpegImage::Decode(unsigned char * const pixels, unsigned int width, unsigned int height,
                          unsigned int pitch, unsigned int format)
{
  if (m_width == 0 || m_height == 0 || format != XB_FMT_A8R8G8B8)
    return false;

  if (pixels == nullptr)
  {
    CLog::Log(LOGERROR, "%s - No valid buffer pointer (nullptr) passed", __FUNCTION__);
    return false;
  }

  if (!m_pFrame || !m_pFrame->data[0])
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "AVFrame member not allocated");
    return false;
  }

  return DecodeFrame(m_pFrame, width, height, pitch, pixels);
}

int CFFmpegImage::EncodeFFmpegFrame(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame)
{
  int ret;

  *got_packet = 0;

  ret = avcodec_send_frame(avctx, frame);
  if (ret < 0)
    return ret;

  ret = avcodec_receive_packet(avctx, pkt);
  if (!ret)
    *got_packet = 1;

  if (ret == AVERROR(EAGAIN))
    return 0;

  return ret;
}

int CFFmpegImage::DecodeFFmpegFrame(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
  int ret;

  *got_frame = 0;

  if (pkt)
  {
    ret = avcodec_send_packet(avctx, pkt);
    // In particular, we don't expect AVERROR(EAGAIN), because we read all
    // decoded frames with avcodec_receive_frame() until done.
    if (ret < 0)
      return ret;
  }

  ret = avcodec_receive_frame(avctx, frame);
  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    return ret;
  if (ret >= 0) // static code analysers would complain
   *got_frame = 1;

  return 0;
}

bool CFFmpegImage::DecodeFrame(AVFrame* frame, unsigned int width, unsigned int height, unsigned int pitch, unsigned char * const pixels)
{
  if (pixels == nullptr)
  {
    CLog::Log(LOGERROR, "%s - No valid buffer pointer (nullptr) passed", __FUNCTION__);
    return false;
  }

  AVFrame* pictureRGB = av_frame_alloc();
  if (!pictureRGB)
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "AVFrame could not be allocated");
    return false;
  }

  int size = av_image_fill_arrays(pictureRGB->data, pictureRGB->linesize, NULL, AV_PIX_FMT_RGB32, width, height, 16);
  if (size < 0)
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate AVFrame member with %i x %i pixes", width, height);
    av_frame_free(&pictureRGB);
    return false;
  }

  bool needsCopy = false;
  int pixelsSize = pitch * height;
  bool aligned = (((uintptr_t)(const void *)(pixels)) % (32) == 0);
  if (!aligned)
    CLog::Log(LOGDEBUG, "Alignment of external buffer is not suitable for ffmpeg intrinsics - please fix your malloc");

  if (aligned && size == pixelsSize && (int)pitch == pictureRGB->linesize[0])
  {
    // We can use the pixels buffer directly
    pictureRGB->data[0] = pixels;
  }
  else
  {
    // We need an extra buffer and copy it manually afterwards
    pictureRGB->format = AV_PIX_FMT_RGB32;
    pictureRGB->width = width;
    pictureRGB->height = height;
    if (av_frame_get_buffer(pictureRGB, 16) < 0)
    {
      CLog::LogFunction(LOGERROR, __FUNCTION__, "Could not allocate temp buffer of size %i bytes", size);
      av_frame_free(&pictureRGB);
      return false;
    }
    needsCopy = true;
  }

  // Especially jpeg formats are full range this we need to take care here
  // Input Formats like RGBA are handled correctly automatically
  AVColorRange range = av_frame_get_color_range(frame);
  AVPixelFormat pixFormat = ConvertFormats(frame);

  // assumption quadratic maximums e.g. 2048x2048
  float ratio = m_width / (float)m_height;
  unsigned int nHeight = m_originalHeight;
  unsigned int nWidth = m_originalWidth;
  if (nHeight > height)
  {
    nHeight = height;
    nWidth = (unsigned int)(nHeight * ratio + 0.5f);
  }
  if (nWidth > width)
  {
    nWidth = width;
    nHeight = (unsigned int)(nWidth / ratio + 0.5f);
  }

  struct SwsContext* context = sws_getContext(m_originalWidth, m_originalHeight, pixFormat,
    nWidth, nHeight, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

  if (range == AVCOL_RANGE_JPEG)
  {
    int* inv_table = nullptr;
    int* table = nullptr;
    int srcRange, dstRange, brightness, contrast, saturation;
    sws_getColorspaceDetails(context, &inv_table, &srcRange, &table, &dstRange, &brightness, &contrast, &saturation);
    srcRange = 1;
    sws_setColorspaceDetails(context, inv_table, srcRange, table, dstRange, brightness, contrast, saturation);
  }

  sws_scale(context, frame->data, frame->linesize, 0, m_originalHeight,
    pictureRGB->data, pictureRGB->linesize);
  sws_freeContext(context);

  if (needsCopy)
  {
    int minPitch = std::min((int)pitch, pictureRGB->linesize[0]);
    if (minPitch < 0)
    {
      CLog::LogFunction(LOGERROR, __FUNCTION__, "negative pitch or height");
      av_frame_free(&pictureRGB);
      return false;
    }
    const unsigned char *src = pictureRGB->data[0];
    unsigned char* dst = pixels;

    for (unsigned int y = 0; y < nHeight; y++)
    {
      memcpy(dst, src, minPitch);
      src += pictureRGB->linesize[0];
      dst += pitch;
    }
    av_frame_free(&pictureRGB);
  }
  else
  {
    // we only lended the data so don't get it deleted
    pictureRGB->data[0] = nullptr;
    av_frame_free(&pictureRGB);
  }

  // update width and height original dimensions are kept
  m_height = nHeight;
  m_width = nWidth;

  return true;
}

bool CFFmpegImage::CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width,
                                             unsigned int height, unsigned int format,
                                             unsigned int pitch,
                                             const std::string& destFile,
                                             unsigned char* &bufferout,
                                             unsigned int &bufferoutSize)
{
  // It seems XB_FMT_A8R8G8B8 mean RGBA and not ARGB
  if (format != XB_FMT_A8R8G8B8)
  {
    CLog::Log(LOGERROR, "Supplied format: %d is not supported.", format);
    return false;
  }

  bool jpg_output = false;
  if (m_strMimeType == "image/jpeg" || m_strMimeType == "image/jpg")
    jpg_output = true;
  else if (m_strMimeType == "image/png")
    jpg_output = false;
  else
  {
    CLog::Log(LOGERROR, "Output Format is not supported: %s is not supported.", destFile.c_str());
    return false;
  }

  ThumbDataManagement tdm;

  tdm.codec = avcodec_find_encoder(jpg_output ? AV_CODEC_ID_MJPEG : AV_CODEC_ID_PNG);
  if (!tdm.codec)
  {
    CLog::Log(LOGERROR, "Your are missing a working encoder for format: %d", jpg_output ? AV_CODEC_ID_MJPEG : AV_CODEC_ID_PNG);
    return false;
  }

  tdm.avOutctx = avcodec_alloc_context3(tdm.codec);
  if (!tdm.avOutctx)
  {
    CLog::Log(LOGERROR, "Could not allocate context for thumbnail: %s", destFile.c_str());
    return false;
  }

  tdm.avOutctx->height = height;
  tdm.avOutctx->width = width;
  tdm.avOutctx->time_base.num = 1;
  tdm.avOutctx->time_base.den = 1;
  tdm.avOutctx->pix_fmt = jpg_output ? AV_PIX_FMT_YUVJ420P : AV_PIX_FMT_RGBA;
  tdm.avOutctx->flags = CODEC_FLAG_QSCALE;
  tdm.avOutctx->mb_lmin = tdm.avOutctx->qmin * FF_QP2LAMBDA;
  tdm.avOutctx->mb_lmax = tdm.avOutctx->qmax * FF_QP2LAMBDA;
  tdm.avOutctx->global_quality = tdm.avOutctx->qmin * FF_QP2LAMBDA;

  unsigned int internalBufOutSize = 0;

  int size = av_image_get_buffer_size(tdm.avOutctx->pix_fmt, tdm.avOutctx->width, tdm.avOutctx->height, 16);
  if (size < 0)
  {
    CLog::Log(LOGERROR, "Could not compute picture size for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }
  internalBufOutSize = (unsigned int) size;

  tdm.intermediateBuffer = (uint8_t*) av_malloc(internalBufOutSize);
  if (!tdm.intermediateBuffer)
  {
    CLog::Log(LOGERROR, "Could not allocate memory for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }

  if (avcodec_open2(tdm.avOutctx, tdm.codec, NULL) < 0)
  {
    CLog::Log(LOGERROR, "Could not open avcodec context thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }

  tdm.frame_input = av_frame_alloc();
  if (!tdm.frame_input)
  {
    CLog::Log(LOGERROR, "Could not allocate frame for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }

  // convert the RGB32 frame to AV_PIX_FMT_YUV420P - we use this later on as AV_PIX_FMT_YUVJ420P
  tdm.frame_temporary = av_frame_alloc();
  if (!tdm.frame_temporary)
  {
    CLog::Log(LOGERROR, "Could not allocate frame for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }

  if (av_image_fill_arrays(tdm.frame_temporary->data, tdm.frame_temporary->linesize, tdm.intermediateBuffer, jpg_output ? AV_PIX_FMT_YUV420P : AV_PIX_FMT_RGBA, width, height, 16) < 0)
  {
    CLog::Log(LOGERROR, "Could not fill picture for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }

  uint8_t* src[] = { bufferin, NULL, NULL, NULL };
  int srcStride[] = { (int) pitch, 0, 0, 0};

  //input size == output size which means only pix_fmt conversion
  tdm.sws = sws_getContext(width, height, AV_PIX_FMT_RGB32, width, height, jpg_output ? AV_PIX_FMT_YUV420P : AV_PIX_FMT_RGBA, 0, 0, 0, 0);
  if (!tdm.sws)
  {
    CLog::Log(LOGERROR, "Could not setup scaling context for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }

  // Setup jpeg range for sws
  if (jpg_output)
  {
    int* inv_table = nullptr;
    int* table = nullptr;
    int srcRange, dstRange, brightness, contrast, saturation;

    if (sws_getColorspaceDetails(tdm.sws, &inv_table, &srcRange, &table, &dstRange, &brightness, &contrast, &saturation) < 0)
    {
      CLog::Log(LOGERROR, "SWS_SCALE failed to get ColorSpaceDetails for thumbnail: %s", destFile.c_str());
      CleanupLocalOutputBuffer();
      return false;
    }
    dstRange = 1; // jpeg full range yuv420p output
    srcRange = 0; // full range RGB32 input
    if (sws_setColorspaceDetails(tdm.sws, inv_table, srcRange, table, dstRange, brightness, contrast, saturation) < 0)
    {
      CLog::Log(LOGERROR, "SWS_SCALE failed to set ColorSpace Details for thumbnail: %s", destFile.c_str());
      CleanupLocalOutputBuffer();
      return false;
    }
  }

  if (sws_scale(tdm.sws, src, srcStride, 0, height, tdm.frame_temporary->data, tdm.frame_temporary->linesize) < 0)
  {
    CLog::Log(LOGERROR, "SWS_SCALE failed for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }
  tdm.frame_input->pts = 1;
  tdm.frame_input->quality = tdm.avOutctx->global_quality;
  tdm.frame_input->data[0] = (uint8_t*) tdm.frame_temporary->data[0];
  tdm.frame_input->data[1] = (uint8_t*) tdm.frame_temporary->data[1];
  tdm.frame_input->data[2] = (uint8_t*) tdm.frame_temporary->data[2];
  tdm.frame_input->height = height;
  tdm.frame_input->width = width;
  tdm.frame_input->linesize[0] = tdm.frame_temporary->linesize[0];
  tdm.frame_input->linesize[1] = tdm.frame_temporary->linesize[1];
  tdm.frame_input->linesize[2] = tdm.frame_temporary->linesize[2];
  // this is deprecated but mjpeg is not yet transitioned
  tdm.frame_input->format = jpg_output ? AV_PIX_FMT_YUVJ420P : AV_PIX_FMT_RGBA;

  int got_package = 0;
  AVPacket avpkt;
  av_init_packet(&avpkt);
  // encoder will allocate memory
  avpkt.data = nullptr;
  avpkt.size = 0;

  int ret = EncodeFFmpegFrame(tdm.avOutctx, &avpkt, &got_package, tdm.frame_input);

  if ((ret < 0) || (got_package == 0))
  {
    CLog::Log(LOGERROR, "Could not encode thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    return false;
  }

  bufferoutSize = avpkt.size;
  m_outputBuffer = (uint8_t*) av_malloc(bufferoutSize);
  if (!m_outputBuffer)
  {
    CLog::Log(LOGERROR, "Could not generate allocate memory for thumbnail: %s", destFile.c_str());
    CleanupLocalOutputBuffer();
    av_packet_unref(&avpkt);
    return false;
  }
  // update buffer ptr for caller
  bufferout = m_outputBuffer;

  // copy avpkt data into outputbuffer
  memcpy(m_outputBuffer, avpkt.data, avpkt.size);
  av_packet_unref(&avpkt);

  return true;
}

void CFFmpegImage::ReleaseThumbnailBuffer()
{
  CleanupLocalOutputBuffer();
}

void CFFmpegImage::CleanupLocalOutputBuffer()
{
  av_free(m_outputBuffer);
  m_outputBuffer = nullptr;
}

std::shared_ptr<Frame> CFFmpegImage::ReadFrame()
{
  AVFrame* avframe = ExtractFrame();
  if (avframe == nullptr)
    return nullptr;
  std::shared_ptr<Frame> frame(new Frame());
  frame->m_delay = (unsigned int)av_frame_get_pkt_duration(avframe);
  frame->m_pitch = avframe->width * 4;
  frame->m_pImage = new unsigned char[avframe->height * frame->m_pitch];
  DecodeFrame(avframe, avframe->width, avframe->height, frame->m_pitch, frame->m_pImage);
  av_frame_free(&avframe);
  return frame;
}
