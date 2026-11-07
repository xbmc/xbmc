/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

/*!
 * \brief Reads a byte buffer as a stream of bits, most significant bit first.
 *
 * A read or skip that does not fit sets Error() and leaves the position unchanged. The error is
 * sticky: once set, every read returns 0 and every skip does nothing, so a caller can parse a
 * whole structure and check Error() once at the end.
 */
class CBitReader
{
public:
  explicit CBitReader(std::span<const uint8_t> data) noexcept
    : m_data(data.data()),
      m_size(std::min(data.size(), MAX_SIZE))
  {
  }

  explicit CBitReader(std::span<const std::byte> data) noexcept
    : CBitReader(std::span(reinterpret_cast<const uint8_t*>(data.data()), data.size()))
  {
  }

  //! Reads n bits, up to 32.
  uint32_t ReadBits(unsigned int n) noexcept
  {
    if (n == 0)
      return 0;
    if (m_error || n > 32 || n > BitsLeft())
      return Fail();
    const auto value{static_cast<uint32_t>(Window() >> (64 - n))};
    m_pos += n;
    return value;
  }

  void SkipBits(size_t n) noexcept
  {
    if (m_error || n > BitsLeft())
      m_error = true;
    else
      m_pos += n;
  }

  //! Reads an unsigned Exp-Golomb code. A value above max is an error.
  uint32_t ReadUE(uint32_t max = UINT32_MAX - 1) noexcept
  {
    if (m_error)
      return 0;
    // The window is zero past the end, so its first 1 bit is always within the data
    const auto zeros{static_cast<unsigned int>(std::countl_zero(Window()))};
    if (zeros > 31 || 2 * zeros + 1 > BitsLeft())
      return Fail();
    const size_t start{m_pos};
    m_pos += zeros;
    const uint32_t value{ReadBits(zeros + 1) - 1};
    if (value > max)
    {
      m_pos = start;
      return Fail();
    }
    return value;
  }

  //! Reads a signed Exp-Golomb code.
  int32_t ReadSE() noexcept
  {
    const uint32_t value{ReadUE()};
    return (value & 1) ? static_cast<int32_t>(value / 2 + 1) : -static_cast<int32_t>(value / 2);
  }

  void SkipUE() noexcept { ReadUE(); }
  void SkipSE() noexcept { ReadUE(); }

  void ByteAlign() noexcept { SkipBits((8 - m_pos % 8) % 8); }

  //! The number of bits read or skipped.
  size_t Position() const noexcept { return m_pos; }
  size_t BitsLeft() const noexcept { return m_size * 8 - m_pos; }
  bool Error() const noexcept { return m_error; }

private:
  // The longest buffer whose length in bits fits size_t. A longer one is read as ending there.
  static constexpr size_t MAX_SIZE{SIZE_MAX / 8};

  // The 64 bits from the current position. Bits past the end of the data read as 0.
  uint64_t Window() const noexcept
  {
    const size_t byte{m_pos / 8};
    const size_t count{std::min<size_t>(m_size - byte, 8)};
    uint64_t window{0};
    if (count == 8)
    {
      for (size_t i = 0; i < 8; ++i)
        window = (window << 8) | m_data[byte + i];
    }
    else
    {
      for (size_t i = 0; i < count; ++i)
        window |= uint64_t{m_data[byte + i]} << (56 - 8 * i);
    }
    return window << (m_pos % 8);
  }

  uint32_t Fail() noexcept
  {
    m_error = true;
    return 0;
  }

  const uint8_t* m_data;
  size_t m_size;
  size_t m_pos{0};
  bool m_error{false};
};
