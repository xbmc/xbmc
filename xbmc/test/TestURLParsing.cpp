/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/StackDirectory.h"
#include "utils/URIUtils.h"

#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include <gtest/gtest.h>

namespace
{

struct ParsedURL
{
  const char* input;
  const char* protocol;
  const char* hostName;
  int port;
  const char* shareName;
  const char* fileName;
  const char* fileType;
  const char* options;
};

// clang-format off
const ParsedURL ParsedURLs[] = {
    // input, protocol, host, port, share, file name, file type, options
    {"/somepath/path/movie.avi", "", "", 0, "", "/somepath/path/movie.avi", "avi", ""},
    {"D:\\Movies\\movie.avi", "", "", 0, "D:", "D:\\Movies\\movie.avi", "avi", ""},
    {"\\\\Server\\Movies\\movie.avi", "", "", 0, "", "\\\\Server\\Movies\\movie.avi", "avi", ""},
    {"smb://somepath/Movies/movie.mkv", "smb", "somepath", 0, "Movies", "Movies/movie.mkv", "mkv", ""},
    {"nfs://192.168.178.23/mnt/HD/Videos/movie.mkv", "nfs", "192.168.178.23", 0, "mnt", "mnt/HD/Videos/movie.mkv", "mkv", ""},
    {"nfs://192.168.178.23/mnt/HD/Videos/movie.mkv?mimetype=video%2fx-matroska", "nfs", "192.168.178.23", 0, "mnt", "mnt/HD/Videos/movie.mkv", "mkv", "?mimetype=video%2fx-matroska"},
    {"nfs://192.168.178.23/mnt/HD/Videos/movie.mkv?mimetype=video%2fmp4", "nfs", "192.168.178.23", 0, "mnt", "mnt/HD/Videos/movie.mkv", "mkv", "?mimetype=video%2fmp4"},
    {"nfs://192.168.178.23:2049/mnt/HD/Videos/movie.mkv", "nfs", "192.168.178.23", 2049, "mnt", "mnt/HD/Videos/movie.mkv", "mkv", ""},
    {"nfs://192.168.178.23:2049/mnt/HD/Videos/movie.mp4?mimetype=video%2fmp4", "nfs", "192.168.178.23", 2049, "mnt", "mnt/HD/Videos/movie.mp4", "mp4", "?mimetype=video%2fmp4"},
    {"nfs://192.168.178.23/share/file.mp4?mimetype=video%2fmp4&timeout=30", "nfs", "192.168.178.23", 0, "share", "share/file.mp4", "mp4", "?mimetype=video%2fmp4&timeout=30"},
    {"nfs://192.168.178.23/mnt/Public/Shared%20Videos/Movie%20(2020).mkv?mimetype=video%2fx-matroska", "nfs", "192.168.178.23", 0, "mnt", "mnt/Public/Shared%20Videos/Movie%20(2020).mkv", "mkv", "?mimetype=video%2fx-matroska"},
    {"nfs://192.168.178.23/share/file.mkv?mimetype=video%2fx-matroska", "nfs", "192.168.178.23", 0, "share", "share/file.mkv", "mkv", "?mimetype=video%2fx-matroska"},
    {"nfs://192.168.1.1/export/path/video.mp4", "nfs", "192.168.1.1", 0, "export", "export/path/video.mp4", "mp4", ""},
    {"nfs://10.0.0.5:2049/data/movie.avi?timeout=30", "nfs", "10.0.0.5", 2049, "data", "data/movie.avi", "avi", "?timeout=30"},
    {"nfs://server.local/nfs/files.iso?mimetype=application%2fx-iso9660-image&version=1", "nfs", "server.local", 0, "nfs", "nfs/files.iso", "iso", "?mimetype=application%2fx-iso9660-image&version=1"},
    {"nfs://192.168.1.100/Public/My%20Videos/Movie%202020.mkv?type=video", "nfs", "192.168.1.100", 0, "Public", "Public/My%20Videos/Movie%202020.mkv", "mkv", "?type=video"},
    // '#' and ';' in a directory name remain part of the file name
    {"nfs://192.168.178.23/share/#recently-added/movie.mkv", "nfs", "192.168.178.23", 0, "share", "share/#recently-added/movie.mkv", "mkv", ""},
    {"nfs://192.168.178.23/share/Comedy;Drama/movie.mkv", "nfs", "192.168.178.23", 0, "share", "share/Comedy;Drama/movie.mkv", "mkv", ""},
};
// clang-format on

struct SetFileNameCase
{
  const char* input;
  const char* fileName;
  const char* expected;
};

const SetFileNameCase SetFileNameCases[] = {
    {"/somepath/path/movie.avi", "/somepath/path/", "/somepath/path/"},
    {"D:\\Movies\\movie.avi", "D:\\Movies\\", "D:\\Movies\\"},
    {"\\\\Server\\Movies\\movie.avi", "\\\\Server\\Movies\\", "\\\\Server\\Movies\\"},
};

struct TestURLGetWithoutUserDetailsData
{
  std::string input;
  std::string expected;
  bool redact;
};

std::ostream& operator<<(std::ostream& os, const TestURLGetWithoutUserDetailsData& rhs)
{
  return os << "(Input: " << rhs.input << "; Redact: " << (rhs.redact ? "true" : "false")
            << "; Expected: " << rhs.expected << ")";
}

class TestURLGetWithoutUserDetails
  : public testing::Test,
    public testing::WithParamInterface<TestURLGetWithoutUserDetailsData>
{
};

// clang-format off
const TestURLGetWithoutUserDetailsData values[] = {
    {"smb://example.com/example", "smb://example.com/example", false},
    {"smb://example.com/example", "smb://example.com/example", true},
    {"smb://god:universe@example.com/example", "smb://example.com/example", false},
    {"smb://god@example.com/example", "smb://USERNAME@example.com/example", true},
    {"smb://god:universe@example.com/example", "smb://USERNAME:PASSWORD@example.com/example", true},
    {"http://god:universe@example.com:8448/example|auth=digest", "http://USERNAME:PASSWORD@example.com:8448/example|auth=digest", true},
    {"smb://fd00::1/example", "smb://[fd00::1]/example", false},
    {"smb://fd00::1/example", "smb://[fd00::1]/example", true},
    {"smb://[fd00::1]:8080/example", "smb://[fd00::1]:8080/example", false},
    {"smb://[fd00::1]:8080/example", "smb://[fd00::1]:8080/example", true},
    {"smb://god:universe@[fd00::1]:8080/example", "smb://[fd00::1]:8080/example", false},
    {"smb://god@[fd00::1]:8080/example", "smb://USERNAME@[fd00::1]:8080/example", true},
    {"smb://god:universe@fd00::1/example", "smb://USERNAME:PASSWORD@[fd00::1]/example", true},
    {"http://god:universe@[fd00::1]:8448/example|auth=digest", "http://USERNAME:PASSWORD@[fd00::1]:8448/example|auth=digest", true},
    {"smb://00ff:1:0000:abde::/example", "smb://[00ff:1:0000:abde::]/example", true},
    {"smb://god:universe@[00ff:1:0000:abde::]:8080/example", "smb://[00ff:1:0000:abde::]:8080/example", false},
    {"smb://god@[00ff:1:0000:abde::]:8080/example", "smb://USERNAME@[00ff:1:0000:abde::]:8080/example", true},
    {"smb://god:universe@00ff:1:0000:abde::/example", "smb://USERNAME:PASSWORD@[00ff:1:0000:abde::]/example", true},
    {"http://god:universe@[00ff:1:0000:abde::]:8448/example|auth=digest", "http://USERNAME:PASSWORD@[00ff:1:0000:abde::]:8448/example|auth=digest", true},
    {"smb://milkyway;god:universe@example.com/example", "smb://DOMAIN;USERNAME:PASSWORD@example.com/example", true},
    {"smb://milkyway;god@example.com/example", "smb://DOMAIN;USERNAME@example.com/example", true},
    {"smb://milkyway;@example.com/example", "smb://example.com/example", true},
    {"smb://milkyway;god:universe@example.com/example", "smb://example.com/example", false},
    {"smb://milkyway;god@example.com/example", "smb://example.com/example", false},
    {"smb://milkyway;@example.com/example", "smb://example.com/example", false},
    {"nfs://user:pass@192.168.178.23/share/movie.mp4?mimetype=video%2fmp4", "nfs://192.168.178.23/share/movie.mp4?mimetype=video%2fmp4", false},
};
// clang-format on

struct GetWithoutOptionsCase
{
  const char* input;
  const char* expected;
};

const GetWithoutOptionsCase GetWithoutOptionsCases[] = {
    // Slashes between the protocol and the path are kept
    {"https://example.com//stream//example/index.m3u8",
     "https://example.com//stream//example/index.m3u8"},
    {"nfs://192.168.178.23/share/movie.mp4?mimetype=video%2fmp4",
     "nfs://192.168.178.23/share/movie.mp4"},
};

struct IPv6HostCase
{
  const char* input;
  const char* withoutOptions;
  const char* withoutFilename;
};

// An IPv6 host keeps its brackets wherever the URL is rebuilt, with or without a port.
const IPv6HostCase IPv6HostCases[] = {
    {"nfs://[2001:db8::1]/mnt/HD/Videos/movie.mp4?mimetype=video%2fmp4",
     "nfs://[2001:db8::1]/mnt/HD/Videos/movie.mp4", "nfs://[2001:db8::1]/"},
    {"nfs://[2001:db8::1]:2049/mnt/HD/Videos/movie.mp4?mimetype=video%2fmp4",
     "nfs://[2001:db8::1]:2049/mnt/HD/Videos/movie.mp4", "nfs://[2001:db8::1]:2049/"},
    {"smb://[fd00::1]/share/movie.mkv", "smb://[fd00::1]/share/movie.mkv", "smb://[fd00::1]/"},
    {"smb://[fd00::1]:445/share/movie.mkv", "smb://[fd00::1]:445/share/movie.mkv",
     "smb://[fd00::1]:445/"},
};

struct ExtensionCase
{
  const char* path;
  const char* extension;
  bool isArchive;
};

const ExtensionCase ExtensionCases[] = {
    {"/somepath/path/movie.zip", ".zip", true},
    {"/somepath/path/movie.tar.gz", ".tar.gz", true},
    {"/somepath/path/movie.gz", ".gz", false},
    {"/somepath/path/movie.name.zip", ".zip", true},
    {"/somepath/path/movie.name.tar.gz", ".tar.gz", true},
    {"/somepath/path/movie.name.gz", ".gz", false},
};

} // namespace

