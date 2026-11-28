/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/FTPParse.h"

#include <gtest/gtest.h>

TEST(TestFTPParse, UnixListing)
{
  CFTPParse parse;
  ASSERT_EQ(1, parse.FTPParse("-rw-r--r-- 1 owner group 1234 Jan 01 2024 file.txt"));
  EXPECT_EQ(1, parse.getFlagtryretr());
  EXPECT_EQ(0, parse.getFlagtrycwd());
  EXPECT_EQ(1234, parse.getSize());
  EXPECT_EQ("file.txt", parse.getName());
}

TEST(TestFTPParse, UnixListingDirectory)
{
  CFTPParse parse;
  ASSERT_EQ(1, parse.FTPParse("drwxr-xr-x 2 owner group 4096 Jan 01 2024 dir name"));
  EXPECT_EQ(1, parse.getFlagtrycwd());
  EXPECT_EQ(0, parse.getFlagtryretr());
  EXPECT_EQ("dir name", parse.getName());
}

TEST(TestFTPParse, UnixListingOversizedNumber)
{
  CFTPParse parse;
  EXPECT_EQ(1,
            parse.FTPParse(
                "-rw-r--r-- 1 owner group 99999999999999999999999 Jan 01 2024 file.txt"));
  EXPECT_EQ(0, parse.getSize());
  EXPECT_EQ("file.txt", parse.getName());
}

TEST(TestFTPParse, EplfListing)
{
  CFTPParse parse;
  ASSERT_EQ(1, parse.FTPParse("+i8388608.296938,m1698680000,r,s1234, file.txt"));
  EXPECT_EQ(1, parse.getFlagtryretr());
  EXPECT_EQ(1234, parse.getSize());
  EXPECT_EQ(1698680000, parse.getTime());
  EXPECT_EQ("file.txt", parse.getName());
}

TEST(TestFTPParse, EplfTimestampRange)
{
  CFTPParse parse;
  ASSERT_EQ(1, parse.FTPParse("+i1,m4102444800,r, file.txt"));
  EXPECT_EQ(sizeof(time_t) >= 8 ? 4102444800 : 0, parse.getTime());

  CFTPParse oversized;
  ASSERT_EQ(1, oversized.FTPParse("+i1,m99999999999999999999,r, file.txt"));
  EXPECT_EQ(0, oversized.getTime());
}

TEST(TestFTPParse, EplfListingMissingFacts)
{
  CFTPParse parse;
  EXPECT_EQ(1, parse.FTPParse("+i8388608.296938,r, file.txt"));
  EXPECT_EQ(1, parse.getFlagtryretr());
  EXPECT_EQ(0, parse.getSize());
  EXPECT_EQ(0, parse.getTime());
  EXPECT_EQ("file.txt", parse.getName());
}

TEST(TestFTPParse, UnreadableNumbersAreZero)
{
  for (const char* line : {
           "- [RWCEAFMS] owner 99999999999999999999999 Jan 01 2024 file.txt", // NetWare
           "drwxrwxr-x               folder   Jan 01  2024 file.txt", // NetPresenz, no size
           "01-01-24  10:00AM 99999999999999999999999 file.txt", // MSDOS
           "-rw-r--r-- 1 owner group 0 Jan 01 :123 file.txt", // Unix, unreadable year
       })
  {
    CFTPParse parse;
    EXPECT_EQ(1, parse.FTPParse(line)) << line;
    EXPECT_EQ(0, parse.getSize()) << line;
    EXPECT_EQ("file.txt", parse.getName()) << line;
  }
}
