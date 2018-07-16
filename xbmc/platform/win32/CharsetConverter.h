/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
