/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "iimage.h"

#include <cstdint>
#include <memory>

extern "C"
{
#include <libavutil/pixfmt.h>
}

class Frame
{
public:
  friend class CFFmpegImage;

  Frame() = default;
  virtual ~Frame();

  int GetPitch() const { return m_pitch; }

  unsigned char* m_pImage = nullptr;
  unsigned int m_delay = 0;

private:
  Frame(const Frame& src);

  int m_pitch = 0;
  unsigned int m_imageSize = 0;
  unsigned int m_height = 0;
  unsigned int m_width = 0;
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
  ~CFFmpegImage() override;

  bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                           unsigned int width, unsigned int height) override;
  bool Decode(unsigned char * const pixels, unsigned int width, unsigned int height,
              unsigned int pitch, unsigned int format) override;
  bool CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width,
                                  unsigned int height, unsigned int format,
                                  unsigned int pitch, const std::string& destFile,
                                  unsigned char* &bufferout,
                                  unsigned int &bufferoutSize) override;
  void ReleaseThumbnailBuffer() override;

  bool Initialize(unsigned char* buffer, size_t bufSize);

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

  AVIOContext* m_ioctx = nullptr;
  AVFormatContext* m_fctx = nullptr;
  AVCodecContext* m_codec_ctx = nullptr;

  AVFrame* m_pFrame;
  uint8_t* m_outputBuffer;
};
