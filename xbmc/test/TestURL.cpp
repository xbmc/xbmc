/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/StackDirectory.h"
#include "utils/URIUtils.h"

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

TEST_F(TestCURL, FoundationCompatibility)
{
  struct Example
  {
    const char* input;
    const char* serialized;
    const char* protocol;
    const char* filename;
  };
  const Example examples[] = {
      {"HTTP://example.test//a b.mkv?x=1|seekable=0", "http://example.test//a b.mkv?x=1|seekable=0",
       "http", "/a b.mkv"},
      {"smb://domain;user:pass@server/share/movie.mkv",
       "smb://domain;user:pass@server/share/movie.mkv", "smb", "share/movie.mkv"},
      {"ftp://server/a.mkv?auth=TLS|cache=no", "ftp://server/a.mkv?auth=TLS|cache=no", "ftp",
       "a.mkv"},
      {"nfs://server/export/a.mkv?version=4", "nfs://server/export/a.mkv?version=4", "nfs",
       "export/a.mkv"},
      {"rtp://source@239.1.2.3:1234", "rtp://source@239.1.2.3:1234/", "rtp", ""},
      {"udp://source@239.1.2.3:1234", "udp://source@239.1.2.3:1234/", "udp", ""},
      {"dav://server/a.mkv", "dav://server/a.mkv", "dav", "a.mkv"},
      {"special://home/a.mkv", "special://home/a.mkv", "special", "home/a.mkv"},
      {"videodb://movies/titles/1/", "videodb://movies/titles/1/", "videodb", "movies/titles/1/"},
      {"zip://%2ftmp%2fmovies.zip/dir/a.mkv", "zip://%2ftmp%2fmovies.zip/dir/a.mkv", "zip",
       "dir/a.mkv"},
      {"image://http%3a%2f%2fserver%2fa.jpg/", "image://http%3a%2f%2fserver%2fa.jpg/", "image", ""},
      {"C:relative", "C:relative", "", "C:relative"},
      {"./notes:2026", "./notes:2026", "", "./notes:2026"},
      {"relative?name#part+100%.mkv", "relative?name#part+100%.mkv", "",
       "relative?name#part+100%.mkv"},
      {"\\\\server\\share\\a.mkv", "\\\\server\\share\\a.mkv", "", "\\\\server\\share\\a.mkv"},
  };
  for (const auto& example : examples)
  {
    SCOPED_TRACE(example.input);
    const CURL url(example.input);
    EXPECT_EQ(example.serialized, url.Get());
    EXPECT_EQ(example.protocol, url.GetProtocol());
    EXPECT_EQ(example.filename, url.GetFileName());
  }
  EXPECT_TRUE(CURL::IsFullPath("C:relative"));
  EXPECT_FALSE(CURL::IsFullPath("./notes:2026"));
}

TEST_F(TestCURL, FoundationLegacyOptionsAndReparse)
{
  CURL url("http://user:pass@server/a.mkv?z=1&a=one+two&a=three&empty=|seekable=0");
  EXPECT_EQ("three", url.GetOption("a"));
  EXPECT_EQ("http://user:pass@server/a.mkv?z=1&a=one+two&a=three&empty=|seekable=0", url.Get());
  url.SetOption("b", "hello world");
  EXPECT_EQ("?a=three&b=hello%20world&empty&z=1", url.GetOptions());
  url.Parse("relative.txt");
  EXPECT_TRUE(url.GetHostName().empty());
  EXPECT_TRUE(url.GetUserName().empty());
  EXPECT_TRUE(url.GetOptions().empty());
  EXPECT_TRUE(url.GetProtocolOptions().empty());
  EXPECT_EQ("txt", url.GetFileType());
  url.Reset();
  EXPECT_TRUE(url.Get().empty());
  EXPECT_TRUE(url.GetShareName().empty());
  EXPECT_TRUE(url.GetFileType().empty());
}

TEST_F(TestCURL, IdentifierRecognitionBeforeNormalization)
{
  for (const auto* input :
       {"DATA:text/plain,http://example.test/a//b?x=1#fragment", "data:,bad%", "data:,a|b",
        "urn:example:a/b?+resolver?=query#part", "mailto:user@example.test", "example+v1:body"})
  {
    SCOPED_TRACE(input);
    const CURL url(input);
    EXPECT_EQ(input, url.Get());
    EXPECT_FALSE(url.GetProtocol().empty());
    EXPECT_TRUE(url.GetFileName().empty());
  }
}

