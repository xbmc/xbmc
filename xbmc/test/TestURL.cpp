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

// Tests for NFS URL options parsing (PR #28249)
TEST_F(TestCURL, TestNFSURLWithOptions)
{
  // Test NFS URL with mimetype option
  std::string nfsUrl = "nfs://192.168.178.23/mnt/HD/Videos/movie.mkv?mimetype=video%2fx-matroska";
  CURL url(nfsUrl);

  EXPECT_EQ(url.GetProtocol(), "nfs");
  EXPECT_EQ(url.GetHostName(), "192.168.178.23");
  EXPECT_EQ(url.GetFileName(), "mnt/HD/Videos/movie.mkv");
  EXPECT_EQ(url.GetFileType(), "mkv");
  EXPECT_EQ(url.GetOptions(), "?mimetype=video%2fx-matroska");
  // Verify options are properly separated from filename
  EXPECT_FALSE(url.GetFileName().find("?") != std::string::npos);
}

TEST_F(TestCURL, TestNFSURLWithoutOptions)
{
  // Test NFS URL without options - should work normally
  std::string nfsUrl = "nfs://192.168.178.23/mnt/HD/Videos/movie.mkv";
  CURL url(nfsUrl);

  EXPECT_EQ(url.GetProtocol(), "nfs");
  EXPECT_EQ(url.GetHostName(), "192.168.178.23");
  EXPECT_EQ(url.GetFileName(), "mnt/HD/Videos/movie.mkv");
  EXPECT_EQ(url.GetFileType(), "mkv");
  EXPECT_EQ(url.GetOptions(), "");
}

TEST_F(TestCURL, TestNFSURLWithPort)
{
  // Test NFS URL with port number
  std::string nfsUrl = "nfs://192.168.178.23:2049/mnt/HD/Videos/movie.mkv";
  CURL url(nfsUrl);

  EXPECT_EQ(url.GetProtocol(), "nfs");
  EXPECT_EQ(url.GetHostName(), "192.168.178.23");
  EXPECT_EQ(url.GetPort(), 2049);
  EXPECT_EQ(url.GetFileName(), "mnt/HD/Videos/movie.mkv");
}

TEST_F(TestCURL, TestNFSURLWithPortAndOptions)
{
  // Test NFS URL with both port and options
  std::string nfsUrl = "nfs://192.168.178.23:2049/mnt/HD/Videos/movie.mp4?mimetype=video%2fmp4";
  CURL url(nfsUrl);

  EXPECT_EQ(url.GetProtocol(), "nfs");
  EXPECT_EQ(url.GetHostName(), "192.168.178.23");
  EXPECT_EQ(url.GetPort(), 2049);
  EXPECT_EQ(url.GetFileName(), "mnt/HD/Videos/movie.mp4");
  EXPECT_EQ(url.GetOptions(), "?mimetype=video%2fmp4");
  EXPECT_FALSE(url.GetFileName().find("?") != std::string::npos);
}

TEST_F(TestCURL, TestNFSURLMultipleOptions)
{
  // Test NFS URL with multiple options
  std::string nfsUrl = "nfs://192.168.178.23/share/file.mp4?mimetype=video%2fmp4&timeout=30";
  CURL url(nfsUrl);

  EXPECT_EQ(url.GetProtocol(), "nfs");
  EXPECT_EQ(url.GetHostName(), "192.168.178.23");
  EXPECT_EQ(url.GetFileName(), "share/file.mp4");
  EXPECT_EQ(url.GetOptions(), "?mimetype=video%2fmp4&timeout=30");
  // Filename should not contain options
  EXPECT_FALSE(url.GetFileName().find("?") != std::string::npos);
  EXPECT_FALSE(url.GetFileName().find("&") != std::string::npos);
}

TEST_F(TestCURL, TestNFSURLWithSpecialCharacters)
{
  // Test NFS URL with spaces and special characters in path
  std::string nfsUrl = "nfs://192.168.178.23/mnt/Public/Shared%20Videos/"
                       "Movie%20(2020).mkv?mimetype=video%2fx-matroska";
  CURL url(nfsUrl);

  EXPECT_EQ(url.GetProtocol(), "nfs");
  EXPECT_EQ(url.GetHostName(), "192.168.178.23");
  // Filename should contain the path with encoded spaces but NOT the query
  EXPECT_EQ(url.GetFileName(), "mnt/Public/Shared%20Videos/Movie%20(2020).mkv");
  EXPECT_EQ(url.GetOptions(), "?mimetype=video%2fx-matroska");
  EXPECT_EQ(url.GetFileType(), "mkv");
}

