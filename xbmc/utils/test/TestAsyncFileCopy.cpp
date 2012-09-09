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

#include "utils/AsyncFileCopy.h"
#include "filesystem/File.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

static const char refdata[] = "\x01\x02\x03\x04\x05\x06\x07\x08"
                              "\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10"
                              "\x11\x12\x13\x14\x15\x16\x17\x18"
                              "\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20"
                              "\x21\x22\x23\x24\x25\x26\x27\x28"
                              "\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30";

TEST(TestAsyncFileCopy, General)
{
  CAsyncFileCopy c;
  XFILE::CFile *f1, *f2;
  char vardata[sizeof(refdata)];

  ASSERT_TRUE((f1 = XBMC_CREATETEMPFILE("")));
  ASSERT_TRUE((f2 = XBMC_CREATETEMPFILE(".copy")));

  EXPECT_EQ((int)sizeof(refdata), f1->Write(refdata, sizeof(refdata)));
  f1->Close();
  f2->Close();
  EXPECT_TRUE(c.Copy(XBMC_TEMPFILEPATH(f1), XBMC_TEMPFILEPATH(f2), ""));
  EXPECT_TRUE(f2->Open(XBMC_TEMPFILEPATH(f2)));
  EXPECT_EQ(sizeof(refdata), f2->Read(vardata, sizeof(refdata)));
  f2->Close();
  EXPECT_TRUE(!memcmp(vardata, refdata, sizeof(refdata)));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(f1));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(f2));
}
