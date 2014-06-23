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

#include "utils/ScraperUrl.h"

#include "gtest/gtest.h"

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
