/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/BitReader.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <span>
#include <vector>

#include <gtest/gtest.h>

namespace
{
uint32_t ReferenceBits(const uint8_t* data, size_t pos, unsigned int n)
{
  uint32_t value{0};
  for (unsigned int i = 0; i < n; ++i, ++pos)
    value = (value << 1) | ((data[pos / 8] >> (7 - pos % 8)) & 1);
  return value;
}

// Reads every width from 0 to 32 at every start bit and compares with ReferenceBits
void SweepReads(const uint8_t* data, size_t size)
{
  const size_t sizeBits{size * 8};
  for (size_t start = 0; start <= sizeBits; ++start)
  {
    for (unsigned int n = 0; n <= 32; ++n)
    {
      CBitReader reader({data, size});
      reader.SkipBits(start);
      ASSERT_FALSE(reader.Error());

      const uint32_t value{reader.ReadBits(n)};
      if (start + n <= sizeBits)
      {
        ASSERT_FALSE(reader.Error()) << "start " << start << " width " << n;
        ASSERT_EQ(value, ReferenceBits(data, start, n)) << "start " << start << " width " << n;
        ASSERT_EQ(reader.Position(), start + n);
      }
      else
      {
        ASSERT_TRUE(reader.Error()) << "start " << start << " width " << n;
        ASSERT_EQ(value, 0u);
        ASSERT_EQ(reader.Position(), start);
        ASSERT_EQ(reader.BitsLeft(), sizeBits - start);
      }
    }
  }
}
} // namespace

TEST(TestBitReader, General)
{
  std::vector<std::byte> data{std::byte{0xAA}, std::byte{0x55}, std::byte{0b00010010},
                              std::byte{0b00010010}, std::byte{0xFF}};
  CBitReader br(data);
  EXPECT_EQ(br.ReadBits(4), 0xAu);
  EXPECT_EQ(br.ReadBits(8), 0xA5u);
  br.SkipBits(4);
  EXPECT_EQ(br.ReadUE(), 8u); // 0001001b = 8
  br.ByteAlign();
  EXPECT_EQ(br.ReadSE(), -4); // 0001001b = -4
  br.ByteAlign();
  EXPECT_FALSE(br.Error());

  CBitReader tooWide{br};
  EXPECT_EQ(tooWide.ReadBits(64), 0u); // max 32
  EXPECT_TRUE(tooWide.Error());

  EXPECT_EQ(br.ReadBits(16), 0u); // only 8 bits left
  EXPECT_TRUE(br.Error());
}

TEST(TestBitReader, ReadBitsMatchesReference)
{
  std::array<uint8_t, 64> data;
  std::mt19937 generator(1234);
  std::uniform_int_distribution<int> distribution(0, 255);
  for (auto& byte : data)
    byte = static_cast<uint8_t>(distribution(generator));

  SweepReads(data.data(), data.size());
}

TEST(TestBitReader, ShortBuffers)
{
  std::mt19937 generator(5678);
  std::uniform_int_distribution<int> distribution(0, 255);

  for (size_t size = 1; size <= 8; ++size)
  {
    // Allocated at its exact size, so that sanitizers catch any read past the end
    auto data{std::make_unique<uint8_t[]>(size)};
    for (size_t i = 0; i < size; ++i)
      data[i] = static_cast<uint8_t>(distribution(generator));

    SweepReads(data.get(), size);

    CBitReader skip({data.get(), size});
    skip.SkipBits(size * 8 + 1);
    EXPECT_TRUE(skip.Error());
    EXPECT_EQ(skip.Position(), 0u);

    // All zeros, so no Exp-Golomb code ends within the buffer
    std::fill_n(data.get(), size, 0);
    CBitReader ue({data.get(), size});
    EXPECT_EQ(ue.ReadUE(), 0u);
    EXPECT_TRUE(ue.Error());
    EXPECT_EQ(ue.Position(), 0u);
  }
}

