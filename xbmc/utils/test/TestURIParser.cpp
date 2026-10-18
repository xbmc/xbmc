/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/URIParser.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <gtest/gtest.h>

namespace
{
std::string Recompose(std::string_view input, const URIParser::Components& parts)
{
  std::string result;
  if (parts.scheme)
  {
    result += parts.scheme->Get(input);
    result += ':';
  }
  if (parts.authority)
  {
    result += "//";
    result += parts.authority->Get(input);
  }
  result += parts.path.Get(input);
  if (parts.query)
  {
    result += '?';
    result += parts.query->Get(input);
  }
  if (parts.fragment)
  {
    result += '#';
    result += parts.fragment->Get(input);
  }
  return result;
}

struct ReferenceCase
{
  std::string_view input;
  std::optional<std::string_view> scheme;
  std::optional<std::string_view> authority;
  std::string_view path;
  std::optional<std::string_view> query;
  std::optional<std::string_view> fragment;
};

void ExpectComponent(std::string_view input,
                     const std::optional<URIParser::Range>& range,
                     const std::optional<std::string_view>& expected)
{
  ASSERT_EQ(range.has_value(), expected.has_value());
  if (range)
  {
    EXPECT_EQ(range->Get(input), *expected);
  }
}
} // namespace

class TestURIParserReference : public testing::TestWithParam<ReferenceCase>
{
};

TEST_P(TestURIParserReference, PreservesComponentsAndSpelling)
{
  const auto& test = GetParam();
  const auto parts = URIParser::ParseReference(test.input);
  ASSERT_TRUE(parts.has_value());
  ExpectComponent(test.input, parts->scheme, test.scheme);
  ExpectComponent(test.input, parts->authority, test.authority);
  EXPECT_EQ(parts->path.Get(test.input), test.path);
  ExpectComponent(test.input, parts->query, test.query);
  ExpectComponent(test.input, parts->fragment, test.fragment);
  EXPECT_EQ(Recompose(test.input, *parts), test.input);
  EXPECT_EQ(URIParser::IsURI(test.input), test.scheme.has_value());
}

// RFC 3986 sections 4.1-4.3 distinguish URI, relative-ref, and absolute-URI;
// the URI production permits fragments. Section 5.3 preserves empty delimiters.
// https://www.rfc-editor.org/rfc/rfc3986.html#section-4.1
// https://www.rfc-editor.org/rfc/rfc3986.html#section-5.3
INSTANTIATE_TEST_SUITE_P(
    GenericSyntax,
    TestURIParserReference,
    testing::Values(
        ReferenceCase{"", {}, {}, "", {}, {}},
        ReferenceCase{"path", {}, {}, "path", {}, {}},
        ReferenceCase{"../a/b", {}, {}, "../a/b", {}, {}},
        ReferenceCase{"./notes:2026", {}, {}, "./notes:2026", {}, {}},
        ReferenceCase{"a/b:c", {}, {}, "a/b:c", {}, {}},
        ReferenceCase{"/tmp/a://b", {}, {}, "/tmp/a://b", {}, {}},
        ReferenceCase{"?query", {}, {}, "", "query", {}},
        ReferenceCase{"#fragment", {}, {}, "", {}, "fragment"},
        ReferenceCase{"?", {}, {}, "", "", {}},
        ReferenceCase{"#", {}, {}, "", {}, ""},
        ReferenceCase{"?#", {}, {}, "", "", ""},
        ReferenceCase{"//host/path", {}, "host", "/path", {}, {}},
        ReferenceCase{"//", {}, "", "", {}, {}},
        ReferenceCase{"///path", {}, "", "/path", {}, {}},
        ReferenceCase{"scheme:", "scheme", {}, "", {}, {}},
        ReferenceCase{"scheme:?", "scheme", {}, "", "", {}},
        ReferenceCase{"scheme:#", "scheme", {}, "", {}, ""},
        ReferenceCase{"scheme:?#", "scheme", {}, "", "", ""},
        ReferenceCase{"scheme://?#", "scheme", "", "", "", ""},
        ReferenceCase{"scheme:/path", "scheme", {}, "/path", {}, {}},
        ReferenceCase{"scheme:path", "scheme", {}, "path", {}, {}},
        ReferenceCase{"file:/tmp/a", "file", {}, "/tmp/a", {}, {}},
        ReferenceCase{"file:///tmp/a", "file", "", "/tmp/a", {}, {}},
        ReferenceCase{"C:relative", "C", {}, "relative", {}, {}},
        ReferenceCase{"HTTP://user:p%40ss@[2001:db8::1]:80/a//b?x=+%2f&x=%2F#part?x", "HTTP",
                      "user:p%40ss@[2001:db8::1]:80", "/a//b", "x=+%2f&x=%2F", "part?x"},
        ReferenceCase{"DATA:text/plain,http://example.test/a//b?x=1#fragment",
                      "DATA",
                      {},
                      "text/plain,http://example.test/a//b",
                      "x=1",
                      "fragment"},
        ReferenceCase{"magnet:?xt=urn:btih:0123456789012345678901234567890123456789&tr=&tr=a+b",
                      "magnet",
                      {},
                      "",
                      "xt=urn:btih:0123456789012345678901234567890123456789&tr=&tr=a+b",
                      {}},
        ReferenceCase{"urn:example:a/b?+resolver?=query#part",
                      "urn",
                      {},
                      "example:a/b",
                      "+resolver?=query",
                      "part"},
        ReferenceCase{"example+v1:body", "example+v1", {}, "body", {}, {}}));

