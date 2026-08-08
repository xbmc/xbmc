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
