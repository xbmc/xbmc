/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"

#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

class TestCURL : public testing::Test
{
};

TEST_F(TestCURL, TestComparison)
{
  CURL url1;
  CURL url2;
  CURL url3("http://www.aol.com/index.html?t=9");
  CURL url4(url3);

  EXPECT_FALSE(url1 == url3);
  EXPECT_FALSE(url2 == url3);

  EXPECT_TRUE(url1 == url1);
  EXPECT_TRUE(url2 == url2);
  EXPECT_TRUE(url3 == url3);
  EXPECT_TRUE(url4 == url4);

  EXPECT_TRUE(url1 == url2);
  EXPECT_TRUE(url3 == url4);

  EXPECT_TRUE(url3 == "http://www.aol.com/index.html?t=9");
  EXPECT_TRUE("http://www.aol.com/index.html?t=9" == url3);

  EXPECT_FALSE(url3 == "http://www.microsoft.com/index.html?t=9");
  EXPECT_FALSE("http://www.microsoft.com/index.html?t=9" == url3);
}

struct TestURLGetWithoutUserDetailsData
{
  std::string input;
  std::string expected;
  bool redact;
};

std::ostream& operator<<(std::ostream& os,
                       const TestURLGetWithoutUserDetailsData& rhs)
{
  return os << "(Input: " << rhs.input <<
    "; Redact: " << (rhs.redact?"true":"false") <<
    "; Expected: " << rhs.expected << ")";
}

class TestURLGetWithoutUserDetails : public Test,
                                     public WithParamInterface<TestURLGetWithoutUserDetailsData>
{
};

TEST_P(TestURLGetWithoutUserDetails, GetWithoutUserDetails)
{
  CURL input(GetParam().input);
  std::string result = input.GetWithoutUserDetails(GetParam().redact);
  EXPECT_EQ(result, GetParam().expected);
}

const TestURLGetWithoutUserDetailsData values[] = {
  { std::string("smb://example.com/example"), std::string("smb://example.com/example"), false },
  { std::string("smb://example.com/example"), std::string("smb://example.com/example"), true },
  { std::string("smb://god:universe@example.com/example"), std::string("smb://example.com/example"), false },
  { std::string("smb://god@example.com/example"), std::string("smb://USERNAME@example.com/example"), true },
  { std::string("smb://god:universe@example.com/example"), std::string("smb://USERNAME:PASSWORD@example.com/example"), true },
  { std::string("http://god:universe@example.com:8448/example|auth=digest"), std::string("http://USERNAME:PASSWORD@example.com:8448/example|auth=digest"), true },
  { std::string("smb://fd00::1/example"), std::string("smb://fd00::1/example"), false },
  { std::string("smb://fd00::1/example"), std::string("smb://fd00::1/example"), true },
  { std::string("smb://[fd00::1]:8080/example"), std::string("smb://[fd00::1]:8080/example"), false },
  { std::string("smb://[fd00::1]:8080/example"), std::string("smb://[fd00::1]:8080/example"), true },
  { std::string("smb://god:universe@[fd00::1]:8080/example"), std::string("smb://[fd00::1]:8080/example"), false },
  { std::string("smb://god@[fd00::1]:8080/example"), std::string("smb://USERNAME@[fd00::1]:8080/example"), true },
  { std::string("smb://god:universe@fd00::1/example"), std::string("smb://USERNAME:PASSWORD@fd00::1/example"), true },
  { std::string("http://god:universe@[fd00::1]:8448/example|auth=digest"), std::string("http://USERNAME:PASSWORD@[fd00::1]:8448/example|auth=digest"), true },
  { std::string("smb://00ff:1:0000:abde::/example"), std::string("smb://00ff:1:0000:abde::/example"), true },
  { std::string("smb://god:universe@[00ff:1:0000:abde::]:8080/example"), std::string("smb://[00ff:1:0000:abde::]:8080/example"), false },
  { std::string("smb://god@[00ff:1:0000:abde::]:8080/example"), std::string("smb://USERNAME@[00ff:1:0000:abde::]:8080/example"), true },
  { std::string("smb://god:universe@00ff:1:0000:abde::/example"), std::string("smb://USERNAME:PASSWORD@00ff:1:0000:abde::/example"), true },
  { std::string("http://god:universe@[00ff:1:0000:abde::]:8448/example|auth=digest"), std::string("http://USERNAME:PASSWORD@[00ff:1:0000:abde::]:8448/example|auth=digest"), true },
  { std::string("smb://milkyway;god:universe@example.com/example"), std::string("smb://DOMAIN;USERNAME:PASSWORD@example.com/example"), true },
  { std::string("smb://milkyway;god@example.com/example"), std::string("smb://DOMAIN;USERNAME@example.com/example"), true },
  { std::string("smb://milkyway;@example.com/example"), std::string("smb://example.com/example"), true },
  { std::string("smb://milkyway;god:universe@example.com/example"), std::string("smb://example.com/example"), false },
  { std::string("smb://milkyway;god@example.com/example"), std::string("smb://example.com/example"), false },
  { std::string("smb://milkyway;@example.com/example"), std::string("smb://example.com/example"), false },
};