TEST_F(TestCURL, LosslessIdentifiers)
{
  const char* examples[] = {
      "data:,hello",
      "DATA:text/plain,http://example.test/a//b?x=1#fragment",
      "data:text/plain,../movie.mkv",
      "data:text/plain,a%2Fb+%23c",
      "data://x",
      "data:,bad%",
      "data:,a|b",
      "urn:example:a/b?+resolver?=query#part",
      "mailto:user@example.test",
      "example+v1:body",
      "notes:2026",
      "magnet:?xt=urn:btih:0123456789012345678901234567890123456789",
      "magnet:?xt=urn:btih:0123456789012345678901234567890123456789&tr=http%3a%2f%2fa"
      "&tr=http%3A%2F%2Fb&xt=urn:btmh:"
      "12200123456789012345678901234567890123456789012345678901234567890123"
      "&empty=&bare&&unknown=one+two#part",
  };
  for (const auto* example : examples)
  {
    SCOPED_TRACE(example);
    const CURL url(example);
    EXPECT_EQ(example, url.Get());
    EXPECT_FALSE(url.GetProtocol().empty());
    EXPECT_TRUE(url.IsLosslessURI());
    EXPECT_FALSE(url.SupportsPathOperations());
    EXPECT_TRUE(url.GetFileName().empty());
    EXPECT_TRUE(url.GetFileNameWithoutPath().empty());
    EXPECT_TRUE(url.GetFileType().empty());
    EXPECT_TRUE(url.GetShareName().empty());
    EXPECT_TRUE(url.GetExtension().empty());
    EXPECT_FALSE(url.HasExtension(".mkv"));
    EXPECT_TRUE(url.GetWithoutFilename().empty());
    EXPECT_TRUE(CURL::IsFullPath(example));
  }
}

TEST_F(TestCURL, RawComponentsAndSchemeSpecificPart)
{
  // RFC 2397 section 3: a data consumer needs the query-shaped payload suffix too.
  const CURL data("DATA:text/plain,http://example.test/a//b?x=1#fragment");
  EXPECT_EQ("data", data.GetProtocol());
  EXPECT_EQ("DATA", data.GetRawScheme());
  EXPECT_EQ("text/plain,http://example.test/a//b", data.GetRawPath());
  EXPECT_EQ("text/plain,http://example.test/a//b?x=1", data.GetSchemeSpecificPart());
  EXPECT_EQ("x=1", data.GetRawQuery());
  EXPECT_EQ("fragment", data.GetRawFragment());
  EXPECT_FALSE(data.HasAuthority());
  EXPECT_TRUE(data.HasQuery());
  EXPECT_TRUE(data.HasFragment());
  for (const auto* input : {"example:", "example:?", "example:#", "example:?#", "example://"})
  {
    const CURL url(input);
    EXPECT_EQ(input, url.Get());
    EXPECT_TRUE(url.GetRawQuery().empty());
    EXPECT_TRUE(url.GetRawFragment().empty());
    EXPECT_EQ(std::string(input).find('?') != std::string::npos, url.HasQuery());
    EXPECT_EQ(std::string(input).find('#') != std::string::npos, url.HasFragment());
    EXPECT_EQ(std::string(input) == "example://", url.HasAuthority());
  }
  const CURL urn("urn:example:a/b?+resolver?=query#part");
  EXPECT_EQ("+resolver?=query", urn.GetRawQuery());
  EXPECT_EQ("part", urn.GetRawFragment());
}

TEST_F(TestCURL, ExplicitAndMixedInputIntent)
{
  for (const auto* input : {"C:relative", "C:/Media/a", "C:\\Media\\a", "\\\\server\\share\\a",
                            "/tmp/a://b", "./notes:2026", "relative?name#part+100%.mkv"})
  {
    SCOPED_TRACE(input);
    const CURL url(input);
    EXPECT_TRUE(url.GetProtocol().empty());
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_FALSE(url.HasQuery());
    EXPECT_FALSE(url.HasFragment());
  }
  CURL explicitURI;
  ASSERT_TRUE(explicitURI.ParseURI("C:relative"));
  EXPECT_EQ("c", explicitURI.GetProtocol());
  EXPECT_EQ("C:relative", explicitURI.Get());
  explicitURI.ParseNativePath("notes:2026");
  EXPECT_TRUE(explicitURI.GetProtocol().empty());
  EXPECT_EQ("notes:2026", explicitURI.GetFileName());
  EXPECT_FALSE(explicitURI.ParseURI("data:,bad%"));
  EXPECT_EQ("data", explicitURI.GetProtocol());
  EXPECT_EQ("data:,bad%", explicitURI.Get());
  EXPECT_FALSE(explicitURI.ParseURI("relative"));
  EXPECT_TRUE(explicitURI.Get().empty());
  ASSERT_TRUE(explicitURI.ParseURI("file:/tmp/a"));
  EXPECT_FALSE(explicitURI.HasAuthority());
  EXPECT_EQ("/tmp/a", explicitURI.GetRawPath());
  ASSERT_TRUE(explicitURI.ParseURI("file:///tmp/a"));
  EXPECT_TRUE(explicitURI.HasAuthority());
  EXPECT_EQ("file:///tmp/a", explicitURI.Get());
}

