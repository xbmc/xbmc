/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/win32/CharsetConverter.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace KODI::PLATFORM::WINDOWS;

TEST(TestWin32CharsetConverter, FromW)
{
  std::wstring ws;
  std::string expected;
  std::string result = FromW(ws);

  EXPECT_EQ(expected, result);

  ws = L"foo";
  result = FromW(ws);
  expected = "foo";

  EXPECT_EQ(expected, result);

  std::wstring_view wsv(L"foo");
  result = FromW(wsv);

  EXPECT_EQ(expected, result);

  wchar_t ca[] = L"foo";

  // substring - not-null terminated
  expected = "fo";
  result = FromW(ca, 2);
  EXPECT_EQ(expected, result);

  // calling the function that way is usually a mistake
  // including the null terminator converts it with the rest of the string
  result = FromW(ca, 4);
  using namespace std::string_literals;
  expected = "foo\0"s;
  EXPECT_EQ(expected, result);
}

TEST(TestWin32CharsetConverter, ToW)
{
  std::string s;
  std::wstring expected;
  std::wstring result = ToW(s);

  EXPECT_EQ(expected, result);

  s = "foo";
  result = ToW(s);
  expected = L"foo";

  EXPECT_EQ(expected, result);

  std::string_view sv("foo");
  result = ToW(sv);

  EXPECT_EQ(expected, result);

  char ca[] = "foo";

  // substring - not-null terminated
  expected = L"fo";
  result = ToW(ca, 2);
  EXPECT_EQ(expected, result);

  // calling the function that way is usually a mistake
  // including the null terminator converts it with the rest of the string
  result = ToW(ca, 4);
  using namespace std::string_literals;
  expected = L"foo\0"s;
  EXPECT_EQ(expected, result);
}
