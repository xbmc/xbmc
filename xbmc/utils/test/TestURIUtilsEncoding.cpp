/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "utils/URIUtils.h"

#include <array>
#include <span>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace
{

struct URLEncodings
{
  std::string_view input;
  std::string_view encoded;
};

constexpr URLEncodings EncodingTestData[] = {
    // No crash
    {"", ""},

    // No Encoding needed
    {"a", "a"},
    {"z", "z"},
    {"A", "A"},
    {"Z", "Z"},
    {"0", "0"},
    {"9", "9"},

    // Encoding needed
    {" ", "%20"},
    {"\"", "%22"},
    {"#", "%23"},
    {"$", "%24"},
    {"%", "%25"},
    {"&", "%26"},
    {"'", "%27"},
    {"*", "%2a"},
    {"+", "%2b"},
    {",", "%2c"},
    {"/", "%2f"},
    {":", "%3a"},
    {";", "%3b"},
    {"<", "%3c"},
    {"=", "%3d"},
    {">", "%3e"},
    {"?", "%3f"},
    {"@", "%40"},
    {"[", "%5b"},
    {"\\", "%5c"},
    {"]", "%5d"},
    {"^", "%5e"},
    {"`", "%60"},
    {"{", "%7b"},
    {"|", "%7c"},
    {"}", "%7d"},
    {"\u03A9", "%ce%a9"}, // Greek Capital Letter Omega

    // Encoding needed (Non alpha)
    {"\x1", "%01"},
    {"\x2", "%02"},
    {"\x6", "%06"},
    {"\a", "%07"},
    {"\b", "%08"},
    {"\t", "%09"},
    {"\n", "%0a"},
    {"\v", "%0b"},
    {"\f", "%0c"},
    {"\r", "%0d"},

    // Double Encoding
    {"%20", "%2520"},
    {"%22", "%2522"},
    {"%2a", "%252a"},
    {"%2b", "%252b"},
    {"%2c", "%252c"},
    {"%2f", "%252f"},
    {"%0a", "%250a"},
    {"%0b", "%250b"},
    {"%0c", "%250c"},
};

constexpr URLEncodings RFC1738EncodingTestData[] = {
    {"-", "-"}, {"_", "_"}, {".", "."}, {"!", "!"}, {"(", "("}, {")", ")"}, {"~", "%7e"},
};

constexpr URLEncodings RFC3986EncodingTestData[] = {
    {"-", "-"}, {"_", "_"}, {".", "."}, {"!", "%21"}, {"(", "%28"}, {")", "%29"}, {"~", "~"},
};

constexpr URLEncodings InvalidEncodingTestData[] = {
    // Incomplete or Invalid encoding
    {"%", "%"},
    {"%%", "%%"},
    {"%;", "%;"},
    {"%-", "%-"},
    {"%2", "%2"},
    {"%2-", "%2-"},
    {"%2x", "%2x"},

    // Incomplete or Invalid encoding recovers to decode future characters
    {"% ", "%%20"},
    {"%% ", "%%%20"},
    {"%; ", "%;%20"},
    {"%- ", "%-%20"},
    {"%2 ", "%2%20"},
    {"%2- ", "%2-%20"},
    {"%2x ", "%2x%20"},
};

// Decodings that URLEncode() never produces.
constexpr URLEncodings DecodeOnlyTestData[] = {
    {" ", "+"}, {"C   Collection", "C++ Collection"}, {"{", "%7B"}, {"|", "%7C"}, {"}", "%7D"},
};

struct SanitiseUrlEncodingTestData
{
  std::string_view dirty;
  std::string_view clean;
};

constexpr SanitiseUrlEncodingTestData SanitiseUrlEncodingData[] = {
    // lower case encoding and '\' in encoded path
    {"rar://D%3A%2FMovies%2Fmovie.rar/movie.mkv", "rar://D%3a%5cMovies%5cmovie.rar/movie.mkv"},
    // lower case encoding and '\' in encoded path
    {"archive://D%3A%2FMovies%2Fmovie.rar/movie.mkv",
     "archive://D%3a%5cMovies%5cmovie.rar/movie.mkv"},
    // lower case encoding
    {"rar://D%3A%5CMovies%5Cmovie.rar/movie.mkv", "rar://D%3a%5cMovies%5cmovie.rar/movie.mkv"},
    // '\' in encoded path
    {"rar://D%3a%2fMovies%2fmovie.rar/movie.mkv", "rar://D%3a%5cMovies%5cmovie.rar/movie.mkv"},
    // lower case encoding
    {"rar://%2FMovies%2Fmovie.rar/movie.mkv", "rar://%2fMovies%2fmovie.rar/movie.mkv"},
    // unchanged
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/episode/3/4",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/episode/3/4"},
    // only sanitise hostname
    {"rar://D%3A%2FMovies%2Fmovie.rar/movie%DD/movie.mkv",
     "rar://D%3a%5cMovies%5cmovie.rar/movie%DD/movie.mkv"},
    // Malformed encoding: lone % at end of hostname - passed through unchanged
    {"rar://D%3a%5cMovies%5cmovie%.rar/movie.mkv", "rar://D%3a%5cMovies%5cmovie%.rar/movie.mkv"},
    // Malformed encoding: % followed by single hex digit then end of hostname
    {"rar://D%3a%5cMovies%5cmovie%2.rar/movie.mkv", "rar://D%3a%5cMovies%5cmovie%2.rar/movie.mkv"},
    // Malformed encoding: % followed by one valid then one invalid hex digit
    {"rar://D%3a%5cMovies%5cmovie%2Z.rar/movie.mkv",
     "rar://D%3a%5cMovies%5cmovie%2Z.rar/movie.mkv"},
    // Malformed encoding: % followed by two invalid hex digits
    {"rar://D%3a%5cMovies%5cmovie%ZZ.rar/movie.mkv",
     "rar://D%3a%5cMovies%5cmovie%ZZ.rar/movie.mkv"},
    // Malformed encoding followed by valid encoding: invalid sequence unchanged, valid sequences lowercased
    {"rar://D%3A%5CMovies%5C%2Zmovie.rar/movie.mkv",
     "rar://D%3a%5cMovies%5c%2Zmovie.rar/movie.mkv"},
    // Malformed protocol with no hostname or path at all - empty hostname
    {"rar://", "rar://"},
    // Malformed protocol marker at very end of string - not a valid protocol, returned as-is
    {"rar:/", "rar:/"},
    // Malformed with no :// at all - no protocol, returned as-is
    {"D%3A%5CMovies%5Cmovie.rar", "D%3A%5CMovies%5Cmovie.rar"},
    // Malformed hostname only, no trailing slash or path
    {"rar://D%3A%5Cmovie.rar", "rar://D%3a%5cmovie.rar"},
    // An authority-free URI, a native path and an identifier are left as they are
    {"http:/a%20b?x=%2F+%25#part%23", "http:/a%20b?x=%2F+%25#part%23"},
    {"Re:Zero%2F.mkv", "Re:Zero%2F.mkv"},
    {"/media/archive.zip/a%2F.mkv", "/media/archive.zip/a%2F.mkv"},
    {"data://host%2F/body", "data://host%2F/body"},
    {"urn://host%2F/body", "urn://host%2F/body"},
    {"magnet://host%2F/body", "magnet://host%2F/body"},
    {"magnet:/movie?xt=%2F#part", "magnet:/movie?xt=%2F#part"},
    // A legacy URL has only its hostname sanitised
    {"custom+vfs://host%2F/file%2F?x=%2F#part", "custom+vfs://host%2f/file%2F?x=%2F#part"},
#ifndef TARGET_WINDOWS
    {"x://host%2F/file%2F", "x://host%2f/file%2F"},
#endif
};

