#pragma once

/*
 *      Copyright (C) 2013 Team XBMC
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
