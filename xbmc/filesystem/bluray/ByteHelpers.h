/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>

// bits 32->1
constexpr unsigned int GetBits(unsigned int value, unsigned int firstBit, unsigned int numBits)
{
  if (numBits == 0 || firstBit > 32 || firstBit < numBits)
    throw std::out_of_range("Bit range out of bounds");
  if (numBits == 32)
    return value;
  return (value >> (firstBit - numBits)) & ((1u << numBits) - 1);
}

// bits 64->1
constexpr uint64_t GetBits64(uint64_t value, unsigned int firstBit, unsigned int numBits)
{
  if (numBits == 0 || firstBit > 64 || firstBit < numBits)
    throw std::out_of_range("Bit range out of bounds");
  if (numBits == 64)
    return value;
  return (value >> (firstBit - numBits)) & ((1ULL << numBits) - 1);
}

constexpr bool Holds(const std::span<const std::byte> bytes,
                     unsigned int offset,
                     unsigned int count)
{
  return static_cast<uint64_t>(offset) + count <= bytes.size();
}

constexpr uint64_t GetQWord(const std::span<const std::byte> bytes, unsigned int offset)
{
  if (!Holds(bytes, offset, 8))
    throw std::out_of_range("Not enough bytes to extract a QWORD");
  return std::to_integer<uint64_t>(bytes[offset + 7]) |
         std::to_integer<uint64_t>(bytes[offset + 6]) << 8 |
         std::to_integer<uint64_t>(bytes[offset + 5]) << 16 |
         std::to_integer<uint64_t>(bytes[offset + 4]) << 24 |
         std::to_integer<uint64_t>(bytes[offset + 3]) << 32 |
         std::to_integer<uint64_t>(bytes[offset + 2]) << 40 |
         std::to_integer<uint64_t>(bytes[offset + 1]) << 48 |
         std::to_integer<uint64_t>(bytes[offset]) << 56;
}

constexpr uint32_t GetDWord(const std::span<const std::byte> bytes, unsigned int offset)
{
  if (!Holds(bytes, offset, 4))
    throw std::out_of_range("Not enough bytes to extract a DWORD");
  return std::to_integer<uint32_t>(bytes[offset + 3]) |
         std::to_integer<uint32_t>(bytes[offset + 2]) << 8 |
         std::to_integer<uint32_t>(bytes[offset + 1]) << 16 |
         std::to_integer<uint32_t>(bytes[offset]) << 24;
}

constexpr uint16_t GetWord(const std::span<const std::byte> bytes, unsigned int offset)
{
  if (!Holds(bytes, offset, 2))
    throw std::out_of_range("Not enough bytes to extract a WORD");
  return static_cast<uint16_t>(std::to_integer<uint16_t>(bytes[offset + 1]) |
                               std::to_integer<uint16_t>(bytes[offset]) << 8);
}

constexpr uint8_t GetByte(const std::span<const std::byte> bytes, unsigned int offset)
{
  if (!Holds(bytes, offset, 1))
    throw std::out_of_range("Not enough bytes to extract a BYTE");
  return std::to_integer<uint8_t>(bytes[offset]);
}

inline std::string GetString(const std::span<const std::byte> bytes,
                             unsigned int offset,
                             unsigned int length)
{
  if (!Holds(bytes, offset, length))
    throw std::out_of_range("Not enough bytes to extract a STRING");
  return std::string{reinterpret_cast<const char*>(bytes.data() + offset), length};
}