TEST_F(TestCURL, LosslessMemberMutation)
{
  // Repeated tr fields: https://www.bittorrent.org/beps/bep_0009.html#magnet-uri-format
  // Absolute member paths are a Kodi extension; navigation preserves query identity.
  const std::string query = "xt=urn:btih:0123456789012345678901234567890123456789"
                            "&tr=http%3a%2f%2fa&tr=http%3A%2F%2Fb&empty=&bare&&x=a+b";
  CURL member("magnet:/dir/movie.mkv?" + query + "#part");
  EXPECT_TRUE(member.SupportsPathOperations());
  EXPECT_TRUE(member.PreservesQueryIdentity());
  EXPECT_EQ("/dir/movie.mkv", member.GetFileName());
  EXPECT_EQ("mkv", member.GetFileType());
  std::map<std::string, std::string> options;
  member.GetOptions(options);
  EXPECT_TRUE(options.empty());
  EXPECT_FALSE(member.HasOption("tr"));
  EXPECT_EQ("magnet:/dir/movie.mkv?" + query + "#part", member.Get());
  member.SetOption("tr", "replacement");
  member.RemoveOption("xt");
  member.SetOptions("?lost=yes");
  member.SetProtocolOptions("lost=yes");
  member.SetProtocolOption("lost", "yes");
  EXPECT_EQ("magnet:/dir/movie.mkv?" + query + "#part", member.Get());
  ASSERT_TRUE(member.SetURIPath("/other/movie.mkv"));
  EXPECT_EQ("magnet:/other/movie.mkv?" + query + "#part", member.Get());
  EXPECT_FALSE(member.SetURIPath("/other?lost=yes"));
  EXPECT_FALSE(member.SetURIPath("//authority"));
  EXPECT_EQ("magnet:/other/movie.mkv?" + query + "#part", member.Get());
  ASSERT_TRUE(member.SetURIPath(""));
  EXPECT_EQ("magnet:?" + query + "#part", member.Get());
  EXPECT_FALSE(member.SupportsPathOperations());
  ASSERT_TRUE(member.SetURIPath("/"));
  EXPECT_EQ("magnet:/?" + query + "#part", member.Get());
  ASSERT_TRUE(member.SetRawQuery(std::string{}));
  EXPECT_EQ("magnet:/?#part", member.Get());
  ASSERT_TRUE(member.SetRawQuery(std::nullopt));
  EXPECT_EQ("magnet:/#part", member.Get());
  EXPECT_FALSE(member.SetRawQuery("x=#fragment"));
  CURL data("data:,movie.mkv");
  EXPECT_FALSE(data.SetURIPath("/movie.mkv"));
  data.SetFileName("/movie.mkv");
  EXPECT_EQ("data:,movie.mkv", data.Get());
}

TEST_F(TestCURL, URIPathMutationUsesAuthorityContext)
{
  struct Mutation
  {
    const char* input;
    const char* path;
    const char* expected;
  };
  const Mutation cases[] = {
      {"https://host/a?x=1#part", "/b", "https://host/b?x=1#part"},
      {"https://host/a?x=1#part", "//b", "https://host//b?x=1#part"},
      {"https://host/a?x=1#part", "///b", "https://host///b?x=1#part"},
      {"https://host/a?x=1#part", "", "https://host?x=1#part"},
      {"file:///a?x=1#part", "//b", "file:////b?x=1#part"},
      {"file:///a?x=1#part", "///b", "file://///b?x=1#part"},
      {"file:///a?x=1#part", "", "file://?x=1#part"},
      {"file:/a?x=1#part", "/b", "file:/b?x=1#part"},
      {"file:/a?x=1#part", "b", "file:b?x=1#part"},
      {"magnet:/a?xt=urn:btih:0123456789012345678901234567890123456789#part", "/b",
       "magnet:/b?xt=urn:btih:0123456789012345678901234567890123456789#part"},
      {"HtTpS://UsEr:p%61ss@HoSt:8443/a?x=a+b&x=%2f&x=%2F#P%61rt", "//%2F%2f/%3f%23/../b.mkv",
       "HtTpS://UsEr:p%61ss@HoSt:8443//%2F%2f/%3f%23/../b.mkv?x=a+b&x=%2f&x=%2F#P%61rt"},
      {"file:///a?#", "//b", "file:////b?#"},
      {"https://host/a", "//b", "https://host//b"},
  };
  for (const auto& test : cases)
  {
    SCOPED_TRACE(test.input);
    SCOPED_TRACE(test.path);
    CURL url;
    ASSERT_TRUE(url.ParseURI(test.input));
    const CURL original = url;
    EXPECT_TRUE(url.SetURIPath(test.path));
    EXPECT_EQ(test.expected, url.Get());
    EXPECT_EQ(test.path, url.GetRawPath());
    EXPECT_EQ(test.path, url.GetFileName());
    EXPECT_TRUE(url.IsLosslessURI());
    EXPECT_EQ(original.GetRawScheme(), url.GetRawScheme());
    EXPECT_EQ(original.HasAuthority(), url.HasAuthority());
    EXPECT_EQ(original.GetRawAuthority(), url.GetRawAuthority());
    EXPECT_EQ(original.HasQuery(), url.HasQuery());
    EXPECT_EQ(original.GetRawQuery(), url.GetRawQuery());
    EXPECT_EQ(original.HasFragment(), url.HasFragment());
    EXPECT_EQ(original.GetRawFragment(), url.GetRawFragment());
  }
}

