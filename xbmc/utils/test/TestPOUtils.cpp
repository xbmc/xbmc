/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "utils/POUtils.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"


TEST(TestPOUtils, General)
{
  CPODocument a;

  EXPECT_TRUE(a.LoadFile(XBMC_REF_FILE_PATH("xbmc/utils/test/data/language/Spanish/strings.po")));

  EXPECT_TRUE(a.GetNextEntry());
  EXPECT_EQ(ID_FOUND, a.GetEntryType());
  EXPECT_EQ((uint32_t)0, a.GetEntryID());
  a.ParseEntry(false);
  EXPECT_STREQ("", a.GetMsgctxt().c_str());
  EXPECT_STREQ("Programs", a.GetMsgid().c_str());
  EXPECT_STREQ("Programas", a.GetMsgstr().c_str());
  EXPECT_STREQ("", a.GetPlurMsgstr(0).c_str());

  EXPECT_TRUE(a.GetNextEntry());
  EXPECT_EQ(ID_FOUND, a.GetEntryType());
  EXPECT_EQ((uint32_t)1, a.GetEntryID());
  a.ParseEntry(false);
  EXPECT_STREQ("", a.GetMsgctxt().c_str());
  EXPECT_STREQ("Pictures", a.GetMsgid().c_str());
  EXPECT_STREQ("Imágenes", a.GetMsgstr().c_str());
  EXPECT_STREQ("", a.GetPlurMsgstr(0).c_str());

  EXPECT_TRUE(a.GetNextEntry());
  EXPECT_EQ(ID_FOUND, a.GetEntryType());
  EXPECT_EQ((uint32_t)2, a.GetEntryID());
  a.ParseEntry(false);
  EXPECT_STREQ("", a.GetMsgctxt().c_str());
  EXPECT_STREQ("Music", a.GetMsgid().c_str());
  EXPECT_STREQ("Música", a.GetMsgstr().c_str());
  EXPECT_STREQ("", a.GetPlurMsgstr(0).c_str());
}
