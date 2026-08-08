/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateCompression.h"

#include "utils/log.h"

#include <utility>

#include <zstd.h>

using namespace KODI;
using namespace RETRO;

bool CSavestateCompression::CompressZstdIfSmaller(const uint8_t* data,
                                                  size_t size,
                                                  std::vector<uint8_t>& compressed)
{
  compressed.clear();

  if (data == nullptr || size == 0)
    return false;

  const size_t bound = ZSTD_compressBound(size);
  std::vector<uint8_t> candidate(bound);

  const size_t result =
      ZSTD_compress(candidate.data(), candidate.size(), data, size, ZSTD_CLEVEL_DEFAULT);

  if (ZSTD_isError(result))
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Zstd compression failed: {}",
              ZSTD_getErrorName(result));
    return false;
  }

  if (result == 0 || result >= size)
    return false;

  candidate.resize(result);
  compressed = std::move(candidate);
  return true;
}

bool CSavestateCompression::DecompressZstd(const uint8_t* compressedData,
                                           size_t compressedSize,
                                           uint8_t* outputData,
                                           size_t expectedOutputSize,
                                           const char* logContext)
{
  if (compressedData == nullptr || compressedSize == 0 || outputData == nullptr ||
      expectedOutputSize == 0)
  {
    return false;
  }

  const char* context = logContext != nullptr ? logContext : "blob";
  const size_t result =
      ZSTD_decompress(outputData, expectedOutputSize, compressedData, compressedSize);

  if (ZSTD_isError(result))
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Zstd {} decompression failed: {}", context,
              ZSTD_getErrorName(result));
    return false;
  }

  if (result != expectedOutputSize)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Zstd {} decompressed to {} bytes, expected {}", context,
              result, expectedOutputSize);
    return false;
  }

  return true;
}
