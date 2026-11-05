/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/URI.h"

#include <string>
#include <utility>

#include <gtest/gtest.h>

TEST(TestURI, LosslessComponents)
{
  const std::string source = "CuStOm://UsEr:p%61ss@HoSt:42//a:%2f?x=1&x=2+3&&flag#F%2f";
  const CURI uri(source);
  EXPECT_TRUE(uri.IsValid());
  EXPECT_EQ(source, uri.Get());
  EXPECT_EQ("CuStOm", uri.GetRawScheme());
  EXPECT_EQ("UsEr:p%61ss@HoSt:42", uri.GetRawAuthority());
  EXPECT_EQ("//a:%2f", uri.GetRawPath());
  EXPECT_EQ("x=1&x=2+3&&flag", uri.GetRawQuery());
  EXPECT_EQ("F%2f", uri.GetRawFragment());
  EXPECT_EQ("//UsEr:p%61ss@HoSt:42//a:%2f?x=1&x=2+3&&flag", uri.GetSchemeSpecificPart());
  EXPECT_EQ(uri.Get().data() + 36, uri.GetRawQuery().data());
  EXPECT_EQ("CuStOm://UsEr:p%61ss@HoSt:42", uri.GetPathPrefix());
  EXPECT_EQ("CuStOm://UsEr:p%61ss@HoSt:42//a:%2f", uri.GetWithoutQueryAndFragment());
}

TEST(TestURI, EmptyDelimiters)
{
  for (const std::string source : {"a:", "a:?", "a:#", "a:?#", "a://", "a://?#"})
  {
    SCOPED_TRACE(source);
    const CURI uri(source);
    EXPECT_TRUE(uri.IsValid());
    EXPECT_EQ(source, uri.Get());
    EXPECT_EQ(source.find("//") != std::string::npos, uri.HasAuthority());
    EXPECT_EQ(source.find('?') != std::string::npos, uri.HasQuery());
    EXPECT_EQ(source.find('#') != std::string::npos, uri.HasFragment());
    EXPECT_TRUE(uri.GetRawQuery().empty());
    EXPECT_TRUE(uri.GetRawFragment().empty());
  }
}

TEST(TestURI, RecognizableMalformedCandidates)
{
  CURI uri;
  for (const std::string source : {"data:,bad%?q#f", "custom://bad host/path?x=1#", "urn:a|b"})
  {
    SCOPED_TRACE(source);
    EXPECT_FALSE(uri.Parse(source));
    EXPECT_FALSE(uri.IsValid());
    EXPECT_EQ(source, uri.Get());
    EXPECT_FALSE(uri.GetRawScheme().empty());
    EXPECT_FALSE(uri.SetPath("/fixed"));
    EXPECT_EQ(source, uri.Get());
  }
  EXPECT_FALSE(uri.Parse("./native:filename"));
  EXPECT_TRUE(uri.Get().empty());
  EXPECT_FALSE(uri.HasQuery());
  EXPECT_FALSE(uri.SetRawQuery("x=1"));
}

TEST(TestURI, PathMutationPreservesComponents)
{
  for (const std::string path : {"", "/", "//a", "///a:/%2f/%3F%23", "/much/longer/path"})
  {
    SCOPED_TRACE(path);
    CURI uri("CuStOm://u:p@host/original?x=1&x=2+%2f#part");
    ASSERT_TRUE(uri.SetPath(path));
    EXPECT_EQ("CuStOm://u:p@host" + path + "?x=1&x=2+%2f#part", uri.Get());
    EXPECT_EQ(path, uri.GetRawPath());
    EXPECT_EQ("u:p@host", uri.GetRawAuthority());
    EXPECT_EQ("x=1&x=2+%2f", uri.GetRawQuery());
    EXPECT_EQ("part", uri.GetRawFragment());
    ASSERT_TRUE(uri.SetPath("/b"));
    EXPECT_EQ("CuStOm://u:p@host/b?x=1&x=2+%2f#part", uri.Get());
  }
  CURI opaque("urn:namespace:identifier?#");
  ASSERT_TRUE(opaque.SetPath("other:identifier"));
  EXPECT_EQ("urn:other:identifier?#", opaque.Get());
  EXPECT_TRUE(opaque.HasQuery());
  EXPECT_TRUE(opaque.HasFragment());
  EXPECT_FALSE(opaque.HasAuthority());
}

