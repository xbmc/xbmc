/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace KODI
{
namespace RETRO
{
class CSavestateCompression
{
public:
  static bool CompressZstdIfSmaller(const uint8_t* data,
                                    size_t size,
                                    std::vector<uint8_t>& compressed);

  static bool DecompressZstd(const uint8_t* compressedData,
                             size_t compressedSize,
                             uint8_t* outputData,
                             size_t expectedOutputSize,
                             const char* logContext);
};
} // namespace RETRO
} // namespace KODI
