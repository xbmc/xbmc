/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/ScraperParser.h"

#include <gtest/gtest.h>

TEST(TestScraperParser, General)
{
  CScraperParser a;

  a.Clear();
  EXPECT_TRUE(a.Load(XBMC_REF_FILE_PATH("/addons/metadata.local/local.xml")));

  EXPECT_STREQ(XBMC_REF_FILE_PATH("/addons/metadata.local/local.xml").c_str(),
               a.GetFilename().c_str());
  EXPECT_STREQ("UTF-8", a.GetSearchStringEncoding().c_str());
}
