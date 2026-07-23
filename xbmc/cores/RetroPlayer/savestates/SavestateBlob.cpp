/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateBlob.h"

#include "SavestateCompression.h"
#include "utils/log.h"

#include <limits>

using namespace KODI;
using namespace RETRO;

namespace
{
constexpr size_t MAX_SAVESTATE_MEMORY_SIZE = 512 * 1024 * 1024;
constexpr size_t MAX_SAVESTATE_VIDEO_SIZE = 128 * 1024 * 1024;

bool GetVideoBytesPerPixel(SAVESTATE::PixelFormat pixelFormat, size_t& bytesPerPixel)
{
  switch (pixelFormat)
  {
    case SAVESTATE::PixelFormat_RGBA_8888:
    case SAVESTATE::PixelFormat_XRGB_8888:
    case SAVESTATE::PixelFormat_BGRX_8888:
      bytesPerPixel = 4;
      return true;

    case SAVESTATE::PixelFormat_RGB_565_BE:
    case SAVESTATE::PixelFormat_RGB_565_LE:
    case SAVESTATE::PixelFormat_RGB_555_BE:
    case SAVESTATE::PixelFormat_RGB_555_LE:
      bytesPerPixel = 2;
      return true;

    default:
      break;
  }

  return false;
}

bool GetPackedMinimumVideoSize(const SAVESTATE::Savestate& savestate, size_t& packedMinimumSize)
{
  const unsigned int width = savestate.video_width();
  const unsigned int height = savestate.video_height();
  size_t bytesPerPixel = 0;

  if (width == 0 || height == 0 || !GetVideoBytesPerPixel(savestate.pixel_format(), bytesPerPixel))
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid video metadata in savestate");
    return false;
  }

  if (width > std::numeric_limits<size_t>::max() / height)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Video dimensions overflow size calculation");
    return false;
  }

  const size_t pixels = static_cast<size_t>(width) * height;
  if (pixels > std::numeric_limits<size_t>::max() / bytesPerPixel)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Video size overflows size calculation");
    return false;
  }

  packedMinimumSize = pixels * bytesPerPixel;
  if (packedMinimumSize == 0 || packedMinimumSize > MAX_SAVESTATE_VIDEO_SIZE)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Video minimum size {} exceeds limit {}",
              packedMinimumSize, MAX_SAVESTATE_VIDEO_SIZE);
    return false;
  }

  return true;
}
} // namespace

void PendingSavestateBlob::Clear()
{
  raw.clear();
  compressed.clear();
  uncompressedSize = 0;
  compression = SAVESTATE::CompressionType_None;
}

bool PendingSavestateBlob::HasCompressedData() const
{
  return compression == SAVESTATE::CompressionType_Zstd && !compressed.empty();
}

SavestateBlobOffsets CSavestateBlob::CreateWriteOffsets(flatbuffers::FlatBufferBuilder& builder,
                                                        const std::vector<uint8_t>& rawData,
                                                        const char* fieldName,
                                                        bool compress)
{
  SavestateBlobOffsets offsets;

  if (compress && !rawData.empty())
  {
    std::vector<uint8_t> compressedData;
    if (CSavestateCompression::CompressZstdIfSmaller(rawData.data(), rawData.size(),
                                                     compressedData))
    {
      const std::vector<uint8_t> emptyData;
      offsets.raw = builder.CreateVector(emptyData);
      offsets.compressed = builder.CreateVector(compressedData);
      offsets.compressionType = SAVESTATE::CompressionType_Zstd;
      offsets.uncompressedSize = rawData.size();

      CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Compressed {} from {} to {} bytes using zstd",
                fieldName ? fieldName : "blob", rawData.size(), compressedData.size());

      return offsets;
    }
  }

  offsets.raw = builder.CreateVector(rawData);
  offsets.compressionType = SAVESTATE::CompressionType_None;
  offsets.uncompressedSize = 0;
  return offsets;
}

SavestateBlobOffsets CSavestateBlob::CreateWriteOffsets(flatbuffers::FlatBufferBuilder& builder,
                                                        const PendingSavestateBlob& pending,
                                                        const char* fieldName,
                                                        bool compress)
{
  if (!compress)
  {
    if (pending.raw.empty() && pending.HasCompressedData())
    {
      CLog::Log(LOGDEBUG,
                "RetroPlayer[SAVE]: Preserving compressed {} because raw data is unavailable",
                fieldName ? fieldName : "blob");
    }
    else
    {
      return CreateWriteOffsets(builder, pending.raw, fieldName, false);
    }
  }

  if (!pending.HasCompressedData())
    return CreateWriteOffsets(builder, pending.raw, fieldName, true);

  if (pending.uncompressedSize == 0 || pending.compressed.size() > pending.uncompressedSize)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid compressed {} pending blob",
              fieldName ? fieldName : "blob");
    return CreateWriteOffsets(builder, pending.raw, fieldName, true);
  }

  SavestateBlobOffsets offsets;
  const std::vector<uint8_t> emptyData;
  offsets.raw = builder.CreateVector(emptyData);
  offsets.compressed = builder.CreateVector(pending.compressed);
  offsets.compressionType = pending.compression;
  offsets.uncompressedSize = pending.uncompressedSize;
  return offsets;
}