TEST_F(TestCURL, URIPathMutationRejectsComponentChanges)
{
  for (const auto* input :
       {"https://host/a.mkv?x=1#part", "file:///a.mkv?#", "file:/a.mkv?x=1#part",
        "magnet:/a.mkv?xt=urn:btih:0123456789012345678901234567890123456789#part"})
  {
    SCOPED_TRACE(input);
    CURL url;
    ASSERT_TRUE(url.ParseURI(input));
    const CURL original = url;
    for (const auto* path : {"/b?", "/b#", "/bad%", "/bad%2", "/bad%GG", "/bad path", "/bad\\path",
                             "/bad[path", "//b", "///b", "b"})
    {
      if (original.HasAuthority() && std::string_view(path).starts_with("//"))
        continue;
      if (!original.HasAuthority() && !original.IsProtocol("magnet") &&
          path == std::string_view("b"))
        continue;
      SCOPED_TRACE(path);
      EXPECT_FALSE(url.SetURIPath(path));
      EXPECT_EQ(input, url.Get());
      EXPECT_TRUE(url.IsLosslessURI());
      EXPECT_EQ(original.GetProtocol(), url.GetProtocol());
      EXPECT_EQ(original.GetRawScheme(), url.GetRawScheme());
      EXPECT_EQ(original.GetRawPath(), url.GetRawPath());
      EXPECT_EQ(original.HasAuthority(), url.HasAuthority());
      EXPECT_EQ(original.GetRawAuthority(), url.GetRawAuthority());
      EXPECT_EQ(original.HasQuery(), url.HasQuery());
      EXPECT_EQ(original.GetRawQuery(), url.GetRawQuery());
      EXPECT_EQ(original.HasFragment(), url.HasFragment());
      EXPECT_EQ(original.GetRawFragment(), url.GetRawFragment());
      EXPECT_EQ(original.GetFileName(), url.GetFileName());
      EXPECT_EQ(original.GetFileType(), url.GetFileType());
      EXPECT_EQ(original.GetShareName(), url.GetShareName());
      EXPECT_EQ(original.GetOptions(), url.GetOptions());
      EXPECT_EQ(original.SupportsPathOperations(), url.SupportsPathOperations());
    }
  }
}

TEST_F(TestCURL, FileNameSetterUsesURIPathAuthorityContext)
{
  CURL url;
  ASSERT_TRUE(url.ParseURI("https://host/a?x=1#part"));
  url.SetFileName("//b.mkv");
  EXPECT_EQ("https://host//b.mkv?x=1#part", url.Get());
  EXPECT_EQ("//b.mkv", url.GetRawPath());
  EXPECT_EQ("//b.mkv", url.GetFileName());
  EXPECT_EQ("mkv", url.GetFileType());
  EXPECT_TRUE(url.HasAuthority());
  EXPECT_EQ("host", url.GetRawAuthority());
  EXPECT_EQ("x=1", url.GetRawQuery());
  EXPECT_EQ("part", url.GetRawFragment());
  url.SetFileName("//b?lost");
  EXPECT_EQ("https://host//b.mkv?x=1#part", url.Get());
  EXPECT_EQ("//b.mkv", url.GetRawPath());
  EXPECT_EQ("mkv", url.GetFileType());
}

TEST_F(TestCURL, LosslessStateTransitions)
{
  CURL url("magnet:/dir/movie.mkv?xt=urn:btih:0123456789012345678901234567890123456789#part");
  CURL copy(url);
  CURL moved(std::move(copy));
  url.Parse("data:,hello?x=1#secret");
  EXPECT_EQ("/dir/movie.mkv", moved.GetRawPath());
  EXPECT_EQ("part", moved.GetRawFragment());
  EXPECT_EQ("data:,hello?x=1#secret", url.Get());
  url.SetHostName("host");
  url.SetUserName("user");
  url.SetPassword("pass");
  url.SetDomain("domain");
  url.SetPort(1234);
  EXPECT_EQ("data:,hello?x=1#secret", url.Get());
  EXPECT_TRUE(url.GetHostName().empty());
  EXPECT_TRUE(url.GetUserName().empty());
  EXPECT_FALSE(url.HasPort());
  url.SetProtocol("urn");
  EXPECT_EQ("urn:,hello?x=1#secret", url.Get());
  EXPECT_EQ("urn:,hello?x=1#secret", url.GetRedacted());
  url.SetProtocol("");
  EXPECT_EQ(",hello", url.Get());
  EXPECT_FALSE(url.HasQuery());
  EXPECT_FALSE(url.HasFragment());
  url.Parse("http://user:pass@server/a.mkv?x=1#part");
  EXPECT_EQ("server", url.GetRawAuthority().substr(url.GetRawAuthority().find('@') + 1));
  EXPECT_EQ("/a.mkv", url.GetRawPath());
  EXPECT_EQ("x=1", url.GetRawQuery());
  EXPECT_EQ("part", url.GetRawFragment());
  url.SetProtocol("data");
  EXPECT_TRUE(url.IsLosslessURI());
  EXPECT_TRUE(url.GetFileName().empty());
  EXPECT_TRUE(url.GetUserName().empty());
  EXPECT_TRUE(url.GetOptions().empty() || url.GetOptions() == "?x=1");
  url.Parse("relative.txt");
  EXPECT_FALSE(url.HasAuthority());
  EXPECT_FALSE(url.HasQuery());
  EXPECT_FALSE(url.HasFragment());
  EXPECT_EQ("txt", url.GetFileType());
  url.Reset();
  EXPECT_TRUE(url.GetRawPath().empty());
  EXPECT_TRUE(url.GetRawScheme().empty());
  EXPECT_TRUE(url.GetShareName().empty());
}