TEST(TestBitReader, ReadAtEnd)
{
  const std::array<uint8_t, 2> data{0x12, 0x34};
  CBitReader reader(data);

  EXPECT_EQ(reader.ReadBits(16), 0x1234u);
  EXPECT_FALSE(reader.Error());

  EXPECT_EQ(reader.ReadBits(1), 0u);
  EXPECT_TRUE(reader.Error());
  EXPECT_EQ(reader.Position(), 16u);
  EXPECT_EQ(reader.BitsLeft(), 0u);
}

TEST(TestBitReader, ErrorIsSticky)
{
  const std::array<uint8_t, 4> data{0xFF, 0xFF, 0xFF, 0xFF};
  CBitReader reader(data);

  EXPECT_EQ(reader.ReadBits(3), 7u);
  reader.SkipBits(40);
  EXPECT_TRUE(reader.Error());
  EXPECT_EQ(reader.Position(), 3u);

  EXPECT_EQ(reader.ReadBits(1), 0u);
  EXPECT_EQ(reader.ReadUE(), 0u);
  reader.SkipBits(1);
  reader.ByteAlign();
  EXPECT_TRUE(reader.Error());
  EXPECT_EQ(reader.Position(), 3u);
  EXPECT_EQ(reader.BitsLeft(), 29u);
}

TEST(TestBitReader, WidthLimits)
{
  const std::array<uint8_t, 8> data{};

  CBitReader tooWide(data);
  EXPECT_EQ(tooWide.ReadBits(33), 0u);
  EXPECT_TRUE(tooWide.Error());
  EXPECT_EQ(tooWide.Position(), 0u);

  CBitReader none(data);
  EXPECT_EQ(none.ReadBits(0), 0u);
  EXPECT_FALSE(none.Error());
  none.SkipBits(64);
  EXPECT_EQ(none.ReadBits(0), 0u);
  EXPECT_FALSE(none.Error());
  EXPECT_EQ(none.Position(), 64u);
}

TEST(TestBitReader, SkipAndAlign)
{
  const std::array<uint8_t, 2> data{};
  CBitReader reader(data);

  reader.SkipBits(5);
  EXPECT_EQ(reader.Position(), 5u);
  reader.ByteAlign();
  EXPECT_EQ(reader.Position(), 8u);
  reader.ByteAlign();
  EXPECT_EQ(reader.Position(), 8u);
  reader.SkipBits(8);
  EXPECT_EQ(reader.Position(), 16u);
  reader.ByteAlign();
  EXPECT_EQ(reader.Position(), 16u);
  EXPECT_FALSE(reader.Error());

  reader.SkipBits(1);
  EXPECT_TRUE(reader.Error());
  EXPECT_EQ(reader.Position(), 16u);
}

TEST(TestBitReader, ReadUE)
{
  // 1, 010, 011, 00100, 0001001
  const std::array<uint8_t, 3> codes{0xA6, 0x41, 0x20};
  CBitReader reader(codes);
  EXPECT_EQ(reader.ReadUE(), 0u);
  EXPECT_EQ(reader.ReadUE(), 1u);
  EXPECT_EQ(reader.ReadUE(), 2u);
  EXPECT_EQ(reader.ReadUE(), 3u);
  EXPECT_EQ(reader.ReadUE(), 8u);
  EXPECT_FALSE(reader.Error());
  EXPECT_EQ(reader.Position(), 19u);

  // 31 zeros, a 1, then 31 ones
  const std::array<uint8_t, 8> largest{0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFE};
  CBitReader largestReader(largest);
  EXPECT_EQ(largestReader.ReadUE(), std::numeric_limits<uint32_t>::max() - 1);
  EXPECT_FALSE(largestReader.Error());
  EXPECT_EQ(largestReader.Position(), 63u);

  const std::array<uint8_t, 5> tooLong{0x00, 0x00, 0x00, 0x00, 0x80};
  CBitReader tooLongReader(tooLong);
  EXPECT_EQ(tooLongReader.ReadUE(), 0u);
  EXPECT_TRUE(tooLongReader.Error());
  EXPECT_EQ(tooLongReader.Position(), 0u);

  const std::array<uint8_t, 2> zeros{0x00, 0x00};
  CBitReader zerosReader(zeros);
  EXPECT_EQ(zerosReader.ReadUE(), 0u);
  EXPECT_TRUE(zerosReader.Error());

  // 15 zeros and the stop bit, but no room for the 15 value bits
  const std::array<uint8_t, 2> cutShort{0x00, 0x01};
  CBitReader cutShortReader(cutShort);
  EXPECT_EQ(cutShortReader.ReadUE(), 0u);
  EXPECT_TRUE(cutShortReader.Error());
  EXPECT_EQ(cutShortReader.Position(), 0u);
  EXPECT_EQ(cutShortReader.BitsLeft(), 16u);
}

