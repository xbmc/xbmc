/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/DataFile.h"
#include "filesystem/File.h"

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace
{
constexpr size_t MEBIBYTE = 1024 * 1024;
constexpr size_t MAX_ENCODED_DATA_SIZE = 128 * MEBIBYTE;
constexpr size_t MAX_DECODED_DATA_SIZE = 64 * MEBIBYTE;

void ExpectContents(std::string_view uri, std::string_view expected)
{
  SCOPED_TRACE(std::string(uri));

  XFILE::CFile file;
  ASSERT_TRUE(file.Open(std::string(uri)));
  ASSERT_EQ(static_cast<int64_t>(expected.size()), file.GetLength());
  ASSERT_EQ(0, file.GetPosition());

  std::string contents(expected.size(), '\0');
  EXPECT_EQ(static_cast<ssize_t>(contents.size()), file.Read(contents.data(), contents.size()));
  EXPECT_EQ(expected, contents);
  EXPECT_EQ(static_cast<int64_t>(expected.size()), file.GetPosition());

  char byte{};
  EXPECT_EQ(0, file.Read(&byte, sizeof(byte)));
}

void ExpectOpenFails(std::string_view uri)
{
  SCOPED_TRACE(std::string(uri));

  XFILE::CFile file;
  EXPECT_FALSE(file.Open(std::string(uri)));
}

void ExpectMetadataFails(std::string_view uri)
{
  SCOPED_TRACE(std::string(uri));

  const CURL url{std::string{uri}};
  XFILE::CDataFile file;
  EXPECT_FALSE(file.Exists(url));

  struct __stat64 buffer
  {
  };
  EXPECT_EQ(-1, file.Stat(url, &buffer));
}
} // namespace

TEST(TestDataFile, OfficialBriefNoteExampleAndDefaults)
{
  // RFC 2397's "A brief note" example uses the default
  // text/plain;charset=US-ASCII media type.
  ExpectContents("data:,A%20brief%20note", "A brief note");
  ExpectContents("data:;charset=US-ASCII,A%20brief%20note", "A brief note");
  ExpectContents("data:;charset=UTF-8,caf%C3%A9", "caf\xC3\xA9");
}

TEST(TestDataFile, MediaTypesParametersAndCase)
{
  ExpectContents("DATA:TEXT/PLAIN;CHARSET=UTF-8,hello", "hello");
  ExpectContents("data:application/vnd.kodi.test+json;version=1,%7B%7D", "{}");
  ExpectContents("data:application/x-kodi-test;version=1;name=a%20b%3Bc%3Dd,hello", "hello");
  ExpectContents("data:text/plain;CHARSET=UTF-8;BaSe64,SGVsbG8=", "Hello");

  // A parameter named base64 is not the encoding marker because it has '='.
  ExpectContents("data:text/plain;base64=not-a-marker,SGVsbG8=", "SGVsbG8=");
  ExpectContents("data:text/plain;base64=parameter;base64,SGVsbG8=", "Hello");
}

TEST(TestDataFile, Base64Text)
{
  ExpectContents("data:text/plain;base64,SGVsbG8gd29ybGQ=", "Hello world");
}

TEST(TestDataFile, Base64Json)
{
  ExpectContents("data:application/json;base64,"
                 "eyJ2ZXJzaW9uIjoxLCJpZCI6ImNpbmVtYXRpYy5lYXJ0aCIsIm5hbWUiOiJjaW5lbWF0aWMu"
                 "ZWFydGgifQ==",
                 R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth"})");
}

TEST(TestDataFile, PercentEncodedText)
{
  ExpectContents("data:text/plain,Hello%20world", "Hello world");
}

TEST(TestDataFile, UsesFirstCommaAsDelimiter)
{
  ExpectContents("data:text/plain,a%2Cb,c", "a,b,c");
  ExpectContents("data:text/plain;note=one%2Ctwo,three,four", "three,four");
}

TEST(TestDataFile, EmptyForms)
{
  ExpectContents("data:,", "");
  ExpectContents("data:text/plain,", "");
  ExpectContents("data:;base64,", "");
  ExpectContents("data:application/octet-stream;base64,", "");
}

