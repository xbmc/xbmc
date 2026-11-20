/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Sinks/AESinkNULL.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"

#include <string>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(TestAESinkNULL, RawStreamTakesThePackedFrameSize)
{
  // The engine gives a raw stream a frame size of one byte; the sink is written IEC packed
  // 16-bit stereo, which is four bytes a frame.
  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_RAW;
  format.m_sampleRate = 48000;
  format.m_channelLayout = AE_CH_LAYOUT_2_0;
  format.m_frameSize = 1;

  CAESinkNULL sink(0s, 0s);
  std::string device;
  ASSERT_TRUE(sink.Initialize(format, device));

  EXPECT_EQ(format.m_dataFormat, AE_FMT_S16NE);
  EXPECT_EQ(format.m_frameSize, 4u);
}