TEST_F(TestCURL, TestNFSURLReconstructed)
{
  // Test that reconstructing NFS URL preserves options
  std::string originalUrl = "nfs://192.168.178.23/mnt/HD/Videos/movie.mkv?mimetype=video%2fmp4";
  CURL url(originalUrl);
  std::string reconstructedUrl = url.Get();

  EXPECT_EQ(reconstructedUrl, originalUrl);
}

TEST_F(TestCURL, TestNFSURLWithIPv6)
{
  // Test NFS URL with IPv6 address
  std::string nfsUrl = "nfs://[2001:db8::1]/mnt/HD/Videos/movie.mp4?mimetype=video%2fmp4";
  CURL url(nfsUrl);

  EXPECT_EQ(url.GetProtocol(), "nfs");
  EXPECT_EQ(url.GetHostName(), "2001:db8::1");
  EXPECT_EQ(url.GetFileName(), "mnt/HD/Videos/movie.mp4");
  EXPECT_EQ(url.GetOptions(), "?mimetype=video%2fmp4");
}

TEST_F(TestCURL, TestNFSURLGetWithoutUserDetails)
{
  // Test GetWithoutUserDetails for NFS URL with options
  std::string nfsUrl = "nfs://user:pass@192.168.178.23/share/movie.mp4?mimetype=video%2fmp4";
  CURL url(nfsUrl);

  std::string result = url.GetWithoutUserDetails(false);
  EXPECT_EQ(result, "nfs://192.168.178.23/share/movie.mp4?mimetype=video%2fmp4");
}

TEST_F(TestCURL, TestNFSURLGetWithoutOptions)
{
  // Test GetWithoutOptions for NFS URL
  std::string nfsUrl = "nfs://192.168.178.23/share/movie.mp4?mimetype=video%2fmp4";
  CURL url(nfsUrl);

  std::string resultWithoutOptions = url.GetWithoutOptions();
  EXPECT_EQ(resultWithoutOptions, "nfs://192.168.178.23/share/movie.mp4");
}

struct TestNFSURLParsingData
{
  std::string input;
  std::string expectedProtocol;
  std::string expectedHostName;
  std::string expectedFileName;
  std::string expectedOptions;
  std::string expectedFileType;
};

std::ostream& operator<<(std::ostream& os, const TestNFSURLParsingData& rhs)
{
  return os << "(Input: " << rhs.input << ")";
}

class TestNFSURLParsing : public Test, public WithParamInterface<TestNFSURLParsingData>
{
};

TEST_P(TestNFSURLParsing, ParseNFSURL)
{
  CURL url(GetParam().input);
  EXPECT_EQ(url.GetProtocol(), GetParam().expectedProtocol);
  EXPECT_EQ(url.GetHostName(), GetParam().expectedHostName);
  EXPECT_EQ(url.GetFileName(), GetParam().expectedFileName);
  EXPECT_EQ(url.GetOptions(), GetParam().expectedOptions);
  EXPECT_EQ(url.GetFileType(), GetParam().expectedFileType);
}

const TestNFSURLParsingData nfsParsingData[] = {
    // Basic NFS URL with options
    {"nfs://192.168.178.23/share/file.mkv?mimetype=video%2fx-matroska", "nfs", "192.168.178.23",
     "share/file.mkv", "?mimetype=video%2fx-matroska", "mkv"},
    // NFS URL without options
    {"nfs://192.168.1.1/export/path/video.mp4", "nfs", "192.168.1.1", "export/path/video.mp4", "",
     "mp4"},
    // NFS URL with port and options
    {"nfs://10.0.0.5:2049/data/movie.avi?timeout=30", "nfs", "10.0.0.5", "data/movie.avi",
     "?timeout=30", "avi"},
    // NFS URL with multiple query parameters
    {"nfs://server.local/nfs/files.iso?mimetype=application%2fx-iso9660-image&version=1", "nfs",
     "server.local", "nfs/files.iso", "?mimetype=application%2fx-iso9660-image&version=1", "iso"},
    // NFS URL with encoded spaces in path
    {"nfs://192.168.1.100/Public/My%20Videos/Movie%202020.mkv?type=video", "nfs", "192.168.1.100",
     "Public/My%20Videos/Movie%202020.mkv", "?type=video", "mkv"},
    // '#' in directory name - must remain part of the file name
    {"nfs://192.168.178.23/share/#recently-added/movie.mkv", "nfs", "192.168.178.23",
     "share/#recently-added/movie.mkv", "", "mkv"},
    // ';' in directory name - must remain part of the file name
    {"nfs://192.168.178.23/share/Comedy;Drama/movie.mkv", "nfs", "192.168.178.23",
     "share/Comedy;Drama/movie.mkv", "", "mkv"},
};

INSTANTIATE_TEST_SUITE_P(NFSParsing, TestNFSURLParsing, ValuesIn(nfsParsingData));
