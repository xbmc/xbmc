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

private:
  typedef std::map<const char32_t, const double> digitsMap;
  typedef digitsMap::value_type digitsMapElement;

  static const digitsMap m_digitsMap;
  static digitsMap digitsMapFiller(void); // implemented in Utf32Utils-data.cpp
};