class TestSanitiseUrlEncoding : public testing::Test,
                                public testing::WithParamInterface<SanitiseUrlEncodingTestData>
{
};

struct DecodingCase
{
  const char* input;
  const char* decoded;
};

const DecodingCase DecodePathEscapesCases[] = {
    {"file%20name", "file name"},
    {"100%25 proof", "100% proof"},
    {"C++ Collection", "C++ Collection"},
};

const DecodingCase GetDecodedFileNameCases[] = {
    {"/path/to/movie.avi", "movie.avi"},
    {"davs://host/the%20movie.avi", "the movie.avi"},
    {"davs://host/100%25%20proof.avi", "100% proof.avi"},
    {"davs://host/C++%20Collection.mkv", "C++ Collection.mkv"},
};

const DecodingCase GetDecodedPathCases[] = {
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "bluray://udf://smb://somepath/path/movie.iso//BDMV/PLAYLIST/00800.mpls"},
    {"bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://smb://somepath/path//BDMV/PLAYLIST/00800.mpls"},
    {"http:/a%20b?x=%2F+%25#part%23", "http:/a%20b?x=%2F+%25#part%23"},
};

struct MatchingEncodingCase
{
  const char* folder;
  const char* segment;
  const char* expected;
};

// Folders whose file names are URL encoded (WebDAV and http) get the segment encoded to match.
const MatchingEncodingCase EncodedFolderCases[] = {
    {"davs://user:pass@host/Movie%20Set%20Information%20Folder/", "Star Wars Collection",
     "davs://user:pass@host/Movie%20Set%20Information%20Folder/Star%20Wars%20Collection"},
    {"dav://host/sets/", "Star Wars Collection", "dav://host/sets/Star%20Wars%20Collection"},
    {"http://host/sets/", "Star Wars Collection", "http://host/sets/Star%20Wars%20Collection"},
    {"https://host/sets/", "Star Wars Collection", "https://host/sets/Star%20Wars%20Collection"},
    // A missing slash at the end of the folder is handled as by AddFileToFolder()
    {"https://host/sets", "Star Wars Collection", "https://host/sets/Star%20Wars%20Collection"},
    // An empty segment leaves the folder untouched (bar the trailing slash)
    {"https://host/sets/", "", "https://host/sets/"},
    // Characters legal in a file name but not in a URL
    {"https://host/sets/", "Fast & Furious Collection",
     "https://host/sets/Fast%20%26%20Furious%20Collection"},
    {"https://host/sets/", "Marvel's Avengers", "https://host/sets/Marvel%27s%20Avengers"},
    // A percent sign is escaped, not passed through as an escape sequence
    {"https://host/sets/", "100% Collection", "https://host/sets/100%25%20Collection"},
    // Non-ASCII is encoded byte-wise (UTF-8, lowercase hex as produced by URLEncode)
    {"https://host/sets/", "Am\xc3\xa9lie Collection",
     "https://host/sets/Am%c3%a9lie%20Collection"},
    // Characters exempt from encoding per RFC1738 are kept verbatim
    {"https://host/sets/", "Alien-Quadrilogy_(1979).!",
     "https://host/sets/Alien-Quadrilogy_(1979).!"},
};

} // namespace