TEST(TestDataFile, UriCharactersAndLiteralPlus)
{
  constexpr std::string_view chars{"AZaz09;/?:@&=+$,-_.!~*'()"};
  ExpectContents(std::string{"data:application/octet-stream,"} + std::string{chars}, chars);
  ExpectContents("data:,+%2B", "++");
  ExpectContents("data:,%25", "%");

  // Characters outside RFC 2396's uric set are valid data when escaped.
  ExpectContents("data:,%20%22%23%5B%5D%5C%5E%60%7B%7C%7D", " \"#[]\\^`{|}");
}

TEST(TestDataFile, FragmentsAreNotPayload)
{
  ExpectContents("data:text/plain,before#not-data", "before");
  ExpectContents("data:text/plain,%23#not-data", "#");
  ExpectContents("data:,https://example.test/a?b#fragment", "https://example.test/a?b");
}

TEST(TestDataFile, BinaryDataAndPercentBeforeBase64)
{
  const std::string expected{"\x00\x01\x02\x00\xff", 5};
  ExpectContents("data:application/octet-stream,%00%01%02%00%FF", expected);
  ExpectContents("data:application/octet-stream;base64,AAECAP8=", expected);
  ExpectContents("data:application/octet-stream;base64,%41%41%45%43%41%50%38%3D", expected);

  const std::string significantChars{"\xfb\xff", 2};
  ExpectContents("data:application/octet-stream;base64,+/8=", significantChars);
  ExpectContents("data:application/octet-stream;base64,%2B/8%3D", significantChars);
}

TEST(TestDataFile, CanonicalBase64)
{
  ExpectContents("data:;base64,", "");
  ExpectContents("data:;base64,Zg==", "f");
  ExpectContents("data:;base64,Zm8=", "fo");
  ExpectContents("data:;base64,Zm9v", "foo");
  ExpectContents("data:;base64,Zm9vYg==", "foob");
  ExpectContents("data:;base64,Zg%3D%3D", "f");

  for (const std::string_view uri : {
           "data:;base64,Z", "data:;base64,Zg", "data:;base64,Zm8", "data:;base64,Zg=",
           "data:;base64,Zg===", "data:;base64,=Zg=", "data:;base64,Z=g=", "data:;base64,Zg=A",
           "data:;base64,Zm=8", "data:;base64,Zm9v=", "data:;base64,Zm$v", "data:;base64,Zm-v",
           "data:;base64,Zm_v",
           "data:;base64,Zh==", // Nonzero unused bits.
           "data:;base64,Zm9=", // Nonzero unused bits.
       })
    ExpectOpenFails(uri);
}

TEST(TestDataFile, MimeBase64Whitespace)
{
  // RFC 2397 requires octets outside urlchar to be percent-encoded. After
  // decoding those escapes, RFC 2045 Base64 permits MIME whitespace.
  ExpectContents("data:;base64,Zg%20%3D%3D", "f");
  ExpectContents("data:;base64,Zg%09%3D%3D", "f");
  ExpectContents("data:;base64,Zg%0D%3D%3D", "f");
  ExpectContents("data:;base64,Zg%0A%3D%3D", "f");
  ExpectContents("data:;base64,Zg%0D%0A%3D%3D", "f");
  ExpectContents("data:;base64,%09Z%20g%0D%3D%0A%3D%20", "f");
  ExpectContents("data:;base64,%20%09%0D%0A", "");

  for (const std::string_view uri : {
           "data:;base64,Zg$=",
           "data:;base64,Z$g==",
           "data:;base64,Zg%00==",
           "data:;base64,Zg%7F==",
           "data:;base64,Zg%0B==",
           "data:;base64,Zg%0C==",
       })
    ExpectOpenFails(uri);
}

