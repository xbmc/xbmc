/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/Digest.h"

#include <gtest/gtest.h>

using KODI::UTILITY::CDigest;
using KODI::UTILITY::TypedDigest;

TEST(TestDigest, Digest_Empty)
{
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::MD5, "").c_str(), "d41d8cd98f00b204e9800998ecf8427e");
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::MD5, nullptr, 0).c_str(), "d41d8cd98f00b204e9800998ecf8427e");
  {
    CDigest digest{CDigest::Type::MD5};
    EXPECT_STREQ(digest.Finalize().c_str(), "d41d8cd98f00b204e9800998ecf8427e");
  }
  {
    CDigest digest{CDigest::Type::MD5};
    digest.Update("");
    digest.Update(nullptr, 0);
    EXPECT_STREQ(digest.Finalize().c_str(), "d41d8cd98f00b204e9800998ecf8427e");
  }
}

TEST(TestDigest, Digest_Basic)
{
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::MD5, "asdf").c_str(), "912ec803b2ce49e4a541068d495ab570");
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::MD5, "asdf", 4).c_str(), "912ec803b2ce49e4a541068d495ab570");
  {
    CDigest digest{CDigest::Type::MD5};
    digest.Update("as");
    digest.Update("df", 2);
    EXPECT_STREQ(digest.Finalize().c_str(), "912ec803b2ce49e4a541068d495ab570");
  }
}

TEST(TestDigest, Digest_SHA1)
{
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::SHA1, "").c_str(), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::SHA1, "asdf").c_str(), "3da541559918a808c2402bba5012f6c60b27661c");
}

TEST(TestDigest, Digest_SHA256)
{
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::SHA256, "").c_str(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::SHA256, "asdf").c_str(), "f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b");
}

TEST(TestDigest, Digest_SHA512)
{
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::SHA512, "").c_str(), "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
  EXPECT_STREQ(CDigest::Calculate(CDigest::Type::SHA512, "asdf").c_str(), "401b09eab3c013d4ca54922bb802bec8fd5318192b0a75f201d8b3727429080fb337591abd3e44453b954555b7a0812e1081c39b740293f765eae731f5a65ed1");
}

TEST(TestDigest, TypedDigest_Empty)
{
  TypedDigest t1, t2;
  EXPECT_EQ(t1, t2);
  EXPECT_EQ(t1.type, CDigest::Type::INVALID);
  EXPECT_EQ(t1.value, "");
  EXPECT_TRUE(t1.Empty());
  t1.type = CDigest::Type::SHA1;
  EXPECT_TRUE(t1.Empty());
}

TEST(TestDigest, TypedDigest_SameType)
{
  TypedDigest t1{CDigest::Type::SHA1, "da39a3ee5e6b4b0d3255bfef95601890afd80709"};
  TypedDigest t2{CDigest::Type::SHA1, "da39a3ee5e6b4b0d3255bfef95601890afd80708"};
  EXPECT_NE(t1, t2);
  EXPECT_FALSE(t1.Empty());
}

TEST(TestDigest, TypedDigest_CompareCase)
{
  TypedDigest t1{CDigest::Type::SHA1, "da39a3ee5e6b4b0d3255bfef95601890afd80708"};
  TypedDigest t2{CDigest::Type::SHA1, "da39A3EE5e6b4b0d3255bfef95601890afd80708"};
  EXPECT_EQ(t1, t2);
}

TEST(TestDigest, TypedDigest_DifferingType)
{
  TypedDigest t1{CDigest::Type::SHA1, "da39a3ee5e6b4b0d3255bfef95601890afd80709"};
  TypedDigest t2{CDigest::Type::SHA256, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
  // Silence "unused expression" warning
  bool a;
  EXPECT_THROW(a = (t1 == t2), std::logic_error);
  // Silence "unused variable" warning
  (void)a;
  EXPECT_THROW(a = (t1 != t2), std::logic_error);
  (void)a;
}
