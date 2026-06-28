/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "savestate_generated.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <flatbuffers/flatbuffers.h>

namespace KODI
{
namespace RETRO
{
struct SavestateBlobOffsets
{
  flatbuffers::Offset<flatbuffers::Vector<uint8_t>> raw;
  flatbuffers::Offset<flatbuffers::Vector<uint8_t>> compressed;
  SAVESTATE::CompressionType compressionType{SAVESTATE::CompressionType_None};
  uint64_t uncompressedSize{0};
};

struct PendingSavestateBlob
{
  std::vector<uint8_t> raw;
  std::vector<uint8_t> compressed;
  uint64_t uncompressedSize{0};
  SAVESTATE::CompressionType compression{SAVESTATE::CompressionType_None};

  void Clear();
  bool HasCompressedData() const;
};

class CSavestateBlob
{
public:
  static SavestateBlobOffsets CreateWriteOffsets(flatbuffers::FlatBufferBuilder& builder,
                                                 const std::vector<uint8_t>& rawData,
                                                 const char* fieldName,
                                                 bool compress);

  static SavestateBlobOffsets CreateWriteOffsets(flatbuffers::FlatBufferBuilder& builder,
                                                 const PendingSavestateBlob& pending,
                                                 const char* fieldName,
                                                 bool compress);

  static bool PrepareMemoryData(const SAVESTATE::Savestate& savestate,
                                size_t expectedSize,
                                std::vector<uint8_t>& decompressedMemoryData);

  static bool PrepareVideoData(const SAVESTATE::Savestate& savestate,
                               std::vector<uint8_t>& decompressedVideoData);

  static bool IsValidRawMemoryData(const SAVESTATE::Savestate& savestate, size_t expectedSize);
  static bool HasRawVideoData(const SAVESTATE::Savestate& savestate);
  static bool IsValidCopiedCompressedMemoryData(const SAVESTATE::Savestate& savestate);
  static bool IsValidMemoryDataSize(size_t size);
  static bool IsSupportedMemorySize(size_t expectedSize);
};
} // namespace RETRO
} // namespace KODI