INSTANTIATE_TEST_SUITE_P(URL, TestURLGetWithoutUserDetails, ValuesIn(values));

TEST(TestURLGetWithoutOptions, PreserveSlashesBetweenProtocolAndPath)
{
  std::string url{"https://example.com//stream//example/index.m3u8"};
  CURL input{url};
  EXPECT_EQ(input.GetWithoutOptions(), url);
}

TEST(TestURL, TestWithoutFilenameEncodingDecoding)
{
  std::string decoded_with_path{
      "smb://dom^inName;u$ername:pa$$word@example.com/stream/example/index.m3u8"};

  CURL input{decoded_with_path};
  EXPECT_EQ("smb://dom%5einName;u%24ername:pa%24%24word@example.com/", input.GetWithoutFilename());
  EXPECT_EQ("dom^inName", input.GetDomain());
  EXPECT_EQ("u$ername", input.GetUserName());
  EXPECT_EQ("pa$$word", input.GetPassWord());

  CURL decoded(input.GetWithoutFilename());
  EXPECT_EQ("smb://dom%5einName;u%24ername:pa%24%24word@example.com/", decoded.Get());
  EXPECT_EQ("dom^inName", decoded.GetDomain());
  EXPECT_EQ("u$ername", decoded.GetUserName());
  EXPECT_EQ("pa$$word", decoded.GetPassWord());
}