TEST(TestBitReader, ReadUEMax)
{
  // 0000001000001 = 64
  const std::array<uint8_t, 2> sixtyFour{0x02, 0x08};
  CBitReader atMax(sixtyFour);
  EXPECT_EQ(atMax.ReadUE(64), 64u);
  EXPECT_FALSE(atMax.Error());
  EXPECT_EQ(atMax.Position(), 13u);

  // 0000001000010 = 65
  const std::array<uint8_t, 2> sixtyFive{0x02, 0x10};
  CBitReader overMax(sixtyFive);
  EXPECT_EQ(overMax.ReadUE(64), 0u);
  EXPECT_TRUE(overMax.Error());
  EXPECT_EQ(overMax.Position(), 0u);
  EXPECT_EQ(overMax.BitsLeft(), 16u);
  EXPECT_EQ(overMax.ReadBits(1), 0u);
  EXPECT_EQ(overMax.Position(), 0u);
}

TEST(TestBitReader, ReadSE)
{
  // ue 0, 1, 2, 3, 4: 1, 010, 011, 00100, 00101
  const std::array<uint8_t, 3> codes{0xA6, 0x42, 0x80};
  CBitReader reader(codes);
  EXPECT_EQ(reader.ReadSE(), 0);
  EXPECT_EQ(reader.ReadSE(), 1);
  EXPECT_EQ(reader.ReadSE(), -1);
  EXPECT_EQ(reader.ReadSE(), 2);
  EXPECT_EQ(reader.ReadSE(), -2);
  EXPECT_FALSE(reader.Error());

  // ue 2^32 - 3
  const std::array<uint8_t, 8> largestPositive{0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFC};
  CBitReader positive(largestPositive);
  EXPECT_EQ(positive.ReadSE(), std::numeric_limits<int32_t>::max());
  EXPECT_FALSE(positive.Error());

  // ue 2^32 - 2
  const std::array<uint8_t, 8> largestNegative{0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFE};
  CBitReader negative(largestNegative);
  EXPECT_EQ(negative.ReadSE(), -std::numeric_limits<int32_t>::max());
  EXPECT_FALSE(negative.Error());
}

TEST(TestBitReader, Empty)
{
  CBitReader bits(std::span<const uint8_t>{});
  EXPECT_EQ(bits.BitsLeft(), 0u);
  EXPECT_EQ(bits.ReadBits(1), 0u);
  EXPECT_TRUE(bits.Error());

  CBitReader ue(std::span<const uint8_t>{});
  EXPECT_EQ(ue.ReadUE(), 0u);
  EXPECT_TRUE(ue.Error());

  CBitReader align(std::span<const uint8_t>{});
  align.ByteAlign();
  EXPECT_FALSE(align.Error());
}

TEST(TestBitReader, SpanTypes)
{
  const std::array<uint8_t, 3> bytes{0xA6, 0x41, 0x20};
  const std::array<std::byte, 3> stdBytes{std::byte{0xA6}, std::byte{0x41}, std::byte{0x20}};
  CBitReader fromBytes(bytes);
  CBitReader fromStdBytes(stdBytes);

  for (int i = 0; i < 5; ++i)
    EXPECT_EQ(fromBytes.ReadUE(), fromStdBytes.ReadUE());
  EXPECT_EQ(fromBytes.ReadBits(5), fromStdBytes.ReadBits(5));
  EXPECT_FALSE(fromBytes.Error());
  EXPECT_FALSE(fromStdBytes.Error());
}
