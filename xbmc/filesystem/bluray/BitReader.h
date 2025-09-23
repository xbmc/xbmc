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
  return (value >> (firstBit - numBits)) & ((1ull << numBits) - 1);
}

constexpr uint64_t GetQWord(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 8)
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

constexpr uint32_t GetDWord(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 4)
    throw std::out_of_range("Not enough bytes to extract a DWORD");
  return std::to_integer<uint32_t>(bytes[offset + 3]) |
         std::to_integer<uint32_t>(bytes[offset + 2]) << 8 |
         std::to_integer<uint32_t>(bytes[offset + 1]) << 16 |
         std::to_integer<uint32_t>(bytes[offset]) << 24;
}

constexpr uint16_t GetWord(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 2)
    throw std::out_of_range("Not enough bytes to extract a WORD");
  return static_cast<uint16_t>(std::to_integer<uint16_t>(bytes[offset + 1]) |
                               std::to_integer<uint16_t>(bytes[offset]) << 8);
}

constexpr uint8_t GetByte(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 1)
    throw std::out_of_range("Not enough bytes to extract a BYTE");
  return std::to_integer<uint8_t>(bytes[offset]);
}

inline std::string GetString(const std::span<std::byte> bytes,
                             unsigned int offset,
                             unsigned int length)
{
  if (bytes.size() < offset + length)
    throw std::out_of_range("Not enough bytes to extract a STRING");
  return std::string{reinterpret_cast<const char*>(bytes.data() + offset), length};
}

class BitReader
{
  const std::byte* data;
  uint32_t size;
  uint32_t offset;
  uint32_t bitPosition;

public:
  BitReader(const std::span<std::byte>& buffer)
    : data(buffer.data()),
      size(static_cast<uint32_t>(buffer.size())),
      offset(0),
      bitPosition(0)
  {
  }

  uint32_t ReadBits(uint32_t n)
  {
    if (n > 32)
      throw std::out_of_range("Can only read up to 32 bits at a time");

    uint32_t value{0};

    while (n > 0 && offset < size)
    {
      uint32_t bitsAvailable{8 - bitPosition};
      uint32_t bitsToRead{(n < bitsAvailable) ? n : bitsAvailable};

      value = (value << bitsToRead) |
              ((std::to_integer<uint32_t>(data[offset]) >> (bitsAvailable - bitsToRead)) &
               ((1u << bitsToRead) - 1));

      bitPosition += bitsToRead;
      n -= bitsToRead;

      if (bitPosition == 8)
      {
        bitPosition = 0;
        offset++;
      }
    }

    if (n > 0)
      throw std::out_of_range("Not enough bits left to read");
    return value;
  }

  void SkipBits(uint32_t n)
  {
    uint32_t totalBits{n + bitPosition};
    offset += totalBits >> 3;
    bitPosition = totalBits & 7;
  }

  uint32_t ReadUE()
  {
    int zeros{0};
    while (offset < size && zeros < 32)
    {
      uint32_t bit{(std::to_integer<uint32_t>(data[offset]) >> (7 - bitPosition)) & 1};
      bitPosition++;
      if (bitPosition == 8)
      {
        bitPosition = 0;
        offset++;
      }

      if (bit)
        break;
      zeros++;
    }

    if (zeros == 0)
      return 0;

    return ((1u << zeros) - 1) + ReadBits(zeros);
  }

  void SkipUE() { ReadUE(); }

  int ReadSE()
  {
    uint32_t value{ReadUE()};
    return (value & 1) ? static_cast<int>((value + 1) >> 1) : -static_cast<int>(value >> 1);
  }

  void SkipSE() { ReadSE(); }

  void ByteAlign()
  {
    offset += (bitPosition != 0);
    bitPosition = 0;
  }
};
