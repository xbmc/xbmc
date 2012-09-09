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

#include "utils/ScraperParser.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

TEST(TestScraperParser, General)
{
  CScraperParser a;

  a.Clear();
  EXPECT_TRUE(
    a.Load(XBMC_REF_FILE_PATH("/addons/metadata.themoviedb.org/tmdb.xml")));

  EXPECT_STREQ(
    XBMC_REF_FILE_PATH("/addons/metadata.themoviedb.org/tmdb.xml").c_str(),
    a.GetFilename().c_str());
  EXPECT_STREQ("UTF-8", a.GetSearchStringEncoding().c_str());
}
