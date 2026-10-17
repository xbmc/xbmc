/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaBackendWasm.h"

#include "MediaBackendEmss.h"

#include "platform/wasm/TizenEmss.h"

#include <algorithm>

#include <fmt/format.h>

extern "C"
{
#include <libavcodec/defs.h>
}

std::unique_ptr<IMediaBackendWasm> IMediaBackendWasm::Create()
{
  if (tizen_emss_available())
    return std::make_unique<CMediaBackendEmss>();
  return nullptr;
}

namespace WASM_MEDIA
{
std::string VideoCodecString(const CDVDStreamInfo& hint)
{
  const int bitDepth = hint.bitdepth > 8 ? hint.bitdepth : 8;
  const bool uhd = hint.height > 1088;

  switch (hint.codec)
  {
    case AV_CODEC_ID_H264:
    {
      // An avcC record: version 1, then profile, constraint flags and level.
      const uint8_t* avcc = hint.extradata.GetData();
      if (hint.extradata.GetSize() >= 4 && avcc[0] == 1)
        return fmt::format("avc1.{:02x}{:02x}{:02x}", avcc[1], avcc[2], avcc[3]);
      const int profile = hint.profile > 0 ? hint.profile & 0xff : AV_PROFILE_H264_HIGH;
      const int level = hint.level > 0 ? hint.level : 41;
      return fmt::format("avc1.{:02x}00{:02x}", profile, level);
    }
    case AV_CODEC_ID_HEVC:
    {
      const bool main10 = hint.profile == AV_PROFILE_HEVC_MAIN_10 || bitDepth > 8;
      const int level = hint.level > 0 ? hint.level : (uhd ? 153 : 123);
      return fmt::format("hev1.{}.{}.L{}.B0", main10 ? 2 : 1, main10 ? 4 : 6, level);
    }
    case AV_CODEC_ID_VP9:
    {
      const int profile = hint.profile > 0 ? hint.profile : 0;
      const int level = hint.level > 0 ? hint.level : (uhd ? 51 : 41);
      return fmt::format("vp09.{:02}.{:02}.{:02}", profile, level, bitDepth);
    }
    case AV_CODEC_ID_AV1:
    {
      const int profile = hint.profile > 0 ? hint.profile : 0;
      const int level = hint.level >= 0 ? hint.level : (uhd ? 12 : 8);
      return fmt::format("av01.{}.{:02}M.{:02}", profile, level, bitDepth);
    }
    default:
      return {};
  }
}

namespace
{
// Whether the NAL unit with this header starts a key frame (1), another picture (0), or
// says nothing about it (-1).
int NalKeyFrame(AVCodecID codec, uint8_t header)
{
  if (codec == AV_CODEC_ID_H264)
  {
    const int type = header & 0x1f;
    if (type == 5)
      return 1;
    return type >= 1 && type <= 4 ? 0 : -1;
  }
  const int type = (header >> 1) & 0x3f;
  if (type >= 16 && type <= 21)
    return 1;
  return type <= 9 ? 0 : -1;
}
} // namespace

bool IsKeyFrame(AVCodecID codec, const uint8_t* data, size_t size, int nalLengthSize)
{
  switch (codec)
  {
    case AV_CODEC_ID_H264:
    case AV_CODEC_ID_HEVC:
      // The first slice decides.
      if (nalLengthSize > 0)
      {
        for (size_t pos = 0; pos + nalLengthSize < size;)
        {
          size_t length = 0;
          for (int i = 0; i < nalLengthSize; ++i)
            length = (length << 8) | data[pos + i];
          pos += nalLengthSize;
          if (length == 0 || pos >= size)
            return false;
          const int key = NalKeyFrame(codec, data[pos]);
          if (key >= 0)
            return key == 1;
          pos += length;
        }
        return false;
      }
      for (size_t i = 0; i + 3 < size; ++i)
      {
        if (data[i] != 0 || data[i + 1] != 0 || data[i + 2] != 1)
          continue;
        const int key = NalKeyFrame(codec, data[i + 3]);
        if (key >= 0)
          return key == 1;
        i += 2;
      }
      return false;
    case AV_CODEC_ID_VP9:
    {
      // frame_marker(2) profile(2) [reserved(1) in profile 3] show_existing_frame(1)
      // frame_type(1), where frame_type 0 is a key frame
      if (size == 0)
        return false;
      const int shift = (data[0] & 0x30) == 0x30 ? 2 : 3;
      return ((data[0] >> shift) & 1) == 0 && ((data[0] >> (shift - 1)) & 1) == 0;
    }
    case AV_CODEC_ID_AV1:
    {
      // The first frame header: show_existing_frame(1) frame_type(2), where frame_type
      // 0 is a key frame. Streams with a reduced still picture header are not expected.
      size_t pos = 0;
      while (pos < size)
      {
        const uint8_t header = data[pos++];
        const int type = (header >> 3) & 0x0f;
        if (header & 0x04)
          pos++;
        size_t obuSize = size - std::min(pos, size);
        if (header & 0x02)
        {
          obuSize = 0;
          for (int shift = 0; pos < size && shift < 56; shift += 7)
          {
            const uint8_t byte = data[pos++];
            obuSize |= static_cast<size_t>(byte & 0x7f) << shift;
            if (!(byte & 0x80))
              break;
          }
        }
        if (pos >= size)
          return false;
        constexpr int OBU_FRAME_HEADER = 3;
        constexpr int OBU_FRAME = 6;
        if (type == OBU_FRAME_HEADER || type == OBU_FRAME)
          return !(data[pos] & 0x80) && ((data[pos] >> 5) & 0x03) == 0;
        pos += obuSize;
      }
      return false;
    }
    default:
      return true;
  }
}
} // namespace WASM_MEDIA
