/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Sinks/AESinkNULL.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"

#include <chrono>
#include <string>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(TestAESinkNULL, RawStreamReportsPackedFrameSize)
{
  // a raw stream as requested by ActiveAE, its frame size is a byte
  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_RAW;
  format.m_sampleRate = 192000;
  format.m_channelLayout = AE_CH_LAYOUT_7_1;
  format.m_frames = 19200;
  format.m_frameSize = 1;

  CAESinkNULL sink(0s, 0s);
  std::string device = "NULL";
  ASSERT_TRUE(sink.Initialize(format, device));

  constexpr unsigned int channels = 8;
  constexpr unsigned int bytesPerSample = 2;
  EXPECT_EQ(AE_FMT_S16NE, format.m_dataFormat);
  EXPECT_EQ(channels * bytesPerSample, format.m_frameSize);
}
