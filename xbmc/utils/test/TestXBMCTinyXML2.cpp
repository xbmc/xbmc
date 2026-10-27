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
#include "utils/XBMCTinyXML.h"
#include "utils/XBMCTinyXML2.h"

#include <array>
#include <string>
#include <string_view>

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

namespace
{
struct CharsetTest
{
  std::string_view name;
  std::string_view data;
  std::string_view charset;
  std::string_view usedCharset;
  std::string_view text;
};

constexpr std::string_view CAFE_UTF8{"caf\xC3\xA9"};

// CXBMCTinyXML is the reference, CXBMCTinyXML2 must produce the same results
constexpr auto charsetTests = std::array{
    CharsetTest{"Utf8", "<a>caf\xC3\xA9</a>", "", "UTF-8", CAFE_UTF8},
    CharsetTest{"Utf8Bom", "\xEF\xBB\xBF<a>caf\xC3\xA9</a>", "", "UTF-8", CAFE_UTF8},
    CharsetTest{"Latin1Declared", "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><a>caf\xE9</a>",
                "", "ISO-8859-1", CAFE_UTF8},
    CharsetTest{"Latin1Undeclared", "<a>caf\xE9</a>", "", "CP1252", CAFE_UTF8},
    CharsetTest{"InvalidUtf8Declared",
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?><a>caf\xE9 au lait</a>", "", "UTF-8",
                "caf\xE9 au lait"},
    CharsetTest{"SuggestedCharset", "<a>caf\xE9</a>", "iso-8859-1", "ISO-8859-1", CAFE_UTF8},
    CharsetTest{"WrongSuggestedCharset", "<a>caf\xC3\xA9</a>", "UTF-16LE", "UTF-8", CAFE_UTF8},
    // TinyXML1 reports a parse error for unconverted data without trailing whitespace
    CharsetTest{"Unconvertible", "<a>\x81</a>\n", "", "", "\x81"},
};

class TestXBMCTinyXML2Charset : public testing::TestWithParam<CharsetTest>
{
};
} // namespace

TEST_P(TestXBMCTinyXML2Charset, ParseTinyXML1)
{
  const CharsetTest& test = GetParam();
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{test.data}, std::string{test.charset}));
  ASSERT_NE(nullptr, doc.RootElement());
  EXPECT_STREQ(std::string{test.text}.c_str(), doc.RootElement()->GetText());
  EXPECT_EQ(test.usedCharset, doc.GetUsedCharset());
}

TEST_P(TestXBMCTinyXML2Charset, LoadFileTinyXML1)
{
  const CharsetTest& test = GetParam();
  if (!test.charset.empty())
    GTEST_SKIP() << "LoadFile takes the suggested charset from the file";

  XFILE::CFile* file = XBMC_CREATETEMPFILE(".xml");
  ASSERT_NE(nullptr, file);
  ASSERT_EQ(static_cast<ssize_t>(test.data.size()),
            file->Write(test.data.data(), test.data.size()));
  file->Close();
  const std::string path = XBMC_TEMPFILEPATH(file);

  CXBMCTinyXML doc;
  const bool loaded = doc.LoadFile(path);
  XBMC_DELETETEMPFILE(file);
  ASSERT_TRUE(loaded);
  ASSERT_NE(nullptr, doc.RootElement());
  EXPECT_STREQ(std::string{test.text}.c_str(), doc.RootElement()->GetText());
  EXPECT_EQ(test.usedCharset, doc.GetUsedCharset());
}

INSTANTIATE_TEST_SUITE_P(TestXBMCTinyXML2,
                         TestXBMCTinyXML2Charset,
                         testing::ValuesIn(charsetTests),
                         [](const testing::TestParamInfo<CharsetTest>& info)
                         { return std::string{info.param.name}; });
