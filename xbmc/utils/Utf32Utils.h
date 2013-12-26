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

#include <map>
#include "utils/uXstrings.h"



class CUtf32Utils
{
public:
  static bool IsDigit(char32_t chr);
  static bool GetDigitValue(char32_t chr, double& resultValue);

  /**
   * Compare two strings with respecting natural order for numbers
   * "a0" < "a2" < "a03" < "a10" < "a0100" < "b4"
   * @param left string to compare
   * @param right string to compare
   * @return positive number if left is greater then right, negative number if left is smaller then right
   *         and zero if strings are equal
   */
  static int NaturalCompare(const std::u32string& left, const std::u32string right);

  /**
   * Fold string for caseless comparison
   * @warning Use result only for string matching, do not use result for displaying for human
   * @param str string to process
   * @return case-folded string
   */
  static std::u32string FoldCase(const std::u32string& str);
  /**
   * Fold unicode character for caseless comparison
   * @warning Use result only for character matching, do not use result for displaying for human
   * @param chr character to process
   * @return case-folded character in string (possibly converted so several characters)
   */
  static std::u32string FoldCase(const char32_t chr);

private:
  typedef std::map<const char32_t, const double> digitsMap;
  typedef digitsMap::value_type digitsMapElement;

  static const digitsMap m_digitsMap;
  static digitsMap digitsMapFiller(void); // implemented in Utf32Utils-data.cpp

  typedef std::map<const char32_t, const char32_t> charcharMap;
  typedef charcharMap::value_type charcharMapElement;

  static const charcharMap m_foldSimpleCharsMap;
  static charcharMap foldSimpleCharsMapFiller(void); // implemented in Utf32Utils-data.cpp

  struct strWithLen
  {
     const char32_t str[3];
     const size_t len;
  };
  typedef std::map<const char32_t, const strWithLen> charstrMap;
  typedef charstrMap::value_type charstrMapElement;

  static const charstrMap m_foldFullCharsMap;
  static charstrMap foldFullCharsMapFiller(void);

  static bool m_useTurkicCaseFolding;
};