TEST(TestURLParsing, Components)
{
  for (const auto& [input, protocol, hostName, port, shareName, fileName, fileType, options] :
       ParsedURLs)
  {
    SCOPED_TRACE(input);
    const CURL url{input};
    EXPECT_EQ(protocol, url.GetProtocol());
    EXPECT_EQ(hostName, url.GetHostName());
    EXPECT_EQ(port, url.GetPort());
    EXPECT_EQ(shareName, url.GetShareName());
    EXPECT_EQ(fileName, url.GetFileName());
    EXPECT_EQ(fileType, url.GetFileType());
    EXPECT_EQ(options, url.GetOptions());
    EXPECT_EQ(input, url.Get());
  }
}

TEST(TestURLParsing, IPv6Host)
{
  const CURL url{"nfs://[2001:db8::1]/mnt/HD/Videos/movie.mp4?mimetype=video%2fmp4"};
  EXPECT_EQ("nfs", url.GetProtocol());
  EXPECT_EQ("2001:db8::1", url.GetHostName());
  EXPECT_EQ("mnt/HD/Videos/movie.mp4", url.GetFileName());
  EXPECT_EQ("?mimetype=video%2fmp4", url.GetOptions());
}

TEST(TestURLParsing, IPv6HostKeepsItsBrackets)
{
  for (const auto& [input, withoutOptions, withoutFilename] : IPv6HostCases)
  {
    SCOPED_TRACE(input);
    const CURL url{input};
    EXPECT_EQ(input, url.Get());
    EXPECT_EQ(withoutOptions, url.GetWithoutOptions());
    EXPECT_EQ(withoutFilename, url.GetWithoutFilename());
    EXPECT_EQ(input, url.GetWithoutUserDetails(false));
    EXPECT_EQ(input, url.GetRedacted());
  }
}

