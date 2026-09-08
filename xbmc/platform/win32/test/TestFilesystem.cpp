/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "platform/Filesystem.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"

#include <gtest/gtest.h>

TEST(TestWin32Filesystem, TempFilePathsAreReserved)
{
  for (const auto* suffix : {"", ".chd", ".m3u", ".sav"})
  {
    SCOPED_TRACE(suffix);
    std::error_code ec;
    const auto path = KODI::PLATFORM::FILESYSTEM::temp_file_path(suffix, ec);
    ASSERT_FALSE(ec) << ec.message();
    ASSERT_FALSE(path.empty());
    EXPECT_TRUE(StringUtils::EndsWith(path, suffix));
    EXPECT_TRUE(XFILE::CFile::Exists(path));
    EXPECT_TRUE(XFILE::CFile::Delete(path));
  }
}

TEST(TestWin32Filesystem, TempFilesPreserveSuffix)
{
  for (const auto* suffix : {"", ".chd", ".m3u", ".sav"})
  {
    SCOPED_TRACE(suffix);
    auto* file = XBMC_CREATETEMPFILE(suffix);
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(StringUtils::EndsWith(XBMC_TEMPFILEPATH(file), suffix));
    EXPECT_EQ(file->Write("test", 4), 4);
    EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
  }
}