TEST(TestURIUtilsEncoding, URLEncode)
{
  for (const auto& [toEncode, expected] : EncodingTestData)
  {
    EXPECT_EQ(expected, URIUtils::URLEncode(toEncode));
  }

  for (const auto& [toEncode, expected] : RFC1738EncodingTestData)
  {
    EXPECT_EQ(expected, URIUtils::URLEncode(toEncode));
  }

  for (const auto& [toEncode, expected] : RFC3986EncodingTestData)
  {
    EXPECT_EQ(expected, URIUtils::URLEncode(toEncode, URIUtils::RFC3986));
  }
}

TEST(TestURIUtilsEncoding, URLDecode)
{
  const std::span<const URLEncodings> decodings[] = {EncodingTestData, InvalidEncodingTestData,
                                                     RFC1738EncodingTestData,
                                                     RFC3986EncodingTestData, DecodeOnlyTestData};
  for (const auto& data : decodings)
  {
    for (const auto& [expected, encoded] : data)
    {
      EXPECT_EQ(expected, URIUtils::URLDecode(encoded));
    }
  }
}

TEST(TestURIUtilsEncoding, URLEncodeDecode)
{
  for (unsigned ch = 1; ch < 128; ++ch)
  {
    std::string str{char(ch)};
    EXPECT_EQ(str, CURL::Decode(CURL::Encode(str)));
  }

  for (const auto& [toEncode, encoded] : EncodingTestData)
  {
    EXPECT_EQ(toEncode, URIUtils::URLDecode(CURL::Encode(toEncode)));
    EXPECT_EQ(encoded, URIUtils::URLDecode(CURL::Encode(encoded)));
  }
}

