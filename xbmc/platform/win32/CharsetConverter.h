#pragma once

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

#include <string>

#if defined(TARGET_WINDOWS)
namespace KODI
{
namespace PLATFORM
{
namespace WINDOWS
{
/**
 * Convert UTF-16 to UTF-8 strings
 * Windows specific method to avoid initialization issues
 * and locking issues that are unique to Windows as API calls
 * expect UTF-16 strings
 * \param str[in] string to be converted
 * \param length[in] length in characters of the string
 * \returns utf8 string, empty string on failure
 */
std::string FromW(const wchar_t* str, size_t length);

/**
 * Convert UTF-16 to UTF-8 strings
 * Windows specific method to avoid initialization issues
 * and locking issues that are unique to Windows as API calls
 * expect UTF-16 strings
 * \param str[in] string to be converted
 * \returns utf8 string, empty string on failure
 */
std::string FromW(const std::wstring& str);

/**
 * Convert UTF-8 to UTF-16 strings
 * Windows specific method to avoid initialization issues
 * and locking issues that are unique to Windows as API calls
 * expect UTF-16 strings
 * \param str[in] string to be converted
 * \param length[in] length in characters of the string
 * \returns UTF-16 string, empty string on failure
 */
std::wstring ToW(const char* str, size_t length);

/**
 * Convert UTF-8 to UTF-16 strings
 * Windows specific method to avoid initialization issues
 * and locking issues that are unique to Windows as API calls
 * expect UTF-16 strings
 * \param str[in] string to be converted
 * \returns UTF-16 string, empty string on failure
 */
std::wstring ToW(const std::string& str);
}
}
}
#endif
