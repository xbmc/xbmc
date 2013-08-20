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


class CCharsetDetection
{
public:
  /**
   * Detect text encoding by Byte Order Mark
   * Multibyte encodings (UTF-16/32) always ends with explicit endianness (LE/BE)
   * @param content     pointer to text to analyze
   * @param contentLength       length of text
   * @return detected encoding or empty string if BOM not detected
   */
  static std::string GetBomEncoding(const char* const content, const size_t contentLength);
  /**
  * Detect text encoding by Byte Order Mark
  * Multibyte encodings (UTF-16/32) always ends with explicit endianness (LE/BE)
  * @param content     the text to analyze
  * @return detected encoding or empty string if BOM not detected
  */
  static inline std::string GetBomEncoding(const std::string& content, std::string& detectedEncoding)
  { return GetBomEncoding(content.c_str(), content.length()); }
private:
};