TEST_F(TestCURL, BoundedDataRedaction)
{
  for (const auto* input : {"data:,hello", "DATA://x?private#secret", "data:,bad%#secret",
                            "data:,a|password=secret#suffix"})
  {
    const CURL url(input);
    EXPECT_EQ("data:[REDACTED]", url.GetRedacted());
    EXPECT_EQ("data:[REDACTED]", CURL::GetRedacted(input));
    EXPECT_EQ(input, url.GetWithoutUserDetails(false));
    EXPECT_EQ("data:[REDACTED]", url.GetWithoutUserDetails(true));
  }
  const std::string large = "data:," + std::string(1024 * 1024, 'a') + "#secret";
  EXPECT_EQ("data:[REDACTED]", CURL::GetRedacted(large));
  EXPECT_EQ("urn:example:abc", CURL("urn:example:abc").GetRedacted());
  EXPECT_EQ("custom://USERNAME:PASSWORD@host/path",
            CURL("custom://user:pass@host/path").GetRedacted());
  EXPECT_EQ("custom://host/path", CURL("custom://user:pass@host/path").GetWithoutUserDetails());
}

TEST_F(TestCURL, ExplicitSchemeSetterTransitions)
{
  CURL url;
  ASSERT_TRUE(url.ParseURI("data:,hello?x=1#part"));
  url.SetProtocol("a");
  EXPECT_EQ("a", url.GetProtocol());
  EXPECT_EQ("a:,hello?x=1#part", url.Get());
  EXPECT_TRUE(url.IsLosslessURI());
  url.SetProtocol("not_a_scheme");
  EXPECT_EQ("a:,hello?x=1#part", url.Get());
  CURL legacy("http://server/a.mkv");
  for (const auto* invalid : {"not_a_scheme", "foo:bar", "1scheme"})
  {
    legacy.SetProtocol(invalid);
    EXPECT_EQ("http://server/a.mkv", legacy.Get());
  }
}

TEST_F(TestCURL, DriveRelativeFilename)
{
  EXPECT_EQ("relative.mkv", CURL("C:relative.mkv").GetFileNameWithoutPath());
}

TEST_F(TestCURL, MovedFromURIHasNoStaleComponents)
{
  CURL source("magnet:/dir/movie.mkv?xt=urn:btih:0123456789012345678901234567890123456789#part");
  CURL moved(std::move(source));
  EXPECT_FALSE(source.HasQuery());
  EXPECT_FALSE(source.HasFragment());
  EXPECT_TRUE(source.GetSchemeSpecificPart().empty());
  EXPECT_FALSE(source.SetRawQuery("x=1"));
  source.Parse("data:,hello");
  moved = std::move(source);
  EXPECT_EQ("data:,hello", moved.Get());
  EXPECT_TRUE(source.Get().empty());
  EXPECT_FALSE(source.IsLosslessURI());
}

TEST_F(TestCURL, PathSettersDoNotRepairMalformedCandidates)
{
  for (const auto* input :
       {"magnet:/bad%?xt=urn:btih:0123456789012345678901234567890123456789", "http:bad%"})
  {
    CURL url(input);
    EXPECT_FALSE(url.SupportsPathOperations());
    EXPECT_FALSE(url.SetURIPath("/fixed"));
    url.SetFileName("/fixed");
    EXPECT_EQ(input, url.Get());
    EXPECT_TRUE(url.GetFileName().empty());
  }
}

TEST_F(TestCURL, URIPathComponentsDoNotHaveNativeDrives)
{
  for (const auto* input : {"http://host/C:dir/movie.mkv", "http:C:dir/movie.mkv"})
  {
    const CURL url(input);
    EXPECT_EQ('/', url.GetDirectorySeparator());
    EXPECT_EQ("C:dir", url.GetShareName());
    EXPECT_EQ("mkv", url.GetFileType());
  }
  EXPECT_TRUE(CURL("http://host/C:dir.with.dot/movie").GetFileType().empty());
  for (const auto* input :
       {"C:dir\\movie.mkv", "file://C:dir\\movie.mkv", "stack://C:dir\\movie.mkv"})
  {
    EXPECT_EQ('\\', CURL(input).GetDirectorySeparator());
  }
  CURL explicitFile;
  ASSERT_TRUE(explicitFile.ParseURI("file:C:dir/movie.mkv"));
  EXPECT_EQ('/', explicitFile.GetDirectorySeparator());
}

TEST_F(TestCURL, LegacyProtocolToNativeClearsURIState)
{
  CURL url("http://user:pass@host:8080/dir/movie.mkv?x=1#part|cache=no");
  url.SetProtocol("");
  EXPECT_EQ("dir/movie.mkv", url.Get());
  EXPECT_TRUE(url.GetHostName().empty());
  EXPECT_TRUE(url.GetUserName().empty());
  EXPECT_TRUE(url.GetPassWord().empty());
  EXPECT_TRUE(url.GetOptions().empty());
  EXPECT_TRUE(url.GetProtocolOptions().empty());
  EXPECT_FALSE(url.HasOption("x"));
  EXPECT_FALSE(url.HasPort());
  EXPECT_FALSE(url.HasAuthority());
  EXPECT_FALSE(url.HasQuery());
  EXPECT_FALSE(url.HasFragment());
  url.SetProtocol("https");
  EXPECT_EQ("https://dir/movie.mkv", url.Get());
}