bool CSavestateBlob::PrepareMemoryData(const SAVESTATE::Savestate& savestate,
                                       size_t expectedSize,
                                       std::vector<uint8_t>& decompressedMemoryData)
{
  decompressedMemoryData.clear();

  if (!IsSupportedMemorySize(expectedSize))
    return false;

  if (savestate.memory_data_uncompressed_size() != expectedSize)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid compressed memory size {}, expected {}",
              savestate.memory_data_uncompressed_size(), expectedSize);
    return false;
  }

  const auto* compressed = savestate.memory_data_compressed();
  if (compressed == nullptr || compressed->size() == 0 || compressed->size() > expectedSize ||
      compressed->size() > MAX_SAVESTATE_MEMORY_SIZE)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid compressed memory data size");
    return false;
  }

  decompressedMemoryData.resize(expectedSize);
  if (!CSavestateCompression::DecompressZstd(compressed->data(), compressed->size(),
                                             decompressedMemoryData.data(), expectedSize, "memory"))
  {
    decompressedMemoryData.clear();
    return false;
  }

  return true;
}

bool CSavestateBlob::PrepareVideoData(const SAVESTATE::Savestate& savestate,
                                      std::vector<uint8_t>& decompressedVideoData)
{
  decompressedVideoData.clear();

  size_t packedMinimumSize = 0;
  if (!GetPackedMinimumVideoSize(savestate, packedMinimumSize))
    return false;

  const uint64_t uncompressedSize = savestate.video_data_uncompressed_size();
  if (uncompressedSize == 0 || uncompressedSize > MAX_SAVESTATE_VIDEO_SIZE ||
      uncompressedSize < packedMinimumSize)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid compressed video size {}", uncompressedSize);
    return false;
  }

  const size_t expectedSize = static_cast<size_t>(uncompressedSize);
  const auto* compressed = savestate.video_data_compressed();
  if (compressed == nullptr || compressed->size() == 0 || compressed->size() > expectedSize ||
      compressed->size() > MAX_SAVESTATE_VIDEO_SIZE)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid compressed video data size");
    return false;
  }

  decompressedVideoData.resize(expectedSize);
  if (!CSavestateCompression::DecompressZstd(compressed->data(), compressed->size(),
                                             decompressedVideoData.data(), expectedSize, "video"))
  {
    decompressedVideoData.clear();
    return false;
  }

  return true;
}

bool CSavestateBlob::IsValidRawMemoryData(const SAVESTATE::Savestate& savestate,
                                          size_t expectedSize)
{
  if (!IsSupportedMemorySize(expectedSize))
    return false;

  const auto* memoryData = savestate.memory_data();
  return memoryData != nullptr && memoryData->size() == expectedSize;
}

bool CSavestateBlob::HasRawVideoData(const SAVESTATE::Savestate& savestate)
{
  return savestate.video_data() != nullptr && savestate.video_data()->size() > 0;
}

bool CSavestateBlob::IsValidCopiedCompressedMemoryData(const SAVESTATE::Savestate& savestate)
{
  const auto* compressed = savestate.memory_data_compressed();
  const uint64_t uncompressedSize = savestate.memory_data_uncompressed_size();

  if (compressed == nullptr || compressed->size() == 0 ||
      compressed->size() > MAX_SAVESTATE_MEMORY_SIZE || uncompressedSize == 0 ||
      uncompressedSize > MAX_SAVESTATE_MEMORY_SIZE || compressed->size() > uncompressedSize)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid compressed memory data size");
    return false;
  }

  return true;
}

bool CSavestateBlob::IsValidMemoryDataSize(size_t size)
{
  if (size > MAX_SAVESTATE_MEMORY_SIZE)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid memory data size");
    return false;
  }

  return true;
}

bool CSavestateBlob::IsSupportedMemorySize(size_t expectedSize)
{
  if (expectedSize == 0 || expectedSize > MAX_SAVESTATE_MEMORY_SIZE)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Invalid memory size {}, limit {}", expectedSize,
              MAX_SAVESTATE_MEMORY_SIZE);
    return false;
  }

  return true;
}