TEST_P(TestSanitiseUrlEncoding, SanitiseUrlEncoding)
{
  EXPECT_EQ(GetParam().clean, URIUtils::SanitiseUrlEncoding(std::string{GetParam().dirty}));
}

INSTANTIATE_TEST_SUITE_P(TestURIUtilsEncoding,
                         TestSanitiseUrlEncoding,
                         testing::ValuesIn(SanitiseUrlEncodingData));

TEST(TestURIUtilsEncoding, DecodePathEscapes)
{
  for (const auto& [input, decoded] : DecodePathEscapesCases)
  {
    EXPECT_EQ(decoded, URIUtils::DecodePathEscapes(input)) << input;
  }
}

TEST(TestURIUtilsEncoding, GetDecodedFileName)
{
  for (const auto& [path, decoded] : GetDecodedFileNameCases)
  {
    EXPECT_EQ(decoded, URIUtils::GetDecodedFileName(path)) << path;
  }
}

TEST(TestURIUtilsEncoding, GetDecodedPath)
{
  for (const auto& [path, decoded] : GetDecodedPathCases)
  {
    EXPECT_EQ(decoded, URIUtils::GetDecodedPath(path)) << path;
  }
}

TEST(TestURIUtilsEncoding, AddFileToFolderMatchingEncodingEncodedFolder)
{
  for (const auto& [folder, segment, expected] : EncodedFolderCases)
  {
    const std::string path{URIUtils::AddFileToFolderMatchingEncoding(folder, segment)};
    EXPECT_EQ(expected, path) << folder << " + " << segment;
    EXPECT_EQ(std::string::npos, CURL(path).GetFileName().find(' ')) << path;
  }
}

TEST(TestURIUtilsEncoding, AddFileToFolderMatchingEncodingUnencodedFolder)
{
  // Folders that don't use an encoded file name must behave exactly like AddFileToFolder(),
  // i.e. the segment is appended verbatim and stays human readable.
  const std::array<std::string, 5> folders = {"/home/user/sets/", "smb://host/share/sets/",
                                              "nfs://host/export/sets/", "C:\\videos\\sets\\",
                                              "ftp://host/sets/"};

  const std::string title = "Star Wars Collection";
  for (const auto& folder : folders)
  {
    const std::string var = URIUtils::AddFileToFolderMatchingEncoding(folder, title);
    EXPECT_EQ(URIUtils::AddFileToFolder(folder, title), var) << "folder: " << folder;
    EXPECT_NE(std::string::npos, var.find(title)) << "folder: " << folder;
  }

  // An empty folder is passed through unchanged as well.
  EXPECT_EQ(URIUtils::AddFileToFolder("", "Star Wars Collection"),
            URIUtils::AddFileToFolderMatchingEncoding("", "Star Wars Collection"));
}