TEST_F(TestCURL, SacdVFSCompatibilityProjection)
{
  // CVFSURLWrapper forwards these getters; the add-on interface is down in this fixture.
  const CURL url("sacd://%2fmedia%2fdisc.iso/1.dsf");
  EXPECT_EQ("sacd", url.GetProtocol());
  EXPECT_EQ("%2fmedia%2fdisc.iso", url.GetHostName());
  EXPECT_EQ("1.dsf", url.GetFileName());
  EXPECT_FALSE(url.HasEncodedHostname());
  EXPECT_FALSE(url.IsLosslessURI());
  EXPECT_TRUE(url.SupportsPathOperations());
  EXPECT_EQ("sacd://%2fmedia%2fdisc.iso/1.dsf", url.Get());
  EXPECT_EQ("sacd://%2fmedia%2fdisc.iso/", url.GetWithoutFilename());
  EXPECT_EQ("sacd://%2fmedia%2fdisc.iso/", URIUtils::GetParentPath(url.Get()));

  CURL escaped("SACD://%2Fmedia%2Fdisc%20%2B%2525.iso/2.dsf");
  EXPECT_EQ("sacd", escaped.GetProtocol());
  EXPECT_EQ("%2Fmedia%2Fdisc%20%2B%2525.iso", escaped.GetHostName());
  EXPECT_EQ("/media/disc +%25.iso", CURL::Decode(escaped.GetHostName()));
  EXPECT_EQ("2.dsf", escaped.GetFileName());
  escaped.SetFileName("3.dsf");
  EXPECT_EQ("sacd://%2Fmedia%2Fdisc%20%2B%2525.iso/3.dsf", escaped.Get());
}

TEST_F(TestCURL, ExistingVFSCompatibilityProjections)
{
  struct VFSCase
  {
    const char* input;
    const char* protocol;
    const char* hostname;
    const char* filename;
    bool encodedHostname;
  };
  const VFSCase cases[] = {
      {"sftp://server/dir/file", "sftp", "server", "dir/file", false},
      {"ssh://server/dir/file", "ssh", "server", "dir/file", false},
      {"smb://server/share/file", "smb", "server", "share/file", false},
      {"rar://%2fmedia%2fdisc.rar/dir/file", "rar", "/media/disc.rar", "dir/file", true},
      {"archive://%2fmedia%2fdisc.7z/dir/file", "archive", "/media/disc.7z", "dir/file", true},
  };
  for (const auto& test : cases)
  {
    SCOPED_TRACE(test.input);
    const CURL url(test.input);
    EXPECT_EQ(test.protocol, url.GetProtocol());
    EXPECT_EQ(test.hostname, url.GetHostName());
    EXPECT_EQ(test.filename, url.GetFileName());
    EXPECT_EQ(test.encodedHostname, url.HasEncodedHostname());
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.SupportsPathOperations());
    EXPECT_EQ(test.input, url.Get());
  }
}

TEST_F(TestCURL, RedactDataThroughResourceWrappers)
{
  const char* locations[] = {
      "image://data%3a%2cSECRET/",
      "image://video@data%3a%2cSECRET/",
      "zip://data%3a%2cSECRET/member",
      "image://zip%3a%2f%2fdata%253a%252cSECRET%2fmember/",
      "zip://image%3a%2f%2fdata%253a%252cSECRET%2f/member",
      "stack:///plain/file , image://data%3a%2cSECRET/",
      "image://data%3a%2f%2fSECRET%3fx%3dSECRET%23SECRET/",
      "zip://data%3a%2cbad%25%7cSECRET%3fx%3dSECRET%23SECRET/member",
  };
  for (const auto* location : locations)
  {
    SCOPED_TRACE(location);
    const CURL url(location);
    const std::string redacted = url.GetRedacted();
    EXPECT_EQ(std::string::npos, redacted.find("SECRET"));
    EXPECT_LT(redacted.size(), 128u);
    EXPECT_EQ(redacted, CURL::GetRedacted(location));
    EXPECT_EQ(redacted, url.GetWithoutUserDetails(true));
    EXPECT_EQ(location, url.Get());
    const auto withoutUserDetails = std::string_view(location).starts_with("image://video@")
                                        ? "image://data%3a%2cSECRET/"
                                        : location;
    EXPECT_EQ(withoutUserDetails, url.GetWithoutUserDetails(false));
  }
}

TEST_F(TestCURL, WrappedDataRedactionIsBounded)
{
  const std::string data = "data:," + std::string(1024 * 1024, 'S') + "?SECRET#SECRET";
  const std::string image = "image://" + CURL::Encode(data) + "/";
  const std::string nested = "zip://" + CURL::Encode(image) + "/member";
  for (const auto& location : {data, image, nested})
  {
    const CURL url(location);
    const auto redacted = url.GetRedacted();
    EXPECT_LT(redacted.size(), 128u);
    EXPECT_EQ(std::string::npos, redacted.find("SECRET"));
    EXPECT_EQ(redacted, CURL::GetRedacted(location));
    EXPECT_EQ(location, url.GetWithoutUserDetails(false));
  }
}

