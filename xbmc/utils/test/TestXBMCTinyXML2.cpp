/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <gtest/gtest.h>

class TestXBMCTinyXML2 : public testing::Test
{
};

TEST(TestXBMCTinyXML2, ParseFromString)
{
  bool retval = false;
  // scraper results with unescaped &
  CXBMCTinyXML2 doc;
  std::string data("<details><url function=\"ParseTMDBRating\" "
                   "cache=\"tmdb-en-12244.json\">"
                   "http://api.themoviedb.org/3/movie/12244"
                   "?api_key=57983e31fb435df4df77afb854740ea9"
                   "&language=en&#x3f;&#x003F;&#0063;</url></details>");
  doc.Parse(data);
  auto* root = doc.RootElement();
  if (root && (strcmp(root->Value(), "details") == 0))
  {
    auto* url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      retval = (strcmp(url->FirstChild()->Value(),
                       "http://api.themoviedb.org/3/movie/"
                       "12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???") == 0);
    }
  }
  EXPECT_TRUE(retval);
}

TEST(TestXBMCTinyXML2, ParseFromChar)
{
  bool retval = false;
  // scraper results with unescaped &
  CXBMCTinyXML2 doc;
  std::string testfile = XBMC_REF_FILE_PATH("/xbmc/utils/test/CXBMCTinyXML-test.xml");
  bool load = doc.LoadFile(testfile);
  EXPECT_TRUE(load);

  auto* root = doc.RootElement();
  bool rootval = (strcmp(root->Value(), "details") == 0);
  EXPECT_TRUE(rootval);
  if (root && rootval)
  {
    auto* url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      std::string str{url->FirstChild()->Value()};
      retval = (StringUtils::Trim(str) ==
                "http://api.themoviedb.org/3/movie/"
                "12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???");
    }
  }
  EXPECT_TRUE(retval);
}

TEST(TestXBMCTinyXML2, ParseFromCharFail)
{
  // scraper results with unescaped &
  CXBMCTinyXML2 doc;
  std::string testfile = XBMC_REF_FILE_PATH("/xbmc/utils/test/Non-existant-CXBMCTinyXML-test.xml");
  bool load = doc.LoadFile(testfile);
  EXPECT_FALSE(load);
}

TEST(TestXBMCTinyXML2, ParseFromFileHandle)
{
  bool retval = false;
  // scraper results with unescaped &
  CXBMCTinyXML2 doc;
  FILE* f = fopen(XBMC_REF_FILE_PATH("/xbmc/utils/test/CXBMCTinyXML-test.xml").c_str(), "r");
  ASSERT_NE(nullptr, f);
  doc.LoadFile(f);
  fclose(f);
  auto* root = doc.RootElement();
  if (root && (strcmp(root->Value(), "details") == 0))
  {
    auto* url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      std::string str{url->FirstChild()->Value()};
      retval = (StringUtils::Trim(str) ==
                "http://api.themoviedb.org/3/movie/"
                "12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???");
    }
  }
  EXPECT_TRUE(retval);
}

#if defined(TARGET_WINDOWS)
// Windows fails this test. Something to do with XBMC_TEMPFILEPATH
// Todo: fix this
TEST(TestXBMCTinyXML2, DISABLED_SaveFile)
#else
TEST(TestXBMCTinyXML2, SaveFile)
#endif
{
  bool retval = false;
  // scraper results with unescaped &
  CXBMCTinyXML2 outputdoc;
  std::string data("<details><url function=\"ParseTMDBRating\" "
                   "cache=\"tmdb-en-12244.json\">"
                   "http://api.themoviedb.org/3/movie/12244"
                   "?api_key=57983e31fb435df4df77afb854740ea9"
                   "&language=en&#x3f;&#x003F;&#0063;</url></details>");
  outputdoc.Parse(data);

  XFILE::CFile* file;
  file = XBMC_CREATETEMPFILE(".xml");
  std::string xmlfile = XBMC_TEMPFILEPATH(file);

  EXPECT_TRUE(outputdoc.SaveFile(xmlfile));
  file->Close();

  CXBMCTinyXML2 inputdoc;
  FILE* f = fopen(xmlfile.c_str(), "r");
  ASSERT_NE(nullptr, f);
  inputdoc.LoadFile(f);
  fclose(f);
  XBMC_DELETETEMPFILE(file);

  auto* root = inputdoc.RootElement();
  if (root && (strcmp(root->Value(), "details") == 0))
  {
    auto* url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      retval = (strcmp(url->FirstChild()->Value(),
                       "http://api.themoviedb.org/3/movie/"
                       "12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???") == 0);
    }
  }
  EXPECT_TRUE(retval);
}
