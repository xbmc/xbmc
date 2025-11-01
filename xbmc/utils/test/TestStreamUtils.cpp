/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StreamUtils.h"

#include <gtest/gtest.h>

TEST(TestStreamUtils, General)
{
  EXPECT_EQ(0, StreamUtils::GetCodecPriority(""));
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("ac3"));
  EXPECT_EQ(2, StreamUtils::GetCodecPriority("dca"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("eac3"));
  EXPECT_EQ(4, StreamUtils::GetCodecPriority("eac3_ddp_atmos"));
  EXPECT_EQ(5, StreamUtils::GetCodecPriority("dtshd_hra"));
  EXPECT_EQ(6, StreamUtils::GetCodecPriority("dtshd_ma"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("truehd"));
  EXPECT_EQ(8, StreamUtils::GetCodecPriority("flac"));
  EXPECT_EQ(9, StreamUtils::GetCodecPriority("dtshd_ma_x"));
  EXPECT_EQ(10, StreamUtils::GetCodecPriority("dtshd_ma_x_imax"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("truehd_atmos"));
}
