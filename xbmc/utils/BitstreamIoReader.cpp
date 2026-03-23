/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BitstreamIoReader.h"

BitstreamIoReader::BitstreamIoReader(const uint8_t* data, size_t size)
  : m_data(data), m_size(size), m_bitPosition(0)
{
}

BitstreamIoReader::BitstreamIoReader(const std::vector<uint8_t>& data)
  : BitstreamIoReader(data.data(), data.size())
{
}

bool BitstreamIoReader::read(bool& out)
{
  if (available_bits() < 1) return false;

  out = ((m_data[m_bitPosition / 8] >> (7 - (m_bitPosition % 8))) & 0x1) != 0;
  ++m_bitPosition;
  return true;
}

bool BitstreamIoReader::read_ue(uint64_t& out)
{
  uint32_t leadingZeroBits = 0;
  bool bit = false;

  while (true)
  {
    if (!read(bit)) return false;

    if (bit) break;

    if (++leadingZeroBits > 31) return false;
  }

  if (leadingZeroBits == 0)
  {
    out = 0;
    return true;
  }

  uint64_t suffix = 0;
  if (!read_n(leadingZeroBits, suffix)) return false;

  out = ((1ULL << leadingZeroBits) - 1ULL) + suffix;
  return true;
}

bool BitstreamIoReader::read_se(int64_t& out)
{
  uint64_t codeNum = 0;
  if (!read_ue(codeNum)) return false;

  out = (codeNum & 1ULL) ? static_cast<int64_t>((codeNum + 1ULL) / 2ULL)
                         : -static_cast<int64_t>(codeNum / 2ULL);
  return true;
}

bool BitstreamIoReader::skip_bits(size_t n)
{
  if (available_bits() < n) return false;

  m_bitPosition += n;
  return true;
}

bool BitstreamIoReader::byte_align()
{
  const size_t misalign = m_bitPosition % 8;
  if (misalign == 0) return true;

  return skip_bits(8 - misalign);
}

bool BitstreamIoReader::is_aligned() const
{
  return (m_bitPosition % 8) == 0;
}

size_t BitstreamIoReader::position() const
{
  return m_bitPosition;
}

size_t BitstreamIoReader::available_bits() const
{
  return (m_size * 8) - m_bitPosition;
}