TEST(TestURIParser, RecognizesOnlyLeadingSchemeWithoutValidatingBody)
{
  for (const std::string_view input : {"a:", "Z:body", "HTTP://host", "example+v1.2-3:bad%",
                                       "data:,a|b", "data://[invalid", "data:body with spaces"})
  {
    SCOPED_TRACE(input);
    const auto scheme = URIParser::GetScheme(input);
    ASSERT_TRUE(scheme.has_value());
    EXPECT_EQ(*scheme, input.substr(0, input.find(':')));
  }

  for (const std::string_view input :
       {"", ":body", "1bad:body", "bad_scheme:body", "+bad:body", "./notes:2026", "/tmp/a://b",
        "path/a:body", "?a:b", "#a:b", "scheme", "scheme/path:body", "//host:123"})
  {
    SCOPED_TRACE(input);
    EXPECT_FALSE(URIParser::GetScheme(input));
  }
}

TEST(TestURIParser, ScansMalformedCandidatesWithoutLosingBytes)
{
  for (const std::string_view input :
       {"data:,bad%", "data:,a|b?x=#suffix", "DATA://[bad]/a b?x%#f%", "1bad:body", "scheme:a#b#c",
        "//host:port/path"})
  {
    SCOPED_TRACE(input);
    const auto parts = URIParser::ScanReference(input);
    EXPECT_EQ(Recompose(input, parts), input);
    EXPECT_FALSE(URIParser::ParseReference(input));
  }
  const std::string_view input = "data:,a|b?x=#suffix";
  const auto parts = URIParser::ScanReference(input);
  ASSERT_TRUE(parts.scheme);
  EXPECT_EQ(parts.scheme->Get(input), "data");
  EXPECT_EQ(parts.path.Get(input), ",a|b");
  ASSERT_TRUE(parts.query);
  EXPECT_EQ(parts.query->Get(input), "x=");
  ASSERT_TRUE(parts.fragment);
  EXPECT_EQ(parts.fragment->Get(input), "suffix");
}

class TestURIParserValid : public testing::TestWithParam<const char*>
{
};

TEST_P(TestURIParserValid, AcceptsGenericSyntax)
{
  EXPECT_TRUE(URIParser::ParseReference(GetParam()));
  EXPECT_TRUE(URIParser::IsURI(GetParam()));
}

