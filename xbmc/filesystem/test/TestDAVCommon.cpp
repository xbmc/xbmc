/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "filesystem/DAVCommon.h"
#include "filesystem/IDirectory.h"
#include "utils/XBMCTinyXML2.h"

#include <clocale>
#include <string>

#include <gtest/gtest.h>
#include <sys/stat.h>

using namespace XFILE;

TEST(TestDAVCommon, ParseHttpDate)
{
  time_t result = 0;
  EXPECT_TRUE(CDAVCommon::ParseHttpDate("Thu, 01 Jan 2026 12:00:00 GMT", result));
  EXPECT_EQ(result, 1767268800);
  EXPECT_FALSE(CDAVCommon::ParseHttpDate("garbage", result));
}

TEST(TestDAVCommon, ParseHttpDateNonEnglishLocale)
{
  const std::string previous{setlocale(LC_TIME, nullptr)};
  if (!setlocale(LC_TIME, "de_DE.UTF-8"))
    GTEST_SKIP() << "de_DE.UTF-8 locale not available";

  time_t result = 0;
  const bool parsed{CDAVCommon::ParseHttpDate("Thu, 01 Jan 2026 12:00:00 GMT", result)};
  setlocale(LC_TIME, previous.c_str());

  EXPECT_TRUE(parsed);
  EXPECT_EQ(result, 1767268800);
}

TEST(TestDAVCommon, ParseIsoDateTime)
{
  time_t result = 0;
  EXPECT_TRUE(CDAVCommon::ParseIsoDateTime("2026-01-01T12:00:00Z", result));
  EXPECT_EQ(result, 1767268800);
  EXPECT_TRUE(CDAVCommon::ParseIsoDateTime("2026-01-01T12:00:00", result));
  EXPECT_EQ(result, 1767268800);
  EXPECT_TRUE(CDAVCommon::ParseIsoDateTime("2026-01-01T14:00:00.5+02:00", result));
  EXPECT_EQ(result, 1767268800);
  EXPECT_FALSE(CDAVCommon::ParseIsoDateTime("2026-01-01T12:00:00+25:00", result));
}

namespace
{
const tinyxml2::XMLElement* FirstResponse(const std::string& multistatus)
{
  static thread_local CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(std::string(multistatus)));
  for (auto* child = doc.RootElement()->FirstChildElement(); child;
       child = child->NextSiblingElement())
  {
    if (CDAVCommon::ValueWithoutNamespace(child, "response"))
      return child;
  }
  return nullptr;
}

std::string StatResponseXml(const std::string& innerProps, const char* status = "HTTP/1.1 200 OK")
{
  return std::string("<D:multistatus xmlns:D=\"DAV:\"><D:response><D:href>/dav/x</D:href>"
                     "<D:propstat><D:prop>") +
         innerProps + "</D:prop><D:status>" + status +
         "</D:status></D:propstat></D:response></D:multistatus>";
}
} // namespace

TEST(TestDAVCommon, StatResponseFileWithLength)
{
  struct __stat64 st
  {
  };
  const auto* response = FirstResponse(
      StatResponseXml("<D:resourcetype/><D:getcontentlength>1234</D:getcontentlength>"
                      "<D:getlastmodified>Thu, 01 Jan 2026 12:00:00 GMT</D:getlastmodified>")
          .c_str());
  ASSERT_NE(nullptr, response);
  ASSERT_TRUE(CDAVCommon::ParseDavStatResponse(response, st));
  EXPECT_TRUE(S_ISREG(st.st_mode));
  EXPECT_EQ(1234, st.st_size);
  EXPECT_EQ(1767268800, st.st_mtime);
}

TEST(TestDAVCommon, StatResponseCollectionWithoutLengthIsDirectory)
{
  struct __stat64 st
  {
  };
  const auto* response =
      FirstResponse(StatResponseXml("<D:resourcetype><D:collection/></D:resourcetype>").c_str());
  ASSERT_NE(nullptr, response);
  ASSERT_TRUE(CDAVCommon::ParseDavStatResponse(response, st));
  EXPECT_TRUE(S_ISDIR(st.st_mode));
}

TEST(TestDAVCommon, StatResponseFileWithoutLengthFallsBack)
{
  struct __stat64 st
  {
  };
  const auto* response =
      FirstResponse(StatResponseXml("<D:resourcetype/><D:getlastmodified>"
                                    "Thu, 01 Jan 2026 12:00:00 GMT</D:getlastmodified>")
                        .c_str());
  ASSERT_NE(nullptr, response);
  EXPECT_FALSE(CDAVCommon::ParseDavStatResponse(response, st));
}

TEST(TestDAVCommon, StatResponseSkipsNon200Propstat)
{
  struct __stat64 st
  {
  };
  const auto* response =
      FirstResponse(StatResponseXml("<D:resourcetype/><D:getcontentlength>5</D:getcontentlength>",
                                    "HTTP/1.1 404 Not Found")
                        .c_str());
  ASSERT_NE(nullptr, response);
  EXPECT_FALSE(CDAVCommon::ParseDavStatResponse(response, st));
}

TEST(TestDAVCommon, DirectoryResponseSetsMtimeOnlyWithCompleteStat)
{
  const char* fileProps = "<D:resourcetype/><D:getcontentlength>10</D:getcontentlength>"
                          "<D:getlastmodified>Thu, 01 Jan 2026 12:00:00 GMT</D:getlastmodified>";
  const char* dirProps = "<D:resourcetype><D:collection/></D:resourcetype>"
                         "<D:getlastmodified>Thu, 01 Jan 2026 12:00:00 GMT</D:getlastmodified>";
  const char* noLengthProps = "<D:resourcetype/><D:getlastmodified>"
                              "Thu, 01 Jan 2026 12:00:00 GMT</D:getlastmodified>";

  {
    CFileItem item;
    CDAVCommon::ParseDavDirectoryResponse(FirstResponse(StatResponseXml(fileProps).c_str()), item);
    EXPECT_EQ("1767268800", item.GetProperty(DIR_PROPERTY_STAT_MTIME).asString());
  }
  {
    CFileItem item;
    CDAVCommon::ParseDavDirectoryResponse(FirstResponse(StatResponseXml(dirProps).c_str()), item);
    EXPECT_EQ("1767268800", item.GetProperty(DIR_PROPERTY_STAT_MTIME).asString());
  }
  {
    CFileItem item;
    CDAVCommon::ParseDavDirectoryResponse(FirstResponse(StatResponseXml(noLengthProps).c_str()),
                                          item);
    EXPECT_TRUE(item.GetProperty(DIR_PROPERTY_STAT_MTIME).asString().empty());
  }
}