TEST_F(TestCURL, TestPaths)
{
  // Linux path
  std::string path{"/somepath/path/movie.avi"};
  CURL url(path);
  EXPECT_EQ(url.GetFileName(), path);
  EXPECT_EQ(url.GetShareName(), "");
  EXPECT_EQ(url.GetHostName(), "");
  EXPECT_EQ(url.GetProtocol(), "");
  EXPECT_EQ(url.GetFileType(), "avi");
  EXPECT_EQ(url.Get(), path);

  path = "/somepath/path/";
  url.SetFileName(path);
  EXPECT_EQ(url.Get(), path);

  // DOS path
  path = "D:\\Movies\\movie.avi";
  url = CURL(path);
  EXPECT_EQ(url.GetFileName(), path);
  EXPECT_EQ(url.GetShareName(), "D:");
  EXPECT_EQ(url.GetHostName(), "");
  EXPECT_EQ(url.GetProtocol(), "");
  EXPECT_EQ(url.GetFileType(), "avi");
  EXPECT_EQ(url.Get(), path);

  path = "D:\\Movies\\";
  url.SetFileName(path);
  EXPECT_EQ(url.Get(), path);

  // Windows server path
  path = "\\\\Server\\Movies\\movie.avi";
  url = CURL(path);
  EXPECT_EQ(url.GetFileName(), path);
  EXPECT_EQ(url.GetShareName(), "");
  EXPECT_EQ(url.GetHostName(), "");
  EXPECT_EQ(url.GetProtocol(), "");
  EXPECT_EQ(url.GetFileType(), "avi");
  EXPECT_EQ(url.Get(), path);

  path = "\\\\Server\\Movies\\";
  url.SetFileName(path);
  EXPECT_EQ(url.Get(), path);

  // URL path with smb://
  path = "smb://somepath/Movies/movie.mkv";
  url = CURL(path);
  EXPECT_EQ(url.GetFileName(), "Movies/movie.mkv");
  EXPECT_EQ(url.GetShareName(), "Movies");
  EXPECT_EQ(url.GetHostName(), "somepath");
  EXPECT_EQ(url.GetProtocol(), "smb");
  EXPECT_EQ(url.GetFileType(), "mkv");
  EXPECT_EQ(url.Get(), path);

  url.SetFileName("Films/film.mp4");
  path = "smb://somepath/Films/film.mp4";
  EXPECT_EQ(url.GetFileName(), "Films/film.mp4");
  EXPECT_EQ(url.GetShareName(), "Films");
  EXPECT_EQ(url.GetHostName(), "somepath");
  EXPECT_EQ(url.GetProtocol(), "smb");
  EXPECT_EQ(url.GetFileType(), "mp4");
  EXPECT_EQ(url.Get(), path);

  // bluray:// path
  path =
      "bluray://"
      "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fMovies%252fAlien%2520(1979)%252fDisc%25201%252f"
      "ALIEN.ISO%2f/BDMV/PLAYLIST/"
      "00800.mpls";
  url = CURL(path);
  EXPECT_EQ(url.GetFileName(), "BDMV/PLAYLIST/00800.mpls");
  EXPECT_EQ(url.GetShareName(), "BDMV");
  EXPECT_EQ(url.GetHostName(),
            "udf://smb%3a%2f%2fsomepath%2fMovies%2fAlien%20(1979)%2fDisc%201%2fALIEN.ISO/");
  EXPECT_EQ(url.GetProtocol(), "bluray");
  EXPECT_EQ(url.GetFileType(), "mpls");
  EXPECT_EQ(url.Get(), path);

  // Contains an embedded udf:// path
  std::string path2{url.GetHostName()};
  CURL url2(path2);
  EXPECT_EQ(url2.GetFileName(), "");
  EXPECT_EQ(url2.GetShareName(), "");
  EXPECT_EQ(url2.GetHostName(), "smb://somepath/Movies/Alien (1979)/Disc 1/ALIEN.ISO");
  EXPECT_EQ(url2.GetProtocol(), "udf");
  EXPECT_EQ(url2.GetFileType(), "");
  EXPECT_EQ(url2.Get(), path2);

  // Contains an embedded smb:// path
  std::string path3{url2.GetHostName()};
  CURL url3(path3);
  EXPECT_EQ(url3.GetFileName(), "Movies/Alien (1979)/Disc 1/ALIEN.ISO");
  EXPECT_EQ(url3.GetShareName(), "Movies");
  EXPECT_EQ(url3.GetHostName(), "somepath");
  EXPECT_EQ(url3.GetProtocol(), "smb");
  EXPECT_EQ(url3.GetFileType(), "iso");
  EXPECT_EQ(url3.Get(), path3);

  // Alter the smb path to point to Disc 2
  url3.SetFileName("Movies/Alien (1979)/Disc 2/ALIEN_DISC2.NRG");
  path3 = "smb://somepath/Movies/Alien (1979)/Disc 2/ALIEN_DISC2.NRG";
  EXPECT_EQ(url3.GetFileName(), "Movies/Alien (1979)/Disc 2/ALIEN_DISC2.NRG");
  EXPECT_EQ(url3.GetShareName(), "Movies");
  EXPECT_EQ(url3.GetHostName(), "somepath");
  EXPECT_EQ(url3.GetProtocol(), "smb");
  EXPECT_EQ(url3.GetFileType(), "nrg");
  EXPECT_EQ(url3.Get(), path3);

  // Update the udf:// path to use the new smb path
  url2.SetHostName(url3.Get());
  path2 = "udf://smb%3a%2f%2fsomepath%2fMovies%2fAlien%20(1979)%2fDisc%202%2fALIEN_DISC2.NRG/";
  EXPECT_EQ(url2.GetFileName(), "");
  EXPECT_EQ(url2.GetShareName(), "");
  EXPECT_EQ(url2.GetHostName(), "smb://somepath/Movies/Alien (1979)/Disc 2/ALIEN_DISC2.NRG");
  EXPECT_EQ(url2.GetProtocol(), "udf");
  EXPECT_EQ(url2.GetFileType(), "");
  EXPECT_EQ(url2.Get(), path2);

  // Update the bluray:// path to use the new udf path
  url.SetHostName(url2.Get());
  path = "bluray://"
         "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fMovies%252fAlien%20(1979)%252fDisc%202%252f"
         "ALIEN_DISC2.NRG%2f/BDMV/PLAYLIST/"
         "00800.mpls";
  url = CURL(path);
  EXPECT_EQ(url.GetFileName(), "BDMV/PLAYLIST/00800.mpls");
  EXPECT_EQ(url.GetShareName(), "BDMV");
  EXPECT_EQ(url.GetHostName(),
            "udf://smb%3a%2f%2fsomepath%2fMovies%2fAlien (1979)%2fDisc 2%2fALIEN_DISC2.NRG/");
  EXPECT_EQ(url.GetProtocol(), "bluray");
  EXPECT_EQ(url.GetFileType(), "mpls");
  EXPECT_EQ(url.Get(), path);
}

TEST_F(TestCURL, TestExtensions)
{
  CURL url("/somepath/path/movie.zip");
  EXPECT_EQ(".zip", url.GetExtension());
  std::string extensions{".zip|.rar|.tar.gz"};
  EXPECT_TRUE(url.HasExtension(extensions));

  url = CURL("/somepath/path/movie.tar.gz");
  EXPECT_EQ(".tar.gz", url.GetExtension());
  EXPECT_TRUE(url.HasExtension(extensions));

  url = CURL("/somepath/path/movie.gz");
  EXPECT_EQ(".gz", url.GetExtension());
  EXPECT_FALSE(url.HasExtension(extensions));

  url = CURL("/somepath/path/movie.name.zip");
  EXPECT_EQ(".zip", url.GetExtension());
  extensions = ".zip|.rar|.tar.gz";
  EXPECT_TRUE(url.HasExtension(extensions));

  url = CURL("/somepath/path/movie.name.tar.gz");
  EXPECT_EQ(".tar.gz", url.GetExtension());
  EXPECT_TRUE(url.HasExtension(extensions));

  url = CURL("/somepath/path/movie.name.gz");
  EXPECT_EQ(".gz", url.GetExtension());
  EXPECT_FALSE(url.HasExtension(extensions));
}
