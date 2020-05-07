/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/ScraperUrl.h"

#include <gtest/gtest.h>

TEST(TestScraperUrl, General)
{
  CScraperUrl a;
  std::string xmlstring;

  xmlstring = "<data spoof=\"blah\" gzip=\"yes\">\n"
              "  <someurl>\n"
              "  </someurl>\n"
              "  <someotherurl>\n"
              "  </someotherurl>\n"
              "</data>\n";
  EXPECT_TRUE(a.ParseFromData(xmlstring));

  const auto url = a.GetFirstUrlByType();
  EXPECT_STREQ("blah", url.m_spoof.c_str());
  EXPECT_STREQ("someurl", url.m_url.c_str());
  EXPECT_STREQ("", url.m_cache.c_str());
  EXPECT_EQ(CScraperUrl::UrlType::General, url.m_type);
  EXPECT_FALSE(url.m_post);
  EXPECT_TRUE(url.m_isgz);
  EXPECT_EQ(-1, url.m_season);
}