// These validate generic syntax, including empty or unusable network locations;
// scheme-specific syntax and network availability are separate concerns.
// Dotted hostnames that do not match IPv4address can still match reg-name.
// https://www.rfc-editor.org/rfc/rfc3986.html#section-3.2.2
INSTANTIATE_TEST_SUITE_P(GenericSyntax,
                         TestURIParserValid,
                         testing::Values("x://",
                                         "x://@",
                                         "x://host:",
                                         "x://host:999999999999",
                                         "x://user:p%40ss@host/",
                                         "x://255.255.255.255/",
                                         "x://256.0.0.1/",
                                         "x://01.2.3.4/",
                                         "x://name%20here/",
                                         "x://[::]/",
                                         "x://[::1]/",
                                         "x://[1::]/",
                                         "x://[1:2:3:4:5:6:7:8]/",
                                         "x://[1:2:3:4:5:6:7::]/",
                                         "x://[::1:2:3:4:5:6:7]/",
                                         "x://[2001:db8::1]:/",
                                         "x://[::ffff:192.0.2.1]/",
                                         "x://[1:2:3:4:5:6:192.0.2.1]/",
                                         "x://[1:2:3:4:5::192.0.2.1]/",
                                         "x://[v1.a:b]/",
                                         "x://[VF.A!$&'()*+,;=:]/",
                                         "x:/",
                                         "x:////",
                                         "x:a/./../b",
                                         "x:%00%7f%FF",
                                         "x:!$&'()*+,;=:@/~_-.",
                                         "x:?a/b?c:d@e+f%20#x/y?z",
                                         "data://x",
                                         "mailto:user@example.test"));

class TestURIParserInvalid : public testing::TestWithParam<const char*>
{
};

TEST_P(TestURIParserInvalid, RejectsInvalidGenericSyntax)
{
  EXPECT_FALSE(URIParser::ParseReference(GetParam()));
  EXPECT_FALSE(URIParser::IsURI(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(GenericSyntax,
                         TestURIParserInvalid,
                         testing::Values(":path",
                                         "1scheme:path",
                                         "bad_scheme:path",
                                         "x:bad%",
                                         "x:bad%0",
                                         "x:bad%GG",
                                         "x:space here",
                                         "x:tab\there",
                                         "x:line\nbreak",
                                         "x:\x7f",
                                         "x:\xc3\xa9",
                                         "x:a\\b",
                                         "x:a|b",
                                         "x:a<b",
                                         "x:a>b",
                                         "x:a\"b",
                                         "x:a{b",
                                         "x:a}b",
                                         "x:a^b",
                                         "x:a`b",
                                         "x:a[b",
                                         "x:a]b",
                                         "x:?bad%",
                                         "x:#bad%",
                                         "x:#a#b",
                                         "x://user@@host",
                                         "x://host:port/",
                                         "x://host:-1/",
                                         "x://host:12:34/",
                                         "x://bad%host/",
                                         "x://[::1",
                                         "x://[]/",
                                         "x://[::1]suffix/",
                                         "x://[::1]:port/",
                                         "x://::1/",
                                         "x://[1:2:3:4:5:6:7]/",
                                         "x://[1:2:3:4:5:6:7:8:9]/",
                                         "x://[1:2:3:4:5:6:7:8::]/",
                                         "x://[1::2::3]/",
                                         "x://[:::]/",
                                         "x://[12345::]/",
                                         "x://[::g]/",
                                         "x://[::ffff:256.1.2.3]/",
                                         "x://[::ffff:01.2.3.4]/",
                                         "x://[192.0.2.1::]/",
                                         "x://[1:2:3:4:5:6::192.0.2.1]/",
                                         "x://[::ffff:1.2.3]/",
                                         "x://[v.a]/",
                                         "x://[v1.]/",
                                         "x://[v1.a%20b]/",
                                         "x://[vG.a]/",
                                         "x://[fe80::1%25eth0]/"));

TEST(TestURIParser, RejectsEmbeddedNul)
{
  const std::string input{"x:a\0b", 5};
  EXPECT_FALSE(URIParser::ParseReference(input));
  EXPECT_EQ(Recompose(input, URIParser::ScanReference(input)), input);
}

TEST(TestURIParser, RangesRemainUsableWithCopiedAndMovedStorage)
{
  std::string input = "scheme://authority/path?query#fragment";
  const auto parts = URIParser::ParseReference(input);
  ASSERT_TRUE(parts);
  std::string copy = input;
  input.clear();
  EXPECT_EQ(Recompose(copy, *parts), "scheme://authority/path?query#fragment");
  std::string moved = std::move(copy);
  moved.reserve(1024);
  EXPECT_EQ(Recompose(moved, *parts), "scheme://authority/path?query#fragment");
}