TEST(TestURI, InvalidMutationsAreTransactional)
{
  for (const std::string source : {"file:/a?x=1#f", "custom://host/a?x=1#f"})
  {
    CURI uri(source);
    for (const std::string path : {"/b?", "/b#", "/bad%", "/bad%GG", "/bad path", "/bad\\path"})
    {
      SCOPED_TRACE(path);
      EXPECT_FALSE(uri.SetPath(path));
      EXPECT_EQ(source, uri.Get());
      EXPECT_EQ("/a", uri.GetRawPath());
      EXPECT_EQ("x=1", uri.GetRawQuery());
      EXPECT_EQ("f", uri.GetRawFragment());
      EXPECT_TRUE(uri.IsValid());
    }
    for (const std::string query : {"x=#f", "bad%", "bad space", "bad\\query"})
    {
      EXPECT_FALSE(uri.SetRawQuery(query));
      EXPECT_EQ(source, uri.Get());
    }
    EXPECT_FALSE(uri.SetPath(uri.HasAuthority() ? "relative" : "//authority"));
    EXPECT_EQ(source, uri.Get());
  }
}

TEST(TestURI, QueryMutationKeepsOffsetsAndValidityCurrent)
{
  CURI uri("SCHEME:/a?bad%#fragment");
  EXPECT_FALSE(uri.IsValid());
  ASSERT_TRUE(uri.SetRawQuery("x=1&x=2+%2f/?"));
  EXPECT_TRUE(uri.IsValid());
  EXPECT_EQ("SCHEME:/a?x=1&x=2+%2f/?#fragment", uri.Get());
  EXPECT_EQ("fragment", uri.GetRawFragment());
  EXPECT_EQ("/a", uri.GetRawPath());
  ASSERT_TRUE(uri.SetRawQuery(""));
  EXPECT_EQ("SCHEME:/a?#fragment", uri.Get());
  EXPECT_TRUE(uri.HasQuery());
  ASSERT_TRUE(uri.SetRawQuery(std::nullopt));
  EXPECT_EQ("SCHEME:/a#fragment", uri.Get());
  EXPECT_FALSE(uri.HasQuery());
  EXPECT_EQ("fragment", uri.GetRawFragment());
  ASSERT_TRUE(uri.SetRawQuery("new"));
  EXPECT_EQ("SCHEME:/a?new#fragment", uri.Get());
  CURI malformed("data:bad%#f");
  ASSERT_TRUE(malformed.SetRawQuery("new"));
  EXPECT_FALSE(malformed.IsValid());
  EXPECT_EQ("data:bad%?new#f", malformed.Get());
}

TEST(TestURI, CopiesAndMovesOwnTheirComponents)
{
  for (const std::string source :
       {"a:b?q#f", "custom://user:pass@host/a/long/path?x=1&x=2#fragment"})
  {
    SCOPED_TRACE(source);
    CURI original(source);
    CURI copy(original);
    original.Parse("other:changed");
    EXPECT_EQ(source, copy.Get());
    CURI moved(std::move(copy));
    EXPECT_EQ(source, moved.Get());
    EXPECT_TRUE(copy.Get().empty());
    EXPECT_TRUE(copy.GetRawPath().empty());
    EXPECT_FALSE(copy.HasQuery());
    EXPECT_FALSE(copy.IsValid());
    copy = moved;
    moved.Parse("other:value");
    EXPECT_EQ(source, copy.Get());
    moved = std::move(copy);
    ASSERT_TRUE(moved.SetRawQuery("replacement"));
    EXPECT_EQ("replacement", moved.GetRawQuery());
    EXPECT_EQ(source.ends_with("#f") ? "f" : "fragment", moved.GetRawFragment());
    EXPECT_TRUE(copy.GetSchemeSpecificPart().empty());
    EXPECT_FALSE(copy.SetPath("/new"));
  }
}
