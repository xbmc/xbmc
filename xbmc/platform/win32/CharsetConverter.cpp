/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CharsetConverter.h"

#include <cassert>
#include <limits>

#include <Windows.h>

namespace KODI
{
namespace PLATFORM
{
namespace WINDOWS
{
std::string FromW(const wchar_t* str, size_t length)
{
  std::string newStr;

  if (length == 0 || length > std::numeric_limits<int>::max())
    return newStr;

  const int result1 =
      WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str, length, nullptr, 0, nullptr, nullptr);
  if (result1 == 0)
    return newStr;

  newStr.resize(result1, '\0');
  const int result2 = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str, length, newStr.data(),
                                          result1, nullptr, nullptr);
  assert(result1 == result2);

  if (result1 != result2)
  {
    if (result2 < result1) // less data written than expected is recoverable though still a problem
      newStr.resize(result2, '\0');
    else
      newStr.clear();
  }

  return newStr;
}

std::string FromW(std::wstring_view str)
{
  return FromW(str.data(), str.length());
}

std::wstring ToW(const char* str, size_t length)
{
  std::wstring newStr;

  if (length == 0 || length > std::numeric_limits<int>::max())
    return newStr;

  const int result1 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, length, nullptr, 0);
  if (result1 == 0)
    return newStr;

  newStr.resize(result1, L'\0');
  const int result2 =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, length, newStr.data(), result1);
  assert(result1 == result2);

  if (result1 != result2)
  {
    if (result2 < result1) // less data written than expected is recoverable though still a problem
      newStr.resize(result2, L'\0');
    else
      newStr.clear();
  }

  return newStr;
}

std::wstring ToW(std::string_view str)
{
  return ToW(str.data(), str.length());
}

}
}
}