TEST_F(TestCURL, RedactDataThroughNestedStackEscapes)
{
  const std::string image = "image://" + CURL::Encode("data:,SECRET") + "/";
  std::string innerStack;
  std::string outerStack;
  ASSERT_TRUE(XFILE::CStackDirectory::ConstructStackPath({"/plain/one", image}, innerStack));
  ASSERT_TRUE(XFILE::CStackDirectory::ConstructStackPath({innerStack, "/plain/two"}, outerStack));
  for (const auto& location : {outerStack, "image://" + CURL::Encode(outerStack) + "/",
                               "zip://" + CURL::Encode(outerStack) + "/member"})
  {
    const CURL url(location);
    EXPECT_EQ(std::string::npos, url.GetRedacted().find("SECRET"));
    EXPECT_EQ(url.GetRedacted(), CURL::GetRedacted(location));
    EXPECT_EQ(location, url.GetWithoutUserDetails(false));
  }
}

TEST_F(TestCURL, RedactDataThroughLegacyWrapperSeparators)
{
  const std::string inner = "image:\\\\data%3a%2cSECRET/";
  const std::string outer = "image://" + CURL::Encode(inner) + "/";
  EXPECT_EQ("image:[REDACTED]", CURL(inner).GetRedacted());
  EXPECT_EQ("image:[REDACTED]", CURL::GetRedacted(inner));
  EXPECT_EQ("image:[REDACTED]", CURL(outer).GetRedacted());
  EXPECT_EQ("image:[REDACTED]", CURL::GetRedacted(outer));
  EXPECT_EQ(outer, CURL(outer).GetWithoutUserDetails(false));
}

TEST_F(TestCURL, RedactionRespectsStackMemberBoundaries)
{
  std::string innerStack;
  std::string outerStack;
  ASSERT_TRUE(XFILE::CStackDirectory::ConstructStackPath({"/a", "/b"}, innerStack));
  ASSERT_TRUE(XFILE::CStackDirectory::ConstructStackPath({innerStack, "/plain , data:,SECRET"},
                                                         outerStack));
  for (const auto& location : {outerStack, "image://" + CURL::Encode(outerStack) + "/"})
  {
    EXPECT_EQ(location, CURL(location).GetRedacted());
    EXPECT_EQ(location, CURL::GetRedacted(location));
  }
}

TEST_F(TestCURL, ResourceRedactionPreservesOtherLegacyBehavior)
{
  const char* unchanged[] = {
      "musicsearch://data%3a%2cSECRET/",
      "episodes://data%3a%2cSECRET/",
      "http://data%3a%2cSECRET/path",
      "http://host/path?next=data:,SECRET",
  };
  for (const auto* location : unchanged)
  {
    SCOPED_TRACE(location);
    EXPECT_EQ(location, CURL(location).GetRedacted());
    EXPECT_EQ(location, CURL::GetRedacted(location));
  }
  const CURL archive("zip://http%3a%2f%2fuser%3apass%40host%2fa.zip/member");
  EXPECT_EQ("zip://http%3a%2f%2fhost%2fa.zip/member", archive.GetRedacted());
  EXPECT_EQ("zip://http%3a%2f%2fhost%2fa.zip/member", archive.GetWithoutUserDetails(false));
  EXPECT_EQ("stack://http://USERNAME:PASSWORD@host/a , /plain/file",
            CURL::GetRedacted("stack://http://user:pass@host/a , /plain/file"));
}

TEST_F(TestCURL, CustomVFSProtocolsRetainLegacyProjection)
{
  // VFSURL getters must work before an add-on is loaded as well as while it is active.
  CURL url("Custom+VFS://user:p%61ss@host:123/share/movie.mkv");
  EXPECT_FALSE(url.IsLosslessURI());
  EXPECT_TRUE(url.SupportsPathOperations());
  EXPECT_EQ("custom+vfs", url.GetProtocol());
  EXPECT_EQ("host", url.GetHostName());
  EXPECT_EQ("user", url.GetUserName());
  EXPECT_EQ("pass", url.GetPassWord());
  EXPECT_EQ(123, url.GetPort());
  EXPECT_EQ("share/movie.mkv", url.GetFileName());
  EXPECT_EQ("share", url.GetShareName());
  EXPECT_EQ("mkv", url.GetFileType());
  url.SetFileName("share/next.mkv");
  url.SetHostName("other");
  url.SetOption("key", "value");
  url.SetProtocolOption("cache", "no");
  EXPECT_EQ("value", url.GetOption("key"));
  EXPECT_EQ("no", url.GetProtocolOption("cache"));
  EXPECT_EQ("custom+vfs://user:pass@other:123/share/next.mkv?key=value|cache=no", url.Get());
  url.SetProtocol("another+vfs");
  EXPECT_EQ("another+vfs://user:pass@other:123/share/next.mkv?key=value|cache=no", url.Get());

  CURL constructed;
  constructed.SetProtocol("custom+vfs");
  constructed.SetHostName("host");
  constructed.SetFileName("dir/file");
  EXPECT_EQ("custom+vfs://host/dir/file", constructed.Get());
}

TEST_F(TestCURL, ExplicitUnknownURIRetainsLosslessIntent)
{
  CURL uri;
  ASSERT_TRUE(uri.ParseURI("CuStOm://user:p%61ss@host/path?x=1&x=2#"));
  EXPECT_TRUE(uri.IsLosslessURI());
  EXPECT_FALSE(uri.SupportsPathOperations());
  EXPECT_EQ("CuStOm://user:p%61ss@host/path?x=1&x=2#", uri.Get());
  EXPECT_EQ("user:p%61ss@host", uri.GetRawAuthority());
  EXPECT_EQ("x=1&x=2", uri.GetRawQuery());
  uri.SetProtocol("custom+vfs");
  EXPECT_FALSE(uri.IsLosslessURI());
  EXPECT_EQ("host", uri.GetHostName());
  EXPECT_EQ("path?x=1&x=2#", uri.GetFileName());
}

