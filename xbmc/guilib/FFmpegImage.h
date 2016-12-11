#pragma once
/*
*      Copyright (C) 2012-2015 Team Kodi
*      http://kodi.tv
*
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

#include "iimage.h"
#include <memory>

extern "C"
{
#include "libavutil/pixfmt.h"
}

class Frame
{
public:
  friend class CFFmpegImage;

  Frame();
  virtual ~Frame();

  int GetPitch() const { return m_pitch; }

  unsigned char* m_pImage;
  unsigned int m_delay;

private:
  Frame(const Frame& src);

  int m_pitch = 0;
  unsigned int m_imageSize;
  unsigned int m_height;
  unsigned int m_width;
};


struct MemBuffer
{
  uint8_t* data = nullptr;
  size_t size = 0;
  size_t pos = 0;
};

struct AVFrame;
struct AVIOContext;
struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;

class CFFmpegImage : public IImage
{
public:
  explicit CFFmpegImage(const std::string& strMimeType);
  virtual ~CFFmpegImage();

  virtual bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                                   unsigned int width, unsigned int height);
  virtual bool Decode(unsigned char * const pixels, unsigned int width, unsigned int height,
                      unsigned int pitch, unsigned int format);
  virtual bool CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width,
                                          unsigned int height, unsigned int format,
                                          unsigned int pitch, const std::string& destFile, 
                                          unsigned char* &bufferout,
                                          unsigned int &bufferoutSize);
  virtual void ReleaseThumbnailBuffer();

  bool Initialize(unsigned char* buffer, unsigned int bufSize);

  std::shared_ptr<Frame> ReadFrame();

private:
  static void FreeIOCtx(AVIOContext** ioctx);
  AVFrame* ExtractFrame();
  bool DecodeFrame(AVFrame* m_pFrame, unsigned int width, unsigned int height, unsigned int pitch, unsigned char * const pixels);
  static int EncodeFFmpegFrame(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame);
  static int DecodeFFmpegFrame(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt);
  static AVPixelFormat ConvertFormats(AVFrame* frame);
  std::string m_strMimeType;
  void CleanupLocalOutputBuffer();


  MemBuffer m_buf;
  uint32_t m_frames = 0;

  AVIOContext* m_ioctx = nullptr;
  AVFormatContext* m_fctx = nullptr;
  AVCodecContext* m_codec_ctx = nullptr;

  AVFrame* m_pFrame;
  uint8_t* m_outputBuffer;
};
