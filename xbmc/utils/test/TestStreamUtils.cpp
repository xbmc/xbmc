/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