TEST(TestURLParsing, SetFileName)
{
  for (const auto& [input, fileName, expected] : SetFileNameCases)
  {
    CURL url{input};
    url.SetFileName(fileName);
    EXPECT_EQ(expected, url.Get()) << input;
  }
}

TEST(TestURLParsing, Comparison)
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

TEST_P(TestURLGetWithoutUserDetails, GetWithoutUserDetails)
{
  CURL input(GetParam().input);
  std::string result = input.GetWithoutUserDetails(GetParam().redact);
  EXPECT_EQ(result, GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(URL, TestURLGetWithoutUserDetails, testing::ValuesIn(values));

TEST(TestURLParsing, GetWithoutOptions)
{
  for (const auto& [input, expected] : GetWithoutOptionsCases)
  {
    EXPECT_EQ(expected, CURL(input).GetWithoutOptions()) << input;
  }
}

TEST(TestURLParsing, WithoutFilenameEncodingDecoding)
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

TEST(TestURLParsing, Extensions)
{
  for (const auto& [path, extension, isArchive] : ExtensionCases)
  {
    const CURL url{path};
    EXPECT_EQ(extension, url.GetExtension()) << path;
    EXPECT_EQ(isArchive, url.HasExtension(".zip|.rar|.tar.gz")) << path;
  }
}

TEST(TestURLParsing, NestedDiscPaths)
{
  // smb:// path, and its file name replaced
  std::string path{"smb://somepath/Movies/movie.mkv"};
  CURL url(path);
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

namespace
{

struct SerializedURL
{
  const char* input;
  const char* serialized;
  const char* protocol;
  const char* fileName;
};

// clang-format off
const SerializedURL SerializedURLs[] = {
    {"HTTP://example.test//a b.mkv?x=1|seekable=0", "http://example.test//a b.mkv?x=1|seekable=0", "http", "/a b.mkv"},
    {"smb://domain;user:pass@server/share/movie.mkv", "smb://domain;user:pass@server/share/movie.mkv", "smb", "share/movie.mkv"},
    {"ftp://server/a.mkv?auth=TLS|cache=no", "ftp://server/a.mkv?auth=TLS|cache=no", "ftp", "a.mkv"},
    {"nfs://server/export/a.mkv?version=4", "nfs://server/export/a.mkv?version=4", "nfs", "export/a.mkv"},
    {"rtp://source@239.1.2.3:1234", "rtp://source@239.1.2.3:1234/", "rtp", ""},
    {"udp://source@239.1.2.3:1234", "udp://source@239.1.2.3:1234/", "udp", ""},
    {"dav://server/a.mkv", "dav://server/a.mkv", "dav", "a.mkv"},
    {"special://home/a.mkv", "special://home/a.mkv", "special", "home/a.mkv"},
    {"videodb://movies/titles/1/", "videodb://movies/titles/1/", "videodb", "movies/titles/1/"},
    {"zip://%2ftmp%2fmovies.zip/dir/a.mkv", "zip://%2ftmp%2fmovies.zip/dir/a.mkv", "zip", "dir/a.mkv"},
    {"image://http%3a%2f%2fserver%2fa.jpg/", "image://http%3a%2f%2fserver%2fa.jpg/", "image", ""},
    {"C:relative", "C:relative", "", "C:relative"},
    {"./notes:2026", "./notes:2026", "", "./notes:2026"},
    {"relative?name#part+100%.mkv", "relative?name#part+100%.mkv", "", "relative?name#part+100%.mkv"},
    {"\\\\server\\share\\a.mkv", "\\\\server\\share\\a.mkv", "", "\\\\server\\share\\a.mkv"},
};
// clang-format on

struct FullPathCase
{
  const char* path;
  bool expected;
};

const FullPathCase FullPathCases[] = {
    {"C:relative", true},
    {"./notes:2026", false},
};

// Identifiers keep their exact spelling and have no file name or hierarchy.
const char* const LosslessIdentifiers[] = {
    "data:,hello",
    "DATA:text/plain,http://example.test/a//b?x=1#fragment",
    "data:text/plain,../movie.mkv",
    "data:text/plain,a%2Fb+%23c",
    "data://x",
    "data:,bad%",
    "data:,a|b",
    "urn:example:a/b?+resolver?=query#part",
    "mailto:user@example.test",
    "magnet:?xt=urn:btih:0123456789012345678901234567890123456789",
    "magnet:?xt=urn:btih:0123456789012345678901234567890123456789&tr=http%3a%2f%2fa"
    "&tr=http%3A%2F%2Fb&xt=urn:btmh:"
    "12200123456789012345678901234567890123456789012345678901234567890123"
    "&empty=&bare&&unknown=one+two#part",
};

// Native file names with a colon are not mistaken for URIs.
const char* const NativeColonFileNames[] = {
    "Re:Zero - 01.mkv",   "AC:DC - Thunderstruck.mp3", "Alien:Covenant (2017).mkv",
    "notes:2026",         "example+v1:disc2",          "Special:Edition.mkv",
    "Archive:Disc 1.chd", "Episodes:Part 2.mkv",       "Library:Collection.mkv",
};

// Only an explicit URI parse turns an unknown scheme into an identifier.
const char* const UnknownIdentifiers[] = {
    "example+v1:disc2",
    "notes:2026",
    "unknown:movie",
    "example+v1:/dir/movie.mkv?x=%2f&x=+#part",
};

struct URIPathMutation
{
  const char* input;
  const char* path;
  const char* expected;
};

// The new path is read in the context of the authority; every other component is kept.
const URIPathMutation URIPathMutations[] = {
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

struct VFSProjection
{
  const char* input;
  const char* protocol;
  const char* hostName;
  const char* fileName;
  bool encodedHostName;
};

// The legacy projection that VFS add-ons read through CVFSURLWrapper.
const VFSProjection VFSProjections[] = {
    {"sftp://server/dir/file", "sftp", "server", "dir/file", false},
    {"ssh://server/dir/file", "ssh", "server", "dir/file", false},
    {"smb://server/share/file", "smb", "server", "share/file", false},
    {"rar://%2fmedia%2fdisc.rar/dir/file", "rar", "/media/disc.rar", "dir/file", true},
    {"archive://%2fmedia%2fdisc.7z/dir/file", "archive", "/media/disc.7z", "dir/file", true},
};

struct DrivePath
{
  const char* path;
  const char* fileNameWithoutPath;
};

// A drive letter is not a one-letter scheme.
const DrivePath DrivePaths[] = {
    {"C:/file", "file"},
    {"C:\\file", "file"},
    {"C:relative", "relative"},
    {"C:relative.mkv", "relative.mkv"},
};

struct LocalPath
{
  const char* path;
  std::optional<bool> local;
  // The answer once the path is parsed as an explicit URI
  std::optional<bool> explicitURILocal;
};

// clang-format off
const LocalPath LocalPaths[] = {
    {"file:/videos/movie.mkv", true, true},
    {"file:movie.mkv", true, true},
    {"/videos/movie.mkv", true, std::nullopt},
    {"C:\\videos\\movie.mkv", true, std::nullopt},
    {"file://server/videos/movie.mkv", false, false},
    {"file:///videos/movie.mkv", false, false},
    {"data:,hello", false, false},
    {"urn:example:movie", false, false},
    {"magnet:?xt=urn:btih:hash", false, false},
    {"http:/videos/movie.mkv", false, false},
    {"https:/videos/movie.mkv", false, false},
    {"http://localhost/videos/movie.mkv", std::nullopt, false},
};
// clang-format on

struct Redaction
{
  const char* input;
  const char* redacted;
  // nullptr where not checked
  const char* withoutUserDetails;
  const char* withoutUserDetailsRedacted;
};

// clang-format off
const Redaction Redactions[] = {
    // A data payload is bounded out of a redacted location
    {"data:,hello", "data:[REDACTED]", "data:,hello", "data:[REDACTED]"},
    {"DATA://x?private#secret", "data:[REDACTED]", "DATA://x?private#secret", "data:[REDACTED]"},
    {"data:,bad%#secret", "data:[REDACTED]", "data:,bad%#secret", "data:[REDACTED]"},
    {"data:,a|password=secret#suffix", "data:[REDACTED]", "data:,a|password=secret#suffix", "data:[REDACTED]"},
    {"urn:example:abc", "urn:example:abc", nullptr, nullptr},
    {"custom://user:pass@host/path", "custom://USERNAME:PASSWORD@host/path", "custom://host/path", nullptr},
    // Wrappers that do not carry a resource keep their legacy spelling
    {"musicsearch://data%3a%2cSECRET/", "musicsearch://data%3a%2cSECRET/", nullptr, nullptr},
    {"episodes://data%3a%2cSECRET/", "episodes://data%3a%2cSECRET/", nullptr, nullptr},
    {"http://data%3a%2cSECRET/path", "http://data%3a%2cSECRET/path", nullptr, nullptr},
    {"http://host/path?next=data:,SECRET", "http://host/path?next=data:,SECRET", nullptr, nullptr},
    // Credentials are removed through an archive's encoded host and a stack's members
    {"zip://http%3a%2f%2fuser%3apass%40host%2fa.zip/member", "zip://http%3a%2f%2fhost%2fa.zip/member", "zip://http%3a%2f%2fhost%2fa.zip/member", nullptr},
    {"stack://http://user:pass@host/a , /plain/file", "stack://http://USERNAME:PASSWORD@host/a , /plain/file", nullptr, nullptr},
};
// clang-format on

struct WrappedData
{
  const char* location;
  const char* withoutUserDetails;
};

// clang-format off
const WrappedData WrappedDataLocations[] = {
    {"image://data%3a%2cSECRET/", "image://data%3a%2cSECRET/"},
    {"image://video@data%3a%2cSECRET/", "image://data%3a%2cSECRET/"},
    {"zip://data%3a%2cSECRET/member", "zip://data%3a%2cSECRET/member"},
    {"image://zip%3a%2f%2fdata%253a%252cSECRET%2fmember/", "image://zip%3a%2f%2fdata%253a%252cSECRET%2fmember/"},
    {"zip://image%3a%2f%2fdata%253a%252cSECRET%2f/member", "zip://image%3a%2f%2fdata%253a%252cSECRET%2f/member"},
    {"stack:///plain/file , image://data%3a%2cSECRET/", "stack:///plain/file , image://data%3a%2cSECRET/"},
    {"image://data%3a%2f%2fSECRET%3fx%3dSECRET%23SECRET/", "image://data%3a%2f%2fSECRET%3fx%3dSECRET%23SECRET/"},
    {"zip://data%3a%2cbad%25%7cSECRET%3fx%3dSECRET%23SECRET/member", "zip://data%3a%2cbad%25%7cSECRET%3fx%3dSECRET%23SECRET/member"},
};
// clang-format on

} // namespace

TEST(TestURLParsing, Serialization)
{
  for (const auto& [input, serialized, protocol, fileName] : SerializedURLs)
  {
    SCOPED_TRACE(input);
    const CURL url(input);
    EXPECT_EQ(serialized, url.Get());
    EXPECT_EQ(protocol, url.GetProtocol());
    EXPECT_EQ(fileName, url.GetFileName());
  }
  for (const auto& [path, expected] : FullPathCases)
  {
    EXPECT_EQ(expected, CURL::IsFullPath(path)) << path;
  }
}

TEST(TestURLParsing, LegacyOptionsAndReparse)
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

TEST(TestURLParsing, LosslessIdentifiers)
{
  for (const auto* example : LosslessIdentifiers)
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

TEST(TestURLParsing, RawComponentsAndSchemeSpecificPart)
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
    CURL url;
    ASSERT_TRUE(url.ParseURI(input));
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

TEST(TestURLParsing, SchemeSpecificPartView)
{
  const CURL data("DATA:text/plain,http://example.test/a?x=%2f+1#fragment");
  EXPECT_EQ("text/plain,http://example.test/a?x=%2f+1", data.GetSchemeSpecificPartView());

  const CURL malformed("data:,bad%?query#fragment");
  EXPECT_EQ(",bad%?query", malformed.GetSchemeSpecificPartView());

  CURL url("https://example.test/path?x=1");
  EXPECT_TRUE(url.GetSchemeSpecificPartView().empty());
  EXPECT_EQ("//example.test/path?x=1", url.GetSchemeSpecificPart());

  ASSERT_TRUE(url.ParseURI("https://example.test/path?x=1#fragment"));
  EXPECT_EQ("//example.test/path?x=1", url.GetSchemeSpecificPartView());

  url.ParseNativePath("data:,literal?query#fragment");
  EXPECT_TRUE(url.GetSchemeSpecificPartView().empty());
  url.Reset();
  EXPECT_TRUE(url.GetSchemeSpecificPartView().empty());
}

TEST(TestURLParsing, ExplicitAndMixedInputIntent)
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
  for (const std::string input : {"C://Movies/movie.mkv", "x://host/file"})
  {
    ASSERT_TRUE(explicitURI.ParseURI(input));
    EXPECT_TRUE(explicitURI.IsLosslessURI());
    EXPECT_EQ(input, explicitURI.Get());
    explicitURI.ParseNativePath(input);
    EXPECT_TRUE(explicitURI.GetProtocol().empty());
    EXPECT_FALSE(explicitURI.IsLosslessURI());
    EXPECT_EQ(input, explicitURI.Get());
  }
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

TEST(TestURLParsing, NativeColonFilenames)
{
  for (const std::string path : NativeColonFileNames)
  {
    SCOPED_TRACE(path);
    const CURL url{path};
    EXPECT_EQ(path, url.Get());
    EXPECT_EQ(path, url.GetFileName());
    EXPECT_TRUE(url.GetProtocol().empty());
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.SupportsPathOperations());
    EXPECT_TRUE(url.IsLocal());
    EXPECT_FALSE(CURL::IsFullPath(path));
  }
}

TEST(TestURLParsing, UnknownIdentifiersRequireExplicitURIIntent)
{
  for (const std::string identifier : UnknownIdentifiers)
  {
    SCOPED_TRACE(identifier);
    CURL url;
    ASSERT_TRUE(url.ParseURI(identifier));
    EXPECT_EQ(identifier, url.Get());
    EXPECT_TRUE(url.IsLosslessURI());
    EXPECT_FALSE(url.SupportsPathOperations());
    EXPECT_FALSE(url.IsLocal());
    EXPECT_TRUE(url.GetFileName().empty());
    EXPECT_TRUE(url.GetExtension().empty());
    EXPECT_EQ(identifier, url.GetWithoutUserDetails());
  }
}

TEST(TestURLParsing, LosslessMemberMutation)
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

TEST(TestURLParsing, URIPathMutationUsesAuthorityContext)
{
  for (const auto& test : URIPathMutations)
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

TEST(TestURLParsing, URIPathMutationRejectsComponentChanges)
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
      {
        continue;
      }
      if (!original.HasAuthority() && !original.IsProtocol("magnet") &&
          path == std::string_view("b"))
      {
        continue;
      }
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

TEST(TestURLParsing, FileNameSetterUsesURIPathAuthorityContext)
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

TEST(TestURLParsing, LosslessStateTransitions)
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
  EXPECT_EQ("?x=1", url.GetOptions());
  EXPECT_EQ("x=1", url.GetRawQuery());
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

TEST(TestURLParsing, ExplicitSchemeSetterTransitions)
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

TEST(TestURLParsing, MovedFromURIHasNoStaleComponents)
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

TEST(TestURLParsing, PathSettersDoNotRepairMalformedCandidates)
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

TEST(TestURLParsing, URIPathComponentsDoNotHaveNativeDrives)
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

TEST(TestURLParsing, LegacyProtocolToNativeClearsURIState)
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

TEST(TestURLParsing, SacdVFSCompatibilityProjection)
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

TEST(TestURLParsing, VFSCompatibilityProjections)
{
  for (const auto& test : VFSProjections)
  {
    SCOPED_TRACE(test.input);
    const CURL url(test.input);
    EXPECT_EQ(test.protocol, url.GetProtocol());
    EXPECT_EQ(test.hostName, url.GetHostName());
    EXPECT_EQ(test.fileName, url.GetFileName());
    EXPECT_EQ(test.encodedHostName, url.HasEncodedHostname());
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.SupportsPathOperations());
    EXPECT_EQ(test.input, url.Get());
  }
}

TEST(TestURLParsing, Redaction)
{
  for (const auto& [input, redacted, withoutUserDetails, withoutUserDetailsRedacted] : Redactions)
  {
    SCOPED_TRACE(input);
    const CURL url(input);
    EXPECT_EQ(redacted, url.GetRedacted());
    EXPECT_EQ(redacted, CURL::GetRedacted(input));
    if (withoutUserDetails)
    {
      EXPECT_EQ(withoutUserDetails, url.GetWithoutUserDetails(false));
    }
    if (withoutUserDetailsRedacted)
    {
      EXPECT_EQ(withoutUserDetailsRedacted, url.GetWithoutUserDetails(true));
    }
  }
}

TEST(TestURLParsing, LargeDataRedactionIsBounded)
{
  const std::string large = "data:," + std::string(1024 * 1024, 'a') + "#secret";
  EXPECT_EQ("data:[REDACTED]", CURL::GetRedacted(large));
}

TEST(TestURLParsing, RedactDataThroughResourceWrappers)
{
  for (const auto& [location, withoutUserDetails] : WrappedDataLocations)
  {
    SCOPED_TRACE(location);
    const CURL url(location);
    const std::string redacted = url.GetRedacted();
    EXPECT_EQ(std::string::npos, redacted.find("SECRET"));
    EXPECT_LT(redacted.size(), 128u);
    EXPECT_EQ(redacted, CURL::GetRedacted(location));
    EXPECT_EQ(redacted, url.GetWithoutUserDetails(true));
    EXPECT_EQ(location, url.Get());
    EXPECT_EQ(withoutUserDetails, url.GetWithoutUserDetails(false));
  }
}

TEST(TestURLParsing, WrappedDataRedactionIsBounded)
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

TEST(TestURLParsing, RedactDataThroughNestedStackEscapes)
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

TEST(TestURLParsing, RedactDataThroughLegacyWrapperSeparators)
{
  const std::string inner = "image:\\\\data%3a%2cSECRET/";
  const std::string outer = "image://" + CURL::Encode(inner) + "/";
  EXPECT_EQ("image:[REDACTED]", CURL(inner).GetRedacted());
  EXPECT_EQ("image:[REDACTED]", CURL::GetRedacted(inner));
  EXPECT_EQ("image:[REDACTED]", CURL(outer).GetRedacted());
  EXPECT_EQ("image:[REDACTED]", CURL::GetRedacted(outer));
  EXPECT_EQ(outer, CURL(outer).GetWithoutUserDetails(false));
}

TEST(TestURLParsing, RedactionRespectsStackMemberBoundaries)
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

TEST(TestURLParsing, RedactCredentialsThroughImageWrappers)
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

TEST(TestURLParsing, ExplicitWrappersRemoveNestedCredentials)
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

TEST(TestURLParsing, ExplicitCredentialFreeWrappersPreserveSpelling)
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

TEST(TestURLParsing, LargeDataInEncodedStackRedactsWithoutChangingResource)
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

TEST(TestURLParsing, CustomVFSProtocolsRetainLegacyProjection)
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

TEST(TestURLParsing, OneLetterVFSProtocolsRetainLegacyProjection)
{
#ifndef TARGET_WINDOWS
  for (const std::string scheme : {"x", "X"})
  {
    const CURL url(scheme + "://user:pass@host:123/share/movie.mkv");
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.SupportsPathOperations());
    EXPECT_EQ("x", url.GetProtocol());
    EXPECT_EQ("host", url.GetHostName());
    EXPECT_EQ("user", url.GetUserName());
    EXPECT_EQ("pass", url.GetPassWord());
    EXPECT_EQ(123, url.GetPort());
    EXPECT_EQ("share/movie.mkv", url.GetFileName());
    EXPECT_EQ("share", url.GetShareName());
    EXPECT_EQ("mkv", url.GetFileType());
    EXPECT_EQ("x://user:pass@host:123/share/movie.mkv", url.Get());
  }
#endif
}

TEST(TestURLParsing, DriveLetterPaths)
{
  for (const auto& [path, fileNameWithoutPath] : DrivePaths)
  {
    SCOPED_TRACE(path);
    const CURL url(path);
    EXPECT_TRUE(url.GetProtocol().empty());
    EXPECT_TRUE(url.GetHostName().empty());
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_FALSE(URIUtils::IsURL(path));
    EXPECT_EQ(fileNameWithoutPath, url.GetFileNameWithoutPath());
  }
}

#ifdef TARGET_WINDOWS
TEST(TestURLParsing, DrivePrefixesPrecedeImplicitOneLetterSchemes)
{
  for (const std::string drive : {"C", "c", "D", "x"})
  {
    SCOPED_TRACE(drive);
    const CURL url(drive + "://Movies/movie.mkv");
    EXPECT_TRUE(url.GetProtocol().empty());
    EXPECT_TRUE(url.GetHostName().empty());
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.IsLocal());
    EXPECT_EQ(drive + R"(:\\Movies\movie.mkv)", url.GetFileName());
    EXPECT_EQ(drive + R"(:\\Movies\movie.mkv)", url.Get());
  }
}
#endif

TEST(TestURLParsing, IsLocal)
{
  for (const auto& [path, local, explicitURILocal] : LocalPaths)
  {
    SCOPED_TRACE(path);
    if (local)
    {
      EXPECT_EQ(*local, CURL(path).IsLocal());
    }
    if (explicitURILocal)
    {
      CURL explicitURI;
      ASSERT_TRUE(explicitURI.ParseURI(path));
      EXPECT_EQ(*explicitURILocal, explicitURI.IsLocal());
    }
  }
}

TEST(TestURLParsing, OneLetterVFSProtocolsSupportLegacyConstruction)
{
  for (const std::string scheme : {"x", "X"})
  {
    SCOPED_TRACE(scheme);
    CURL url;
    url.SetProtocol(scheme);
    url.SetHostName("host");
    url.SetFileName("file");
    EXPECT_EQ("x://host/file", url.Get());
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.SupportsPathOperations());

    url.SetUserName("user");
    url.SetPassword("pass");
    url.SetPort(123);
    url.SetFileName("share/movie.mkv");
    url.SetOptions("?key=value");
    url.SetProtocolOptions("cache=no");
    EXPECT_EQ("x", url.GetProtocol());
    EXPECT_EQ("host", url.GetHostName());
    EXPECT_EQ("user", url.GetUserName());
    EXPECT_EQ("pass", url.GetPassWord());
    EXPECT_EQ(123, url.GetPort());
    EXPECT_EQ("share/movie.mkv", url.GetFileName());
    EXPECT_EQ("share", url.GetShareName());
    EXPECT_EQ("value", url.GetOption("key"));
    EXPECT_EQ("no", url.GetProtocolOption("cache"));
    EXPECT_EQ("x://user:pass@host:123/share/movie.mkv?key=value|cache=no", url.Get());
  }
}

TEST(TestURLParsing, ExplicitUnknownURIRetainsLosslessIntent)
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

TEST(TestURLParsing, PlaylistProtocolsRetainLegacySemantics)
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

TEST(TestURLParsing, OneLetterSchemeTransitionsPreserveRepresentation)
{
  CURL uri;
  ASSERT_TRUE(uri.ParseURI("x://host/path?x=1#part"));
  EXPECT_TRUE(uri.IsLosslessURI());
  EXPECT_EQ("x://host/path?x=1#part", uri.Get());
  ASSERT_TRUE(uri.ParseURI("custom://host/path?x=1#part"));
  uri.SetProtocol("a");
  EXPECT_TRUE(uri.IsLosslessURI());
  EXPECT_EQ("a://host/path?x=1#part", uri.Get());
  EXPECT_EQ("host", uri.GetRawAuthority());
  EXPECT_EQ("x=1", uri.GetRawQuery());
  EXPECT_EQ("part", uri.GetRawFragment());
  EXPECT_FALSE(uri.SupportsPathOperations());
  EXPECT_FALSE(uri.SetURIPath("/next"));
  EXPECT_EQ("a://host/path?x=1#part", uri.Get());
  CURL legacy("http://host/path?x=1#part");
  legacy.SetProtocol("a");
  EXPECT_FALSE(legacy.IsLosslessURI());
  EXPECT_TRUE(legacy.SupportsPathOperations());
  EXPECT_EQ("a://host/path?x=1#part", legacy.Get());

  for (const std::string protocol : {"smb", "custom+vfs"})
  {
    SCOPED_TRACE(protocol);
    CURL url(protocol + "://host/dir/file.mkv");
    url.SetProtocol("x");
    EXPECT_FALSE(url.IsLosslessURI());
    EXPECT_TRUE(url.SupportsPathOperations());
    EXPECT_EQ("x://host/dir/file.mkv", url.Get());
    EXPECT_EQ("host", url.GetHostName());
    EXPECT_EQ("file.mkv", URIUtils::GetFileName(url));
    url.SetFileName("dir/next.mkv");
    EXPECT_EQ("x://host/dir/next.mkv", url.Get());
#ifndef TARGET_WINDOWS
    const CURL reconstructed(url.Get());
    EXPECT_FALSE(reconstructed.IsLosslessURI());
    EXPECT_TRUE(reconstructed.SupportsPathOperations());
    EXPECT_EQ("x://host/dir/next.mkv", reconstructed.Get());
    EXPECT_EQ("dir/next.mkv", reconstructed.GetFileName());
    EXPECT_EQ("x://host/dir/", URIUtils::GetDirectory(reconstructed.Get()));
#endif
  }
}
