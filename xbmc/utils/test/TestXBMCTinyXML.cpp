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

#include "utils/XBMCTinyXML.h"
#include "test/TestUtils.h"

#include "gtest/gtest.h"

TEST(TestXBMCTinyXML, ParseFromString)
{
  bool retval = false;
  // scraper results with unescaped &
  CXBMCTinyXML doc;
  CStdString data("<details><url function=\"ParseTMDBRating\" "
                  "cache=\"tmdb-en-12244.json\">"
                  "http://api.themoviedb.org/3/movie/12244"
                  "?api_key=57983e31fb435df4df77afb854740ea9"
                  "&language=en&#x3f;&#x003F;&#0063;</url></details>");
  doc.Parse(data.c_str());
  TiXmlNode *root = doc.RootElement();
  if (root && root->ValueStr() == "details")
  {
    TiXmlElement *url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      retval = (url->FirstChild()->ValueStr() == "http://api.themoviedb.org/3/movie/12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???");
    }
  }
  EXPECT_TRUE(retval);
}

TEST(TestXBMCTinyXML, ParseFromFileHandle)
{
  bool retval = false;
  // scraper results with unescaped &
  CXBMCTinyXML doc;
  FILE *f = fopen(XBMC_REF_FILE_PATH("/xbmc/utils/test/CXBMCTinyXML-test.xml"), "r");
  ASSERT_TRUE(f);
  doc.LoadFile(f);
  fclose(f);
  TiXmlNode *root = doc.RootElement();
  if (root && root->ValueStr() == "details")
  {
    TiXmlElement *url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      CStdString str = url->FirstChild()->ValueStr();
      retval = (str.Trim() == "http://api.themoviedb.org/3/movie/12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???");
    }
  }
  EXPECT_TRUE(retval);
}
