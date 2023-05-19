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
  AVSampleFormat GetInputFormat(int inBitsPerSample);

  AVFormatContext* m_formatCtx{nullptr};
  AVCodecContext* m_codecCtx{nullptr};
  SwrContext* m_swrCtx{nullptr};
  AVStream* m_stream{nullptr};
  AVSampleFormat m_inFormat;
  AVSampleFormat m_outFormat;

  /* From libavformat/avio.h:
   * The buffer size is very important for performance.
   * For protocols with fixed blocksize it should be set to this
   * blocksize.
   * For others a typical size is a cache page, e.g. 4kb.
   */
  static constexpr size_t BUFFER_SIZE = 4096;

  unsigned int m_neededFrames{0};
  size_t m_neededBytes{0};
  uint8_t* m_buffer{nullptr};
  size_t m_bufferSize{0};
  AVFrame* m_bufferFrame{nullptr};
  uint8_t* m_resampledBuffer{nullptr};
  size_t m_resampledBufferSize{0};
  AVFrame* m_resampledFrame{nullptr};
  bool m_needConversion{false};
  int64_t m_samplesCount{0};
  int64_t m_samplesCountMultiply{1000};
};

} /* namespace CDRIP */
} /* namespace KODI */