TEST(TestDataFile, Seek)
{
  XFILE::CFile file;
  ASSERT_TRUE(file.Open("data:text/plain,abcdef"));
  ASSERT_EQ(6, file.GetLength());

  EXPECT_EQ(2, file.Seek(2, SEEK_SET));
  char byte{};
  ASSERT_EQ(1, file.Read(&byte, sizeof(byte)));
  EXPECT_EQ('c', byte);

  EXPECT_EQ(4, file.Seek(1, SEEK_CUR));
  ASSERT_EQ(1, file.Read(&byte, sizeof(byte)));
  EXPECT_EQ('e', byte);

  EXPECT_EQ(5, file.Seek(-1, SEEK_END));
  ASSERT_EQ(1, file.Read(&byte, sizeof(byte)));
  EXPECT_EQ('f', byte);
  EXPECT_EQ(0, file.Read(&byte, sizeof(byte)));

  // Match regular-file semantics: seeking beyond EOF is valid and reads EOF.
  EXPECT_EQ(11, file.Seek(11, SEEK_SET));
  EXPECT_EQ(0, file.Read(&byte, sizeof(byte)));

  EXPECT_EQ(-1, file.Seek(-1, SEEK_SET));
  EXPECT_EQ(11, file.GetPosition());
  EXPECT_EQ(-1, file.Seek(-7, SEEK_END));
  EXPECT_EQ(11, file.GetPosition());
  EXPECT_EQ(-1, file.Seek(0, 12345));
  EXPECT_EQ(11, file.GetPosition());

  EXPECT_EQ(std::numeric_limits<int64_t>::max(),
            file.Seek(std::numeric_limits<int64_t>::max(), SEEK_SET));
  EXPECT_EQ(-1, file.Seek(1, SEEK_CUR));
  EXPECT_EQ(std::numeric_limits<int64_t>::max(), file.GetPosition());
}

TEST(TestDataFile, MalformedMediaTypesAndParametersFail)
{
  for (const std::string_view uri : {
           "data:text,hello",
           "data:/plain,hello",
           "data:text/,hello",
           "data:text//plain,hello",
           "data:text/plain/extra,hello",
           "data:text%2Fplain,hello",
           "data:text/plain;,hello",
           "data:text/plain;;charset=UTF-8,hello",
           "data:text/plain;charset,hello",
           "data:text/plain;=UTF-8,hello",
           "data:text/plain;charset=,hello",
           "data:text/plain;charset=UTF-8=extra,hello",
           "data:text/plain;charset=%00,hello",
           "data:text/plain;charset=%7F,hello",
           "data:text/plain;bad%3Aname=value,hello",
       })
    ExpectOpenFails(uri);
}

TEST(TestDataFile, MisplacedOrMalformedBase64MarkersFail)
{
  for (const std::string_view uri : {
           "data:text/plain;base64;charset=UTF-8,SGVsbG8=",
           "data:text/plain;charset=UTF-8;base64;version=1,SGVsbG8=",
           "data:text/plain;base64;base64,SGVsbG8=",
           "data:text/plain;notbase64,hello",
           "data:text/plain;base64;,SGVsbG8=",
       })
    ExpectOpenFails(uri);
}

TEST(TestDataFile, MalformedUriCharactersAndEscapesFail)
{
  for (const std::string_view uri : {
           "data:text/plain",
           "data:text/plain,bad%",
           "data:text/plain,bad%2",
           "data:text/plain,bad%GG",
           "data:text/plain,bad value",
           "data:text/plain,bad\"value",
           "data:text/plain,bad[value]",
           "data:text/plain,bad\\value",
           "data:text/plain,bad^value",
           "data:text/plain,bad`value",
           "data:text/plain,bad{value}",
           "data:text/plain,bad|value",
           "data:text/plain#fragment,hello",
           "data:text/plain;base64,%%%%",
           "data:text/plain;base64,SGVsbG8%2",
           "data://host/text/plain,Hello",
           "data:///text/plain,Hello",
           "data:/text/plain,Hello",
       })
    ExpectOpenFails(uri);
}

TEST(TestDataFile, EncodedSizeBoundary)
{
  // The limit covers everything after "data:", including metadata and comma.
  constexpr size_t schemePrefixSize = std::string_view{"data:"}.size();
  std::string uri{"data:text/plain;x="};
  uri.reserve(schemePrefixSize + MAX_ENCODED_DATA_SIZE + 1);
  uri.append(MAX_ENCODED_DATA_SIZE - (uri.size() - schemePrefixSize) - 1, 'a');
  uri.push_back(',');
  ASSERT_EQ(schemePrefixSize + MAX_ENCODED_DATA_SIZE, uri.size());

  {
    const CURL url{uri};
    XFILE::CDataFile file;
    ASSERT_TRUE(file.Open(url));
    EXPECT_EQ(0, file.GetLength());
  }

  uri.push_back('x');
  ASSERT_EQ(schemePrefixSize + MAX_ENCODED_DATA_SIZE + 1, uri.size());

  const CURL url{uri};
  XFILE::CDataFile file;
  EXPECT_FALSE(file.Open(url));
}

