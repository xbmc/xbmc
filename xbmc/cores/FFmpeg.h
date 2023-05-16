/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceBroker.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>
#include <libavutil/ffversion.h>
#include <libavfilter/avfilter.h>
#include <libpostproc/postprocess.h>
}

#include <tuple>

namespace FFMPEG_HELP_TOOLS
{

struct FFMpegException : public std::exception
{
  std::string s;
  template<typename... Args>
  FFMpegException(const std::string& fmt, Args&&... args)
    : s(StringUtils::Format(fmt, std::forward<Args>(args)...))
  {
  }
  const char* what() const noexcept override { return s.c_str(); }
};

std::string FFMpegErrorToString(int err);

} // namespace FFMPEG_HELP_TOOLS

inline int PPCPUFlags()
{
  unsigned int cpuFeatures = CServiceBroker::GetCPUInfo()->GetCPUFeatures();
  int flags = 0;

  if (cpuFeatures & CPU_FEATURE_MMX)
    flags |= PP_CPU_CAPS_MMX;
  if (cpuFeatures & CPU_FEATURE_MMX2)
    flags |= PP_CPU_CAPS_MMX2;
  if (cpuFeatures & CPU_FEATURE_3DNOW)
    flags |= PP_CPU_CAPS_3DNOW;
  if (cpuFeatures & CPU_FEATURE_ALTIVEC)
    flags |= PP_CPU_CAPS_ALTIVEC;

  return flags;
}

// callback used for logging
void ff_avutil_log(void* ptr, int level, const char* format, va_list va);
void ff_flush_avutil_log_buffers(void);

class CFFmpegLog
{
public:
  static void SetLogLevel(int level);
  static int GetLogLevel();
  static void ClearLogLevel();
  int level;
};

class FFmpegExtraData
{
public:
  FFmpegExtraData() = default;
  explicit FFmpegExtraData(size_t size);
  FFmpegExtraData(const uint8_t* data, size_t size);
  FFmpegExtraData(const FFmpegExtraData& other);
  FFmpegExtraData(FFmpegExtraData&& other) noexcept;

  ~FFmpegExtraData();

  FFmpegExtraData& operator=(const FFmpegExtraData& other);
  FFmpegExtraData& operator=(FFmpegExtraData&& other) noexcept;

  bool operator==(const FFmpegExtraData& other) const;
  bool operator!=(const FFmpegExtraData& other) const;

  operator bool() const { return m_data != nullptr && m_size != 0; }
  uint8_t* GetData() { return m_data; }
  const uint8_t* GetData() const { return m_data; }
  size_t GetSize() const { return m_size; }
  /*!
   * \brief Take ownership over the extra data buffer
   *
   * It's in the responsibility of the caller to free the buffer with av_free. After the call
   * FFmpegExtraData is empty.
   *
   * \return The extra data buffer or nullptr if FFmpegExtraData is empty.
   */
  uint8_t* TakeData();

private:
  uint8_t* m_data{nullptr};
  size_t m_size{};
};

FFmpegExtraData GetPacketExtradata(const AVPacket* pkt, const AVCodecParameters* codecPar);
