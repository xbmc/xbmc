/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace
{
constexpr unsigned int CORE_SIZE = 2012;
constexpr unsigned int HD_SIZE = 1000;

//! A 16-bit big-endian DTS core frame: 512 samples, 48 kHz, 5.1.
std::vector<uint8_t> CoreFrame()
{
  std::vector<uint8_t> frame(CORE_SIZE, 0);
  const uint8_t header[] = {0x7F, 0xFE, 0x80, 0x01, 0x00, 0x3C, 0x7D, 0xB2, 0x77, 0x00, 0x02};
  std::copy(std::begin(header), std::end(header), frame.begin());
  return frame;
}

//! A DTS-HD extension substream carrying XLL, so the frame is DTS-HD MA.
std::vector<uint8_t> HdExtension()
{
  std::vector<uint8_t> ext(HD_SIZE, 0);
  const uint8_t header[] = {0x64, 0x58, 0x20, 0x25, 0x00, 0x01, 0xE0, 0x7C, 0xE0};
  std::copy(std::begin(header), std::end(header), ext.begin());
  const uint8_t xll[] = {0x41, 0xA2, 0x95, 0x47};
  std::copy(std::begin(xll), std::end(xll), ext.begin() + 16);
  return ext;
}

std::vector<uint8_t> MaFrame()
{
  std::vector<uint8_t> frame = CoreFrame();
  const std::vector<uint8_t> ext = HdExtension();
  frame.insert(frame.end(), ext.begin(), ext.end());
  return frame;
}

void Append(std::vector<uint8_t>& stream, const std::vector<uint8_t>& bytes)
{
  stream.insert(stream.end(), bytes.begin(), bytes.end());
}

//! The type the parser reported for every frame it handed out.
std::vector<CAEStreamInfo::DataType> Parse(std::vector<uint8_t> stream)
{
  CAEStreamParser parser;
  std::vector<CAEStreamInfo::DataType> types;
  size_t pos = 0;
  while (pos < stream.size())
  {
    uint8_t* out = nullptr;
    unsigned int outSize = 0;
    const int used = parser.AddData(stream.data() + pos,
                                    static_cast<unsigned int>(stream.size() - pos), &out, &outSize);
    if (outSize > 0)
      types.push_back(parser.GetDataType());
    if (used <= 0 && outSize == 0)
      break;
    pos += used;
  }
  return types;
}
} // namespace

//! After a flush the first bytes can hold a core frame cut from its extension. Taking it as a
//! plain DTS stream reconfigures the output twice - to DTS, then back to DTS-HD MA - and the
//! receiver has to acquire the format again for nothing.
TEST(TestAEStreamParser, ACoreFrameNotFollowedByASyncIsNotTakenAsTheStream)
{
  std::vector<uint8_t> stream = CoreFrame();
  Append(stream, {0x1C, 0x4A, 0x40, 0xA6, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
  for (int i = 0; i < 4; ++i)
    Append(stream, MaFrame());

  const std::vector<CAEStreamInfo::DataType> types = Parse(stream);

  ASSERT_FALSE(types.empty());
  for (const CAEStreamInfo::DataType type : types)
    EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_DTSHD_MA, type);
}

TEST(TestAEStreamParser, ADtsHdMaStreamIsDetectedFromItsFirstFrame)
{
  std::vector<uint8_t> stream;
  for (int i = 0; i < 4; ++i)
    Append(stream, MaFrame());

  const std::vector<CAEStreamInfo::DataType> types = Parse(stream);

  ASSERT_FALSE(types.empty());
  EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_DTSHD_MA, types.front());
}

TEST(TestAEStreamParser, ACoreOnlyStreamIsStillDts)
{
  std::vector<uint8_t> stream;
  for (int i = 0; i < 4; ++i)
    Append(stream, CoreFrame());

  const std::vector<CAEStreamInfo::DataType> types = Parse(stream);

  ASSERT_FALSE(types.empty());
  for (const CAEStreamInfo::DataType type : types)
    EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_DTS_512, type);
}
