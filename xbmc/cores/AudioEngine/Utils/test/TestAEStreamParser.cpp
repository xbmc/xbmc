/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include <algorithm>
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

//! A DTS-HD extension substream whose asset starts with no sync the parser knows.
std::vector<uint8_t> UnknownExtension()
{
  std::vector<uint8_t> ext = HdExtension();
  const uint8_t unknown[] = {0xF0, 0xC7, 0x89, 0x11};
  std::copy(std::begin(unknown), std::end(unknown), ext.begin() + 16);
  return ext;
}

//! A DTS-HD MA extension at peak bit rate, where an XLL frame straddles the substream frame: the
//! word after the header is XLL payload, here one that happens to read as the XCH sync.
std::vector<uint8_t> StraddlingExtension()
{
  std::vector<uint8_t> ext = HdExtension();
  const uint8_t xchLookalike[] = {0x5A, 0x5A, 0x5A, 0x5A};
  std::copy(std::begin(xchLookalike), std::end(xchLookalike), ext.begin() + 16);
  return ext;
}

std::vector<uint8_t> StraddlingFrame()
{
  std::vector<uint8_t> frame = CoreFrame();
  const std::vector<uint8_t> ext = StraddlingExtension();
  frame.insert(frame.end(), ext.begin(), ext.end());
  return frame;
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

void Feed(CAEStreamParser& parser,
          std::vector<uint8_t>& stream,
          std::vector<CAEStreamInfo::DataType>& types,
          size_t chunk = SIZE_MAX)
{
  size_t pos = 0;
  while (pos < stream.size())
  {
    uint8_t* out = nullptr;
    unsigned int outSize = 0;
    const size_t available = std::min(chunk, stream.size() - pos);
    const int used =
        parser.AddData(stream.data() + pos, static_cast<unsigned int>(available), &out, &outSize);
    delete[] out;
    if (outSize > 0)
      types.push_back(parser.GetDataType());
    if (used <= 0 && outSize == 0)
      break;
    pos += used;
  }
}

//! The type the parser reported for every frame it handed out, fed at most \p chunk bytes a call.
std::vector<CAEStreamInfo::DataType> Parse(std::vector<uint8_t> stream,
                                           size_t chunk = SIZE_MAX,
                                           bool knownMa = false)
{
  CAEStreamParser parser;
  parser.SetDTSHDMA(knownMa);
  std::vector<CAEStreamInfo::DataType> types;
  Feed(parser, stream, types, chunk);
  return types;
}

std::vector<uint8_t> TrueHdMajorSyncFrame()
{
  // 64-byte TrueHD major audio unit: 48 kHz, 7.1 (8 channels), 3 substreams, no extensions.
  std::vector<uint8_t> frame(64, 0);
  frame[0] = 0x00;
  frame[1] = 0x20;
  frame[4] = 0xF8;
  frame[5] = 0x72;
  frame[6] = 0x6F;
  frame[7] = 0xBA;
  frame[11] = 0x1F;
  frame[20] = 0x30;

  AVCRC crcTable[1024];
  av_crc_init(crcTable, 0, 16, 0x2D, sizeof(crcTable));
  uint16_t crc = av_crc(crcTable, 0, frame.data() + 4, 24);
  crc ^= (frame[29] << 8) | frame[28];
  frame[30] = crc & 0xFF;
  frame[31] = crc >> 8;
  return frame;
}

//! A non-major-sync TrueHD payload containing random bytes that read as an E-AC-3 sync
//! (0x0B77, bsid=16, fscod=2 -> 32 kHz, acmod=3 -> 3 channels).
std::vector<uint8_t> TrueHdPayloadWithEac3Lookalike()
{
  return {0x0B, 0x77, 0x00, 0x05, 0x86, 0x80, 0x00, 0x00,
          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
}

void AssertTrueHdLocksAcrossRecovery(const std::vector<uint8_t>& lookalike,
                                     CAEStreamInfo::DataType expectedUnlocked,
                                     bool callReset)
{
  const std::vector<uint8_t> majorSync = TrueHdMajorSyncFrame();
  std::vector<uint8_t> chunk;
  Append(chunk, lookalike);
  Append(chunk, majorSync);

  std::vector<CAEStreamInfo::DataType> unlockedTypes = Parse(chunk);
  ASSERT_FALSE(unlockedTypes.empty());
  EXPECT_EQ(expectedUnlocked, unlockedTypes.front());

  CAEStreamParser parser;
  parser.SetTrueHDOnly(true);

  std::vector<CAEStreamInfo::DataType> types;
  Feed(parser, chunk, types);
  if (callReset)
    parser.Reset();

  // Force SyncTrueHD() to drop sync (via Reset() or parity failure) and fall back to DetectType().
  std::vector<uint8_t> zeros(16, 0);
  Feed(parser, zeros, types);
  ASSERT_FALSE(parser.IsValid());
  ASSERT_EQ(1u, types.size());

  Feed(parser, chunk, types);

  ASSERT_EQ(2u, types.size());
  for (const CAEStreamInfo::DataType type : types)
    EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_TRUEHD, type);
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

//! After a lost sync there is no previous type to fall back on, and a stream without a type has no
//! frame duration, which ActiveAE cannot buffer.
TEST(TestAEStreamParser, AnExtensionWithAnUnknownAssetIsNotTakenAsTheStream)
{
  std::vector<uint8_t> stream = CoreFrame();
  Append(stream, UnknownExtension());
  for (int i = 0; i < 4; ++i)
    Append(stream, MaFrame());

  const std::vector<CAEStreamInfo::DataType> types = Parse(stream);

  ASSERT_FALSE(types.empty());
  for (const CAEStreamInfo::DataType type : types)
    EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_DTSHD_MA, type);
}

TEST(TestAEStreamParser, AnExtensionSplitBeforeItsAssetIsStillDetected)
{
  std::vector<uint8_t> stream;
  for (int i = 0; i < 4; ++i)
    Append(stream, MaFrame());

  const std::vector<CAEStreamInfo::DataType> types = Parse(stream, CORE_SIZE + 12);

  ASSERT_EQ(4u, types.size());
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

//! At peak bit rate the word after an MA extension header can be XLL payload that reads as the XCH
//! sync. Taking such frames as DTS-HD HRA packs 8320-byte MA frames into 8192-byte HRA bursts.
TEST(TestAEStreamParser, AnMaStreamStaysMaThroughXllPayloadThatLooksLikeXch)
{
  std::vector<uint8_t> stream = MaFrame();
  for (int i = 0; i < 4; ++i)
    Append(stream, StraddlingFrame());
  Append(stream, MaFrame());

  // One frame per call, as the demuxer delivers them.
  const std::vector<CAEStreamInfo::DataType> types = Parse(stream, CORE_SIZE + HD_SIZE);

  ASSERT_EQ(6u, types.size());
  for (const CAEStreamInfo::DataType type : types)
    EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_DTSHD_MA, type);
}

//! A resume or chapter jump can start inside a straddling run, before any XLL sync is seen. The
//! codec knows the stream is MA from the demuxer's profile and tells the parser.
TEST(TestAEStreamParser, AKnownMaStreamStartingInsideAStraddlingRunIsMa)
{
  std::vector<uint8_t> stream;
  for (int i = 0; i < 4; ++i)
    Append(stream, StraddlingFrame());
  Append(stream, MaFrame());

  const std::vector<CAEStreamInfo::DataType> types = Parse(stream, CORE_SIZE + HD_SIZE, true);

  ASSERT_EQ(5u, types.size());
  for (const CAEStreamInfo::DataType type : types)
    EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_DTSHD_MA, type);
}

