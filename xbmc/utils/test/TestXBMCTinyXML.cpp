/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <gtest/gtest.h>

TEST(TestXBMCTinyXML, ParseFromString)
{
  bool retval = false;
  // scraper results with unescaped &
  CXBMCTinyXML doc;
  std::string data("<details><url function=\"ParseTMDBRating\" "
                  "cache=\"tmdb-en-12244.json\">"
                  "http://api.themoviedb.org/3/movie/12244"
                  "?api_key=57983e31fb435df4df77afb854740ea9"
                  "&language=en&#x3f;&#x003F;&#0063;</url></details>");
  doc.Parse(data);
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
  FILE *f = fopen(XBMC_REF_FILE_PATH("/xbmc/utils/test/CXBMCTinyXML-test.xml").c_str(), "r");
  ASSERT_NE(nullptr, f);
  doc.LoadFile(f);
  fclose(f);
  TiXmlNode *root = doc.RootElement();
  if (root && root->ValueStr() == "details")
  {
    TiXmlElement *url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      std::string str = url->FirstChild()->ValueStr();
      retval = (StringUtils::Trim(str) == "http://api.themoviedb.org/3/movie/12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???");
    }
  }
  EXPECT_TRUE(retval);
}
