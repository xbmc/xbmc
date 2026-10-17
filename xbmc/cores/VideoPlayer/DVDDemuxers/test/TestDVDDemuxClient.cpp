/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxClient.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"

#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/crc.h>
}

namespace
{
constexpr int AUDIO_ID = 1;

// An AC-3 frame with nothing but a header and a checksum: 2 channels, 48 kHz, 64 kbit/s
std::vector<uint8_t> MakeAc3Frame()
{
  std::vector<uint8_t> frame(256, 0);
  frame[0] = 0x0B;
  frame[1] = 0x77;
  frame[4] = 0x08;
  frame[5] = 0x40;
  frame[6] = 0x40;
  const uint32_t crc =
      av_crc(av_crc_get_table(AV_CRC_16_ANSI), 0, frame.data() + 2, frame.size() - 4);
  frame[frame.size() - 2] = static_cast<uint8_t>(crc & 0xFF);
  frame[frame.size() - 1] = static_cast<uint8_t>(crc >> 8);
  return frame;
}

// An input stream that demuxes for itself, as an inputstream add-on does: it announces one
// AC-3 stream with the channel count it is told to, and every packet is a 2 channel frame
class CTestDemuxInputStream : public CDVDInputStream, public CDVDInputStream::IDemux
{
public:
  explicit CTestDemuxInputStream(int channels)
    : CDVDInputStream(DVDSTREAM_TYPE_ADDON, CFileItem()),
      m_channels(channels)
  {
  }

  void Announce(int channels) { m_channels = channels; }
  void Announce(AVCodecID codec, int profile)
  {
    m_codec = codec;
    m_profile = profile;
  }
  //! The next packet tells the demuxer that the streams changed
  void SignalStreamChange() { m_streamChange = true; }

  int Read(uint8_t* buf, int buf_size) override { return 0; }
  int64_t Seek(int64_t offset, int whence) override { return 0; }
  int64_t GetLength() override { return 0; }
  bool IsEOF() override { return false; }

  bool OpenDemux() override { return true; }
  DemuxPacket* ReadDemux() override
  {
    if (std::exchange(m_streamChange, false))
    {
      DemuxPacket* packet = CDVDDemuxUtils::AllocateDemuxPacket(0);
      packet->iStreamId = DMX_SPECIALID_STREAMCHANGE;
      return packet;
    }
    DemuxPacket* packet = CDVDDemuxUtils::AllocateDemuxPacket(static_cast<int>(m_frame.size()));
    std::memcpy(packet->pData, m_frame.data(), m_frame.size());
    packet->iSize = static_cast<int>(m_frame.size());
    packet->iStreamId = AUDIO_ID;
    return packet;
  }
  CDemuxStream* GetStream(int iStreamId) const override
  {
    auto stream = std::make_unique<CDemuxStreamAudio>();
    stream->uniqueId = iStreamId;
    stream->codec = m_codec;
    stream->profile = m_profile;
    stream->iChannels = m_channels;
    stream->iSampleRate = 48000;
    m_announced.emplace_back(std::move(stream));
    return m_announced.back().get();
  }
  std::vector<CDemuxStream*> GetStreams() const override { return {GetStream(AUDIO_ID)}; }
  bool OpenStream(int iStreamId) override { return true; }
  int GetNrOfStreams() const override { return 1; }
  void SetSpeed(int iSpeed) override {}
  bool SeekTime(double time, bool backward = false, double* startpts = nullptr) override
  {
    return false;
  }
  void AbortDemux() override {}
  void FlushDemux() override {}

private:
  int m_channels;
  AVCodecID m_codec{AV_CODEC_ID_AC3};
  int m_profile{AV_PROFILE_UNKNOWN};
  bool m_streamChange{false};
  std::vector<uint8_t> m_frame{MakeAc3Frame()};
  mutable std::vector<std::unique_ptr<CDemuxStream>> m_announced;
};

// The same, announcing one video stream of the aspect it is told to
class CTestVideoInputStream : public CTestDemuxInputStream
{
public:
  explicit CTestVideoInputStream(double aspect) : CTestDemuxInputStream(0), m_aspect(aspect) {}

  void AnnounceAspect(double aspect) { m_aspect = aspect; }

