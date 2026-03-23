/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

class BitstreamIoReader
{
public:
  BitstreamIoReader(const uint8_t* data, size_t size);
  explicit BitstreamIoReader(const std::vector<uint8_t>& data);

  bool read(bool& out);

  template<typename T>
  bool read_n(uint32_t n, T& out)
  {
    static_assert(std::is_integral_v<T>, "BitstreamIoReader::read_n requires integral type");

    if (n > 64 || available_bits() < n)
      return false;

    uint64_t value = 0;
    for (uint32_t i = 0; i < n; ++i)
    {
      bool bit = false;
      if (!read(bit))
        return false;

      value = (value << 1) | (bit ? 1ULL : 0ULL);
    }

    out = static_cast<T>(value);
    return true;
  }

  bool read_ue(uint64_t& out);
  bool read_se(int64_t& out);

  bool skip_bits(size_t n);
  bool byte_align();
  bool is_aligned() const;

  size_t position() const;
  size_t available_bits() const;

private:
  const uint8_t* m_data;
  size_t m_size;
  size_t m_bitPosition;
};
