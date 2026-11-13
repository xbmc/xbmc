/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UltraHdr.h"

#include "PictureXmp.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

using namespace KODI::PICTURE;

namespace
{
float GetFloat(std::string_view xmp, std::string_view name, float fallback)
{
  const std::string_view value = XMP::GetValue(xmp, name);
  return value.empty() ? fallback : StringUtils::ToFloat(value, fallback);
}

// one value for all three channels, or one for each
GainMapChannels GetChannels(std::string_view xmp,
                            std::string_view name,
                            const GainMapChannels& fallback)
{
  const std::vector<std::string_view> values = XMP::GetValues(xmp, name);
  if (values.size() != 1 && values.size() != 3)
    return fallback;
  GainMapChannels channels{};
  for (size_t c = 0; c < channels.size(); ++c)
    channels[c] = StringUtils::ToFloat(values[values.size() == 1 ? 0 : c], fallback[c]);
  return channels;
}

uint32_t GetUnsigned(std::string_view data, size_t pos, size_t bytes, bool littleEndian)
{
  uint32_t value = 0;
  for (size_t i = 0; i < bytes; ++i)
  {
    const auto byte = static_cast<uint8_t>(data[pos + (littleEndian ? bytes - 1 - i : i)]);
    value = (value << 8) | byte;
  }
  return value;
}

//! \brief Find the gain map from the MP entries of an MPF index, whose offsets count from \p base
bool ParseMpf(
    std::string_view tiff, uint64_t base, uint64_t fileSize, uint64_t& offset, uint64_t& size)
{
  if (tiff.size() < 8 || (!tiff.starts_with("II") && !tiff.starts_with("MM")))
    return false;
  const bool littleEndian = tiff.starts_with("II");

  // 64-bit sums: tiff.size() - n wraps for a short TIFF, a size_t sum on 32-bit builds
  const uint32_t ifd = GetUnsigned(tiff, 4, 4, littleEndian);
  if (static_cast<uint64_t>(ifd) + 2 > tiff.size())
    return false;
  const uint32_t count = GetUnsigned(tiff, ifd, 2, littleEndian);
  for (uint32_t i = 0; i < count; ++i)
  {
    const size_t entry = ifd + 2 + 12 * static_cast<size_t>(i);
    if (entry + 12 > tiff.size())
      return false;
    if (GetUnsigned(tiff, entry, 2, littleEndian) != 0xB002)
      continue;

    // The gain map is the second picture
    const uint32_t bytes = GetUnsigned(tiff, entry + 4, 4, littleEndian);
    const uint32_t entries = GetUnsigned(tiff, entry + 8, 4, littleEndian);
    if (bytes < 32 || static_cast<uint64_t>(entries) + 32 > tiff.size())
      return false;
    size = GetUnsigned(tiff, entries + 20, 4, littleEndian);
    const uint32_t relative = GetUnsigned(tiff, entries + 24, 4, littleEndian);
    offset = base + relative;
    return relative > 0 && size > 0 && offset < fileSize && size <= fileSize - offset;
  }
  return false;
}

//! \brief Find the gain map from the MPF index in the primary picture's APP2 segment
bool FindGainMapInMpf(XFILE::CFile& file, uint64_t fileSize, uint64_t& offset, uint64_t& size)
{
  int64_t pos = 2;
  uint8_t marker[4];
  while (file.Seek(pos) == pos &&
         file.Read(marker, sizeof(marker)) == static_cast<ssize_t>(sizeof(marker)) &&
         marker[0] == 0xFF && marker[1] != 0xDA && marker[1] != 0xD9)
  {
    const size_t length = (marker[2] << 8) | marker[3];
    if (length < 2)
      return false;
    if (marker[1] == 0xE2)
    {
      std::string segment(length - 2, '\0');
      if (file.Read(segment.data(), segment.size()) != static_cast<ssize_t>(segment.size()))
        return false;
      if (segment.starts_with(std::string_view("MPF\0", 4)))
        return ParseMpf(std::string_view(segment).substr(4), pos + 8, fileSize, offset, size);
    }
    pos += 2 + static_cast<int64_t>(length);
  }
  return false;
}

//! \brief Find the gain map from the lengths of the XMP container's items after the primary picture
bool FindGainMapInXmp(std::string_view xmp, uint64_t fileSize, uint64_t& offset, uint64_t& size)
{
  uint64_t offsetFromEnd = 0;
  size = 0;
  for (const auto& item : XMP::GetContainerItems(xmp))
  {
    if (item.semantic == "GainMap" && size == 0)
      size = item.length;
    if (size == 0)
      continue;
    if (item.length > fileSize || item.padding > fileSize - item.length ||
        offsetFromEnd > fileSize - item.length - item.padding)
      return false;
    offsetFromEnd += item.length + item.padding;
  }

  if (size == 0 || offsetFromEnd >= fileSize)
    return false;
  offset = fileSize - offsetFromEnd;
  return true;
}
} // namespace

