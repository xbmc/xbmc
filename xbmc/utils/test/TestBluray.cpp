/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/bluray/BitReader.h"
#include "filesystem/bluray/M2TSParser.h"
#include "test/TestUtils.h"

#include <ranges>

#include <gtest/gtest.h>

TEST(TestBitReader, General)
{
  std::vector<std::byte> data{std::byte{0xAA}, std::byte{0x55}, std::byte{0b00010010},
                              std::byte{0b00010010}, std::byte{0xFF}};
  BitReader br(data);
  EXPECT_EQ(br.ReadBits(4), 0xA);
  EXPECT_EQ(br.ReadBits(8), 0xA5);
  br.SkipBits(4);
  EXPECT_EQ(br.ReadUE(), 8); // 0001001b = 8
  br.ByteAlign();
  EXPECT_EQ(br.ReadSE(), -4); // 0001001b = -4
  br.ByteAlign();

  EXPECT_THROW(br.ReadBits(64), std::out_of_range); // max 32
  EXPECT_THROW(br.ReadBits(16), std::out_of_range); // only 8 bits left
}

TEST(TestBytes, General)
{
  std::vector<std::byte> data{std::byte{0xAA}, std::byte{0x55}, std::byte{0xAA}, std::byte{0x55},
                              std::byte{0xAA}, std::byte{0x55}, std::byte{0xAA}, std::byte{0x55}};

  EXPECT_EQ(GetByte(data, 0), 0xAA);
  EXPECT_EQ(GetWord(data, 0), 0xAA55);
  EXPECT_EQ(GetDWord(data, 0), 0xAA55AA55);
  EXPECT_EQ(GetQWord(data, 0), 0xAA55AA55AA55AA55);

  EXPECT_THROW(GetByte(data, 8), std::out_of_range);
  EXPECT_THROW(GetWord(data, 7), std::out_of_range);
  EXPECT_THROW(GetDWord(data, 5), std::out_of_range);
  EXPECT_THROW(GetQWord(data, 1), std::out_of_range);

  EXPECT_EQ(GetBits(0x12345678, 24, 4), 3);
  EXPECT_EQ(GetBits(0x12345678, 32, 32), 0x12345678); // fast return
  EXPECT_EQ(GetBits64(0x123456789ABCDEF0, 56, 4), 3);
  EXPECT_EQ(GetBits64(0x123456789ABCDEF0, 64, 64), 0x123456789ABCDEF0); // fast return

  EXPECT_THROW(GetBits(0x12345678, 28, 32), std::out_of_range);
  EXPECT_THROW(GetBits(0x12345678, 36, 32), std::out_of_range);
  EXPECT_THROW(GetBits(0x12345678, 32, 0), std::out_of_range);
  EXPECT_THROW(GetBits64(0x12345678ABCDEF0, 60, 64), std::out_of_range);
  EXPECT_THROW(GetBits64(0x12345678ABCDEF0, 68, 64), std::out_of_range);
  EXPECT_THROW(GetBits64(0x12345678ABCDEF0, 64, 0), std::out_of_range);
}

TEST(TestM2TSParser, General)
{
  XFILE::StreamMap streams;
  EXPECT_EQ(XFILE::CM2TSParser::GetStreamsFromFile(
                XBMC_REF_FILE_PATH("xbmc/utils/test/data/bluray/"), 1, "M2TS", streams),
            true);
  if (!streams.empty())
  {
    const auto videoStreams{XFILE::CM2TSParser::GetVideoStreams(streams)};
    EXPECT_EQ(videoStreams.size(), 1);
    if (videoStreams.size() == 1)
    {
      const auto& v{videoStreams[0].get()};
      EXPECT_EQ(v.width, 3840);
      EXPECT_EQ(v.height, 2160);
      EXPECT_EQ(v.dolbyVision, true);
      EXPECT_EQ(v.streamType, XFILE::ENCODING_TYPE::VIDEO_HEVC);
    }
    const auto audioStreams{XFILE::CM2TSParser::GetAudioStreams(streams)};
    EXPECT_EQ(audioStreams.size(), 4);
    if (audioStreams.size() == 4)
    {
      auto& a{audioStreams[0].get()};
      EXPECT_EQ(a.isAtmos, true);
      EXPECT_EQ(a.streamType, XFILE::ENCODING_TYPE::AUDIO_TRUHD);
      a = audioStreams[1].get();
      EXPECT_EQ(a.channels, 2);
      EXPECT_EQ(a.isAtmos, false);
      EXPECT_EQ(a.streamType, XFILE::ENCODING_TYPE::AUDIO_AC3);
      a = audioStreams[2].get();
      EXPECT_EQ(a.channels, 6);
      EXPECT_EQ(a.isAtmos, false);
      EXPECT_EQ(a.streamType, XFILE::ENCODING_TYPE::AUDIO_AC3);
      a = audioStreams[3].get();
      EXPECT_EQ(a.channels, 6);
      EXPECT_EQ(a.isAtmos, false);
      EXPECT_EQ(a.streamType, XFILE::ENCODING_TYPE::AUDIO_AC3);
    }
    const auto subtitleStreams{XFILE::CM2TSParser::GetSubtitleStreams(streams)};
    EXPECT_EQ(subtitleStreams.size(), 4);
  }
}
