/*
 *  Copyright (C) 2025 Team Kodi
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
  EXPECT_EQ(CAEBitstreamPacker::GetOutputRate(info), 192000u);
}

TEST(TestAEBitstreamPacker, TrueHDOutputRate192kAt96k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
  info.m_sampleRate = 96000;
  EXPECT_EQ(CAEBitstreamPacker::GetOutputRate(info), 192000u);
}

TEST(TestAEBitstreamPacker, TrueHDOutputRate176kAt44k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
  info.m_sampleRate = 44100;
  EXPECT_EQ(CAEBitstreamPacker::GetOutputRate(info), 176400u);
}

TEST(TestAEBitstreamPacker, TrueHDOutputChannels8)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
  info.m_sampleRate = 0;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(channels.Count(), 8u);
}

TEST(TestAEBitstreamPacker, DTSHDMAOutputRate192k)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_MA;
  info.m_sampleRate = 48000;
  EXPECT_EQ(CAEBitstreamPacker::GetOutputRate(info), 192000u);
}

TEST(TestAEBitstreamPacker, DTSHDMAOutputChannels8)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_MA;
  info.m_sampleRate = 0;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(channels.Count(), 8u);
}

TEST(TestAEBitstreamPacker, AC3OutputChannels2)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
  info.m_sampleRate = 0;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(channels.Count(), 2u);
}

TEST(TestAEBitstreamPacker, EAC3OutputRate4x)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_EAC3;
  info.m_sampleRate = 48000;
  EXPECT_EQ(CAEBitstreamPacker::GetOutputRate(info), 192000u);
}

TEST(TestAEBitstreamPacker, DTS512OutputChannels2)
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_DTS_512;
  info.m_sampleRate = 0;
  CAEChannelInfo channels = CAEBitstreamPacker::GetOutputChannelMap(info);
  EXPECT_EQ(channels.Count(), 2u);
}