GainMap KODI::PICTURE::ReadGainMap(const std::string& path)
{
  if (!URIUtils::HasExtension(path, ".jpg|.jpeg"))
    return {};

  XFILE::CFile file;
  if (!file.Open(path))
    return {};

  const std::string xmp = XMP::Read(file);
  if (XMP::GetValue(xmp, "hdrgm:Version").empty())
    return {};

  const int64_t fileSize = file.GetLength();
  if (fileSize <= 0)
    return {};
  const uint64_t limit = static_cast<uint64_t>(fileSize);

  // The MPF index is right even when an editor has dropped a Motion Photo's clip but left its
  // length in the XMP container
  uint64_t start = 0;
  uint64_t size = 0;
  if (!FindGainMapInMpf(file, limit, start, size) && !FindGainMapInXmp(xmp, limit, start, size))
    return {};

  GainMap gainMap;
  gainMap.image.resize(static_cast<size_t>(size));
  const auto offset = static_cast<int64_t>(start);
  if (file.Seek(offset) != offset ||
      file.Read(gainMap.image.data(), gainMap.image.size()) !=
          static_cast<ssize_t>(gainMap.image.size()) ||
      !gainMap.image.starts_with("\xFF\xD8"))
  {
    CLog::Log(LOGDEBUG, "UltraHdr: {} has no JPEG where its XMP says the gain map is", path);
    return {};
  }

  // The gain map's own XMP says how to apply it
  file.Seek(offset);
  const std::string gainMapXmp = XMP::Read(file);
  if (XMP::GetValue(gainMapXmp, "hdrgm:Version").empty() ||
      StringUtils::EqualsNoCase(XMP::GetValue(gainMapXmp, "hdrgm:BaseRenditionIsHDR"), "True"))
    return {};

  GainMapParams& params = gainMap.params;
  params.min = GetChannels(gainMapXmp, "hdrgm:GainMapMin", params.min);
  params.max = GetChannels(gainMapXmp, "hdrgm:GainMapMax", params.max);
  params.gamma = GetChannels(gainMapXmp, "hdrgm:Gamma", params.gamma);
  params.offsetSdr = GetChannels(gainMapXmp, "hdrgm:OffsetSDR", params.offsetSdr);
  params.offsetHdr = GetChannels(gainMapXmp, "hdrgm:OffsetHDR", params.offsetHdr);
  params.capacityMin = GetFloat(gainMapXmp, "hdrgm:HDRCapacityMin", params.capacityMin);
  params.capacityMax = GetFloat(gainMapXmp, "hdrgm:HDRCapacityMax", params.capacityMax);
  const auto finite = [](float value) { return std::isfinite(value); };
  for (const GainMapChannels& channels :
       {params.min, params.max, params.gamma, params.offsetSdr, params.offsetHdr})
  {
    if (!std::ranges::all_of(channels, finite))
      return {};
  }
  if (!finite(params.capacityMin) || !finite(params.capacityMax) ||
      std::ranges::any_of(params.gamma, [](float gamma) { return gamma <= 0.0f; }) ||
      params.capacityMax <= params.capacityMin)
    return {};

  return gainMap;
}
