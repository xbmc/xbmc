/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"
#include "filesystem/DiscDirectoryHelper.h"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace XFILE
{
enum class ENCODING_TYPE : uint8_t;
struct BlurayPlaylistInformation;

struct Descriptor
{
  unsigned int tag;
  int length;
  std::vector<std::byte> data;
};

struct TSStreamInfo
{
  unsigned int pid{};
  ENCODING_TYPE streamType{};
  std::vector<Descriptor> descriptors{};

  // Determine if details complete
  unsigned int seen{0};
  bool completed{false};

  // Methods
  TSStreamInfo() = default;
  virtual ~TSStreamInfo() = default;
  TSStreamInfo(const TSStreamInfo&) = default;
  TSStreamInfo& operator=(const TSStreamInfo&) = default;
  TSStreamInfo(TSStreamInfo&&) noexcept = default;
  TSStreamInfo& operator=(TSStreamInfo&&) noexcept = default;
};

struct TSAudioStreamInfo : TSStreamInfo
{
  unsigned int channels{0};
  unsigned int sampleRate{0};

  // DTS
  bool isXLL{false}; // DTS-HD MA - needs to be true for DTS:X
  bool hasSubstream{false}; // Needs to be true for DTS:X
  bool isXLLX{false};
  bool isXLLXIMAX{false};

  // AC3 / Dolby
  bool hasDependantStream{false};
  bool isAtmos{false};
};

struct TSVideoStreamInfo : TSStreamInfo
{
  unsigned int height{0};
  unsigned int width{0};
  unsigned int bitDepth{0};
  float aspectRatio{0.0};
  bool is3d{false};

  bool hdr10{false};
  bool hdr10Plus{false};
  bool dolbyVision{false};
  bool isEnhancementLayer{false};
};

using StreamMap = std::unordered_map<unsigned int, std::shared_ptr<TSStreamInfo>>;

class CM2TSParser
{
public:
  static bool GetStreams(const CURL& url,
                         BlurayPlaylistInformation& playlistInformation,
                         StreamMap& streams);

  static bool GetStreamsFromFile(const std::string& path,
                                 unsigned int clip,
                                 const std::string& clipExtension,
                                 StreamMap& streams);

  static std::vector<std::reference_wrapper<TSVideoStreamInfo>> GetVideoStreams(
      const StreamMap& streams);
  static std::vector<std::reference_wrapper<TSAudioStreamInfo>> GetAudioStreams(
      const StreamMap& streams);
  static std::vector<std::reference_wrapper<TSStreamInfo>> GetSubtitleStreams(
      const StreamMap& streams);
};
} // namespace XFILE
