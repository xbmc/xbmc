/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>

#include "CharsetConverter.h"
namespace KODI
{
namespace PLATFORM
{
namespace WINDOWS
{
std::string FromW(const wchar_t* str, size_t length)
{
  int result = WideCharToMultiByte(CP_UTF8, MB_ERR_INVALID_CHARS, str, length, nullptr, 0, nullptr, nullptr);
  if (result == 0)
    return std::string();

  auto newStr = std::make_unique<char[]>(result);
  result = WideCharToMultiByte(CP_UTF8, MB_ERR_INVALID_CHARS, str, length, newStr.get(), result, nullptr, nullptr);
  if (result == 0)
    return std::string();

  return std::string(newStr.get(), result);
}

std::string FromW(const std::wstring& str)
{
  return FromW(str.c_str(), str.length());
}

std::wstring ToW(const char* str, size_t length)
{
  int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, length, nullptr, 0);
  if (result == 0)
    return std::wstring();

  auto newStr = std::make_unique<wchar_t[]>(result);
  result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, length, newStr.get(), result);

  if (result == 0)
    return std::wstring();

  return std::wstring(newStr.get(), result);
}

std::wstring ToW(const std::string& str)
{
  return ToW(str.c_str(), str.length());
}

}
}
}
