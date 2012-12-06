/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StreamUtils.h"

#include "gtest/gtest.h"

TEST(TestStreamUtils, General)
{
  EXPECT_EQ(0, StreamUtils::GetCodecPriority(""));
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("ac3"));
  EXPECT_EQ(2, StreamUtils::GetCodecPriority("dca"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("eac3"));
  EXPECT_EQ(4, StreamUtils::GetCodecPriority("dtshd_hra"));
  EXPECT_EQ(5, StreamUtils::GetCodecPriority("dtshd_ma"));
  EXPECT_EQ(6, StreamUtils::GetCodecPriority("truehd"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("flac"));
}
