/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/POUtils.h"

#include <gtest/gtest.h>


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