//! The MA latch must not catch a real DTS-HD HRA stream, which never carries an XLL sync.
TEST(TestAEStreamParser, ADtsHdHraStreamIsStillHra)
{
  std::vector<uint8_t> stream;
  for (int i = 0; i < 4; ++i)
    Append(stream, StraddlingFrame());

  const std::vector<CAEStreamInfo::DataType> types = Parse(stream, CORE_SIZE + HD_SIZE);

  ASSERT_EQ(4u, types.size());
  for (const CAEStreamInfo::DataType type : types)
    EXPECT_EQ(CAEStreamInfo::STREAM_TYPE_DTSHD, type);
}

//! After a seek or flush a TrueHD stream can start on a non-major-sync packet whose payload
//! happens to contain an AC-3/E-AC-3 or DTS sync word. When the codec already knows the stream
//! is TrueHD, DetectType() must not sync onto payload bytes as E-AC-3 or DTS, and both Reset()
//! and mid-stream TrueHD sync loss must preserve that constraint.
TEST(TestAEStreamParser, AKnownTrueHdStreamDoesNotFalseSyncOnEac3PayloadAcrossReset)
{
  AssertTrueHdLocksAcrossRecovery(TrueHdPayloadWithEac3Lookalike(), CAEStreamInfo::STREAM_TYPE_EAC3,
                                  true);
}

TEST(TestAEStreamParser, AKnownTrueHdStreamDoesNotFalseSyncOnDtsPayloadAcrossReset)
{
  AssertTrueHdLocksAcrossRecovery(MaFrame(), CAEStreamInfo::STREAM_TYPE_DTSHD_MA, true);
}

TEST(TestAEStreamParser, AKnownTrueHdStreamDoesNotFalseSyncAfterParitySyncLoss)
{
  AssertTrueHdLocksAcrossRecovery(TrueHdPayloadWithEac3Lookalike(), CAEStreamInfo::STREAM_TYPE_EAC3,
                                  false);
  AssertTrueHdLocksAcrossRecovery(MaFrame(), CAEStreamInfo::STREAM_TYPE_DTSHD_MA, false);
}
