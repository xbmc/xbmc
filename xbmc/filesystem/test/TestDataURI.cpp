/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/DataURI.h"

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

TEST(TestDataURI, PercentEncoded)
{
  std::size_t decodedSize{0};
  ASSERT_TRUE(XFILE::DataURI::Validate(CURL{"data:text/plain,Hello%20world"}, decodedSize));
  EXPECT_EQ(11U, decodedSize);

  std::string decoded;
  ASSERT_TRUE(XFILE::DataURI::Materialize(CURL{"data:text/plain,Hello%20world"}, decoded));
  EXPECT_EQ("Hello world", decoded);
}

TEST(TestDataURI, Base64Binary)
{
  std::size_t decodedSize{0};
  ASSERT_TRUE(
      XFILE::DataURI::Validate(CURL{"data:application/octet-stream;base64,AAECAP8="}, decodedSize));
  EXPECT_EQ(5U, decodedSize);

  std::string decoded;
  ASSERT_TRUE(
      XFILE::DataURI::Materialize(CURL{"data:application/octet-stream;base64,AAECAP8="}, decoded));
  EXPECT_EQ(std::string("\x00\x01\x02\x00\xFF", 5), decoded);
}

TEST(TestDataURI, SchemeSpecificPayload)
{
  const std::string uri{"DATA:text/plain,+/:=%25?,;https://example.test/a#fragment?ignored"};
  CURL explicitURI;
  ASSERT_TRUE(explicitURI.ParseURI(uri));

  for (const CURL& url : {CURL{uri}, explicitURI})
  {
    std::size_t decodedSize{0};
    ASSERT_TRUE(XFILE::DataURI::Validate(url, decodedSize));
    EXPECT_EQ(30U, decodedSize);

    std::string decoded;
    ASSERT_TRUE(XFILE::DataURI::Materialize(url, decoded));
    EXPECT_EQ("+/:=%?,;https://example.test/a", decoded);
  }
}

TEST(TestDataURI, RejectsAuthorityAndMalformedQueryPayload)
{
  for (const auto* input :
       {"data://host/text/plain,hello", "data:///text/plain,hello", "data:/text/plain,hello",
        "data:,hello?bad%", "data:,hello?bad|value", "other:,hello"})
  {
    SCOPED_TRACE(input);
    CURL url;
    url.ParseURI(input);

    std::size_t decodedSize{0};
    EXPECT_FALSE(XFILE::DataURI::Validate(url, decodedSize));

    std::string decoded;
    EXPECT_FALSE(XFILE::DataURI::Materialize(url, decoded));
  }
}

TEST(TestDataURI, FailurePreservesOutputs)
{
  for (const auto* input :
       {"data:text%/plain,hello", "data:text/plain;name=bad%,hello", "data:,hello%20world%",
        "data:,hello%20world bad", "data:;base64,Zm9vZh==", "data:;base64,Zm9vZg==A",
        "data:;base64,Zm9vZg=", "data:;base64,Zm9v%0A bad"})
  {
    SCOPED_TRACE(input);
    const CURL url{input};
    std::size_t decodedSize{123};
    EXPECT_FALSE(XFILE::DataURI::Validate(url, decodedSize));
    EXPECT_EQ(123U, decodedSize);

    std::string decoded{"unchanged"};
    EXPECT_FALSE(XFILE::DataURI::Materialize(url, decoded));
    EXPECT_EQ("unchanged", decoded);
  }
}
