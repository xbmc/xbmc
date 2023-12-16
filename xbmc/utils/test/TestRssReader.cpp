/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/RssReader.h"

#include <cstring>
#include <fstream>
#include <iosfwd>
#include <string>
#include <vector>

#include <gtest/gtest.h>

class TestRssReader : public testing::Test
{
};

TEST(TestRssReader, ParseCorrectRss)
{
  std::ifstream fstr(XBMC_REF_FILE_PATH("/xbmc/utils/test/rss.xml"));
  std::stringstream buffer;
  buffer << fstr.rdbuf();

  std::vector<std::string> urls{"dummy"};
  std::vector<int> times{0};
  CRssReader rssReader;
  rssReader.Create(nullptr, urls, times, 0, false);

  EXPECT_TRUE(rssReader.Parse(buffer.str(), 0));
}

TEST(TestRssReader, ParseCorruptRss)
{
  std::ifstream fstr(XBMC_REF_FILE_PATH("/xbmc/utils/test/rss.xml"));
  std::stringstream buffer;
  buffer << fstr.rdbuf();

  std::string content = buffer.str();
  auto xmlTagPos = content.find("</pubDate>");
  content.replace(xmlTagPos, strlen("</pubDate>"), "</pubDatee>");

  std::vector<std::string> urls{"dummy"};
  std::vector<int> times{0};
  CRssReader rssReader;
  rssReader.Create(nullptr, urls, times, 0, false);

  EXPECT_FALSE(rssReader.Parse(content, 0));
}
