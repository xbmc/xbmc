/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/ScraperParser.h"

#include <string>

#include <gtest/gtest.h>

TEST(TestScraperParser, General)
{
  const std::string path{CSpecialProtocol::TranslatePath("special://temp/TestScraperParser.xml")};
  const std::string scraper{R"(<?xml version="1.0" encoding="UTF-8"?><scraper framework="1.1"/>)"};
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(path, true));
  ASSERT_EQ(static_cast<ssize_t>(scraper.size()), file.Write(scraper.data(), scraper.size()));
  file.Close();

  CScraperParser a;

  a.Clear();
  EXPECT_TRUE(a.Load(path));

  EXPECT_STREQ("UTF-8", a.GetSearchStringEncoding().c_str());
  XFILE::CFile::Delete(path);
}
