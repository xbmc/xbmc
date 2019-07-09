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
  EXPECT_TRUE(a.ParseString(xmlstring));

  EXPECT_STREQ("blah", a.GetFirstThumb().m_spoof.c_str());
  EXPECT_STREQ("someurl", a.GetFirstThumb().m_url.c_str());
  EXPECT_STREQ("", a.GetFirstThumb().m_cache.c_str());
  EXPECT_EQ(CScraperUrl::URL_TYPE_GENERAL, a.GetFirstThumb().m_type);
  EXPECT_FALSE(a.GetFirstThumb().m_post);
  EXPECT_TRUE(a.GetFirstThumb().m_isgz);
  EXPECT_EQ(-1, a.GetFirstThumb().m_season);
}
