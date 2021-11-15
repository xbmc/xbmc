/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Encoder.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

namespace KODI
{
namespace CDRIP
{

class CEncoderFFmpeg : public CEncoder
{
public:
  CEncoderFFmpeg() = default;
  ~CEncoderFFmpeg() override = default;

  bool Init() override;
  ssize_t Encode(uint8_t* pbtStream, size_t nNumBytesRead) override;
  bool Close() override;

private:
  static int avio_write_callback(void* opaque, uint8_t* buf, int buf_size);
  static int64_t avio_seek_callback(void* opaque, int64_t offset, int whence);
  void SetTag(const std::string& tag, const std::string& value);
  bool WriteFrame();

  AVFormatContext* m_Format{nullptr};
  AVCodecContext* m_CodecCtx{nullptr};
  SwrContext* m_SwrCtx{nullptr};
  AVStream* m_Stream{nullptr};
  AVSampleFormat m_InFormat;
  AVSampleFormat m_OutFormat;

  /* From libavformat/avio.h:
   * The buffer size is very important for performance.
   * For protocols with fixed blocksize it should be set to this
   * blocksize.
   * For others a typical size is a cache page, e.g. 4kb.
   */
  unsigned char m_BCBuffer[4096];

  unsigned int m_NeededFrames;
  size_t m_NeededBytes;
  uint8_t* m_Buffer{nullptr};
  size_t m_BufferSize{0};
  AVFrame* m_BufferFrame{nullptr};
  uint8_t* m_ResampledBuffer{nullptr};
  size_t m_ResampledBufferSize{0};
  AVFrame* m_ResampledFrame{nullptr};
  bool m_NeedConversion{false};
};

} /* namespace CDRIP */
} /* namespace KODI */
