/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include <gtest/gtest.h>

TEST(TestAEBitstreamPacker, TrueHDOutputRate192kAt48k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
  info.m_sampleRate = 48000;
  EXPECT_EQ(192000u, CAEBitstreamPacker::GetOutputRate(info));
}

TEST(TestAEBitstreamPacker, TrueHDOutputRate192kAt96k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
  info.m_sampleRate = 96000;
  EXPECT_EQ(192000u, CAEBitstreamPacker::GetOutputRate(info));
}

TEST(TestAEBitstreamPacker, TrueHDOutputRate176kAt44k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
  info.m_sampleRate = 44100;
  EXPECT_EQ(176400u, CAEBitstreamPacker::GetOutputRate(info));
}

TEST(TestAEBitstreamPacker, TrueHDOutputChannels8)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(8u, channels.Count());
}

TEST(TestAEBitstreamPacker, DTSHDMAOutputRate192k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_MA;
  info.m_sampleRate = 48000;
  EXPECT_EQ(192000u, CAEBitstreamPacker::GetOutputRate(info));
}

TEST(TestAEBitstreamPacker, DTSHDMAOutputChannels8)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_MA;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(8u, channels.Count());
}

TEST(TestAEBitstreamPacker, AC3OutputChannels2)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(2u, channels.Count());
}

TEST(TestAEBitstreamPacker, EAC3OutputRate192kAt48k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_EAC3;
  info.m_sampleRate = 48000;
  EXPECT_EQ(192000u, CAEBitstreamPacker::GetOutputRate(info));
}

TEST(TestAEBitstreamPacker, DTS512OutputChannels2)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_DTS_512;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(2u, channels.Count());
}