TEST_F(TestCURL, PlaylistProtocolsRetainLegacySemantics)
{
  for (const std::string scheme : {"playlistmusic", "playlistvideo"})
  {
    CURL url(scheme + "://");
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.SupportsPathOperations());
    url.SetFileName("entry");
    EXPECT_EQ(scheme + "://entry", url.Get());
  }
}

TEST_F(TestCURL, RedactCredentialsThroughImageWrappers)
{
  const std::string image = "image://http%3a%2f%2fuser%3aSECRET%40host%2fimage.png/";
  for (const std::string& location :
       {image, "image://" + CURL::Encode(image) + "/", "zip://" + CURL::Encode(image) + "/member"})
  {
    SCOPED_TRACE(location);
    EXPECT_EQ(std::string::npos, CURL::GetRedacted(location).find("SECRET"));
    EXPECT_EQ(std::string::npos, CURL(location).GetRedacted().find("SECRET"));
    CURL explicitURI;
    ASSERT_TRUE(explicitURI.ParseURI(location));
    EXPECT_EQ(std::string::npos, explicitURI.GetRedacted().find("SECRET"));
  }
  EXPECT_EQ("image://http%3a%2f%2fhost%2fimage.png/", CURL::GetRedacted(image));
}

TEST_F(TestCURL, ExplicitWrappersRemoveNestedCredentials)
{
  const std::string resource = "http://user:SECRET@host/resource";
  const std::string cleanResource = "http://host/resource";
  for (const std::string scheme : {"image", "zip"})
  {
    std::string location = scheme + "://" + CURL::Encode(resource) + "/";
    std::string expected = scheme + "://" + CURL::Encode(cleanResource) + "/";
    for (int depth = 0; depth < 3; ++depth)
    {
      SCOPED_TRACE(location);
      CURL uri;
      ASSERT_TRUE(uri.ParseURI(location));
      EXPECT_EQ(expected, uri.GetWithoutUserDetails());
      EXPECT_EQ(expected, uri.GetWithoutUserDetails(false));
      EXPECT_EQ(expected, uri.GetWithoutUserDetails(true));
      EXPECT_EQ(expected, uri.GetRedacted());
      EXPECT_EQ(expected, CURL::GetRedacted(location));
      EXPECT_EQ(location, uri.Get());
      location = "zip://" + CURL::Encode(location) + "/member";
      expected = "zip://" + CURL::Encode(expected) + "/member";
    }
  }
}

TEST_F(TestCURL, ExplicitCredentialFreeWrappersPreserveSpelling)
{
  for (const std::string resource : {"http://host/im%61ge.png?x=1&x=2", "HTTP://host/image.png"})
  {
    for (const std::string scheme : {"image", "zip"})
    {
      const std::string location = scheme + "://" + CURL::Encode(resource) + "/";
      for (const std::string& wrapper : {location, "zip://" + CURL::Encode(location) + "/member",
                                         "IMAGE://" + CURL::Encode(location) + "/"})
      {
        SCOPED_TRACE(wrapper);
        CURL uri;
        ASSERT_TRUE(uri.ParseURI(wrapper));
        EXPECT_EQ(wrapper, uri.GetWithoutUserDetails(false));
        EXPECT_EQ(CURL(wrapper).GetWithoutUserDetails(true), uri.GetWithoutUserDetails(true));
        EXPECT_EQ(CURL::GetRedacted(wrapper), uri.GetRedacted());
        EXPECT_EQ(wrapper, uri.Get());
      }
    }
  }
}

TEST_F(TestCURL, OneLetterSchemeTransitionsRemainExplicitURIs)
{
  CURL uri;
  ASSERT_TRUE(uri.ParseURI("custom://host/path?x=1#part"));
  uri.SetProtocol("a");
  EXPECT_TRUE(uri.IsLosslessURI());
  EXPECT_EQ("a://host/path?x=1#part", uri.Get());
  EXPECT_EQ("host", uri.GetRawAuthority());
  EXPECT_EQ("x=1", uri.GetRawQuery());
  EXPECT_EQ("part", uri.GetRawFragment());
}

TEST_F(TestCURL, LargeDataInEncodedStackRedactsWithoutChangingResource)
{
  const std::string data = "DATA:;base64," + std::string(2 * 1024 * 1024, 'S') + "?SECRET#SECRET";
  std::string stack;
  ASSERT_TRUE(XFILE::CStackDirectory::ConstructStackPath({"/plain/file", data}, stack));
  const std::string wrapper = "image://" + CURL::Encode(stack) + "/";
  EXPECT_EQ("stack:[REDACTED]", CURL::GetRedacted(stack));
  EXPECT_EQ("image:[REDACTED]", CURL::GetRedacted(wrapper));
  CURL explicitURI;
  ASSERT_TRUE(explicitURI.ParseURI(wrapper));
  EXPECT_EQ("image:[REDACTED]", explicitURI.GetRedacted());
  EXPECT_EQ(wrapper, explicitURI.Get());
}