TEST(TestDataFile, DecodedSizeBoundary)
{
  std::string uri{"data:,"};
  uri.reserve(uri.size() + MAX_DECODED_DATA_SIZE + 1);
  uri.append(MAX_DECODED_DATA_SIZE, 'x');

  {
    const CURL url{uri};
    XFILE::CDataFile file;
    ASSERT_TRUE(file.Open(url));
    EXPECT_EQ(static_cast<int64_t>(MAX_DECODED_DATA_SIZE), file.GetLength());
  }

  uri.push_back('x');

  const CURL url{uri};
  XFILE::CDataFile file;
  EXPECT_FALSE(file.Open(url));
}

TEST(TestDataFile, ExistsAndStat)
{
  const std::string uri{"data:application/octet-stream;base64,AAECAP8="};

  EXPECT_TRUE(XFILE::CFile::Exists(uri, false));
  EXPECT_FALSE(XFILE::CFile::Exists("data:text/plain;base64", false));

  struct __stat64 buffer
  {
  };
  ASSERT_EQ(0, XFILE::CFile::Stat(uri, &buffer));
  EXPECT_EQ(5, buffer.st_size);
  EXPECT_EQ(_S_IFREG, buffer.st_mode & _S_IFREG);

  XFILE::CFile file;
  ASSERT_TRUE(file.Open(uri));
  buffer = {};
  ASSERT_EQ(0, file.Stat(&buffer));
  EXPECT_EQ(5, buffer.st_size);
  EXPECT_EQ(_S_IFREG, buffer.st_mode & _S_IFREG);
}

TEST(TestDataFile, MetadataQueriesValidatePayload)
{
  const CURL validUrl{"data:;base64,Zg%0D%0A%3D%3D"};
  XFILE::CDataFile file;
  EXPECT_TRUE(file.Exists(validUrl));

  struct __stat64 buffer
  {
  };
  ASSERT_EQ(0, file.Stat(validUrl, &buffer));
  EXPECT_EQ(1, buffer.st_size);

  for (const std::string_view uri : {
           "data:,bad%",
           "data:;base64,Zg%2",
           "data:;base64,Zg=",
           "data:;base64,Zh==", // Nonzero unused bits.
           "data:;base64,Zm9=", // Nonzero unused bits.
           "data:;base64,Zg%00==",
       })
    ExpectMetadataFails(uri);
}

TEST(TestDataFile, MetadataQueriesLargePayload)
{
  std::string uri{"data:,"};
  uri.reserve(uri.size() + MAX_DECODED_DATA_SIZE);
  uri.append(MAX_DECODED_DATA_SIZE, 'x');

  const CURL url{uri};
  XFILE::CDataFile file;
  EXPECT_TRUE(file.Exists(url));

  struct __stat64 buffer
  {
  };
  ASSERT_EQ(0, file.Stat(url, &buffer));
  EXPECT_EQ(static_cast<int64_t>(MAX_DECODED_DATA_SIZE), buffer.st_size);
  EXPECT_EQ(_S_IFREG, buffer.st_mode & _S_IFREG);
}

TEST(TestDataFile, IsReadOnly)
{
  const std::string uri{"data:text/plain,hello"};

  XFILE::CFile file;
  ASSERT_TRUE(file.Open(uri));
  const char value{'!'};
  EXPECT_EQ(-1, file.Write(&value, sizeof(value)));
  EXPECT_EQ(-1, file.Truncate(0));

  XFILE::CFile writableFile;
  EXPECT_FALSE(writableFile.OpenForWrite(uri, true));
  EXPECT_FALSE(XFILE::CFile::Delete(uri));
  EXPECT_FALSE(XFILE::CFile::Rename(uri, "data:text/plain,replacement"));
}
