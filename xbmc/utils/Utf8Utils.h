/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CUtf8Utils
{
public:
  enum utf8CheckResult
  {
    plainAscii = -1, // only US-ASCII characters (valid for UTF-8 too)
    hiAscii    =  0, // non-UTF-8 sequence with high ASCII characters
                     // (possible single-byte national encoding like WINDOWS-1251, multi-byte encoding like UTF-32 or invalid UTF-8)
    utf8string =  1  // valid UTF-8 sequences, but not US-ASCII only
  };

  /**
   * Check given string for valid UTF-8 sequences
   * @param str string to check
   * @return result of check, "plainAscii" for empty string
   */
  static utf8CheckResult checkStrForUtf8(const std::string& str);

  static inline bool isValidUtf8(const std::string& str)
  {
    return checkStrForUtf8(str) != hiAscii;
  }

  static size_t FindValidUtf8Char(const std::string& str, const size_t startPos = 0);
  static size_t RFindValidUtf8Char(const std::string& str, const size_t startPos);

  static size_t SizeOfUtf8Char(const std::string& str, const size_t charStart = 0);
private:
  static size_t SizeOfUtf8Char(const char* const str);
};