  CDemuxStream* GetStream(int iStreamId) const override
  {
    auto stream = std::make_unique<CDemuxStreamVideo>();
    stream->uniqueId = iStreamId;
    stream->codec = AV_CODEC_ID_H264;
    stream->iWidth = 720;
    stream->iHeight = 576;
    stream->fAspect = m_aspect;
    m_announced.emplace_back(std::move(stream));
    return m_announced.back().get();
  }

private:
  double m_aspect;
  mutable std::vector<std::unique_ptr<CDemuxStream>> m_announced;
};

int Channels(const CDVDDemuxClient& demuxer)
{
  return static_cast<CDemuxStreamAudio*>(demuxer.GetStream(AUDIO_ID))->iChannels;
}

// What CVideoPlayer does with a stream of the demuxer: open it and keep its hints, then open
// it again for every packet whose stream no longer matches them. Returns how often it opened
// the stream again over the given number of packets.
int PlayPackets(CDVDDemuxClient& demuxer, int packets)
{
  demuxer.OpenStream(AUDIO_ID);
  CDVDStreamInfo hint(*demuxer.GetStream(AUDIO_ID), true);

  int reopened = 0;
  for (int i = 0; i < packets; ++i)
  {
    DemuxPacket* packet = demuxer.Read();
    if (!packet)
      break;
    CDVDDemuxUtils::FreeDemuxPacket(packet);

    if (hint != *demuxer.GetStream(AUDIO_ID))
    {
      ++reopened;
      demuxer.OpenStream(AUDIO_ID);
      hint = *demuxer.GetStream(AUDIO_ID);
    }
  }
  return reopened;
}
} // namespace

TEST(TestDVDDemuxClient, StreamAnnouncedWithoutChannelsIsOpenedAgainOnce)
{
  auto input = std::make_shared<CTestDemuxInputStream>(0);
  CDVDDemuxClient demuxer;
  ASSERT_TRUE(demuxer.Open(input));

  EXPECT_EQ(PlayPackets(demuxer, 10), 1);
  EXPECT_EQ(Channels(demuxer), 2);
}

TEST(TestDVDDemuxClient, StreamAnnouncedWithOtherChannelsIsOpenedAgainOnce)
{
  auto input = std::make_shared<CTestDemuxInputStream>(6);
  CDVDDemuxClient demuxer;
  ASSERT_TRUE(demuxer.Open(input));

  EXPECT_EQ(PlayPackets(demuxer, 10), 1);
  EXPECT_EQ(Channels(demuxer), 2);
}

TEST(TestDVDDemuxClient, OpeningAStreamTakesAChangedAnnouncement)
{
  auto input = std::make_shared<CTestDemuxInputStream>(0);
  CDVDDemuxClient demuxer;
  ASSERT_TRUE(demuxer.Open(input));
  PlayPackets(demuxer, 10);
  ASSERT_EQ(Channels(demuxer), 2);

  input->Announce(6);
  demuxer.OpenStream(AUDIO_ID);

  EXPECT_EQ(Channels(demuxer), 6);
}

TEST(TestDVDDemuxClient, OpeningAStreamTakesAChangedAspect)
{
  auto input = std::make_shared<CTestVideoInputStream>(1.25);
  CDVDDemuxClient demuxer;
  ASSERT_TRUE(demuxer.Open(input));
  demuxer.OpenStream(AUDIO_ID);

  input->AnnounceAspect(1.75);
  demuxer.OpenStream(AUDIO_ID);

  EXPECT_EQ(static_cast<CDemuxStreamVideo*>(demuxer.GetStream(AUDIO_ID))->fAspect, 1.75);
}

TEST(TestDVDDemuxClient, AStreamThatChangesCodecTakesTheAnnouncedProfile)
{
  auto input = std::make_shared<CTestDemuxInputStream>(2);
  CDVDDemuxClient demuxer;
  ASSERT_TRUE(demuxer.Open(input));
  demuxer.OpenStream(AUDIO_ID);

  input->Announce(AV_CODEC_ID_AAC, AV_PROFILE_AAC_HE);
  input->SignalStreamChange();
  CDVDDemuxUtils::FreeDemuxPacket(demuxer.Read());
  demuxer.OpenStream(AUDIO_ID);

  EXPECT_EQ(demuxer.GetStream(AUDIO_ID)->profile, AV_PROFILE_AAC_HE);
}
