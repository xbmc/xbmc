/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"

#include <gtest/gtest.h>

using namespace ActiveAE;

TEST(TestActiveAEBuffer, AtempoPoolTakesTempoAboveTwo)
{
  // 2x tempo with sync to display speeding the clock up further, eg. 2.085x for 23.976 fps on
  // 100 Hz, needs audio above 2x to keep up
  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_FLOAT;
  format.m_sampleRate = 48000;
  format.m_channelLayout = AE_CH_LAYOUT_2_0;
  format.m_frames = 1024;
  format.m_frameSize = format.m_channelLayout.Count() * sizeof(float);

  CActiveAEBufferPoolAtempo pool(format);
  ASSERT_TRUE(pool.Create(100));

  pool.SetTempo(2.085f);
  EXPECT_FLOAT_EQ(pool.GetTempo(), 2.085f);
  pool.SetTempo(4.17f);
  EXPECT_FLOAT_EQ(pool.GetTempo(), 4.17f);
}
