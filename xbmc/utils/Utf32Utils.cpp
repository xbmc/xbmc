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

#include <cassert>
#include <locale>
#include <algorithm>
#include "Utf32Utils.h"
#include "utils/CharsetConverter.h"

const CUtf32Utils::digitsMap CUtf32Utils::m_digitsMap(digitsMapFiller());
const CUtf32Utils::charcharMap CUtf32Utils::m_foldSimpleCharsMap(foldSimpleCharsMapFiller());
const CUtf32Utils::charstrMap CUtf32Utils::m_foldFullCharsMap(foldFullCharsMapFiller());


inline bool CUtf32Utils::IsDigit(char32_t chr)
{
  double val;
  return GetDigitValue(chr, val);
}

bool CUtf32Utils::GetDigitValue(char32_t chr, double& resultValue)
{
  digitsMap::const_iterator it = m_digitsMap.find(chr);
  if (it != m_digitsMap.end())
  {
    resultValue = it->second;
    return true;
  }
  return false;
}

int CUtf32Utils::NaturalCompare(const std::u32string& left, const std::u32string right)
{
  const char32_t* const leftC = left.c_str();
  const char32_t* const rightC = right.c_str();
  const size_t leftLen = left.length();
  const size_t rightLen = right.length();
  const size_t shortestLen = std::min(leftLen, rightLen);
  
  const std::collate<wchar_t>& cl = std::use_facet< std::collate<wchar_t> >(std::locale());

  for (size_t pos = 0; pos < shortestLen; pos++)
  {
    if (leftC[pos] != rightC[pos])
    {
      double valL, valR;
      if (GetDigitValue(leftC[pos], valL) && GetDigitValue(rightC[pos], valR))
      {
        // FIXME: improve comparison of non-Arabic (and non-Arabic-Indic) numbers
        size_t posL = pos, posR = pos;
        bool leftIsDigit = true;  // initial value
        bool rightIsDigit = true; // initial value

        // check for non-zero digits before current position "pos"
        size_t lookBackPos = pos;
        while (lookBackPos-- > 0)
        {
          double digitVal;
          // "left" and "right" are same before "pos", no need to check them both
          if (!GetDigitValue(leftC[lookBackPos], digitVal))
          { // no digits or only zero digits before "pos"
            // skip all leading zero digits in "left"
            while (leftIsDigit && valL == 0) 
              leftIsDigit = GetDigitValue(leftC[++posL], valL);
            // skip all leading zero digits in "right"
            while (rightIsDigit && valR == 0)
              rightIsDigit = GetDigitValue(rightC[++posR], valR);
            break;
          }
          if (digitVal != 0)
            break; // non-zero digit is found before "pos", no need to skip leading zeros
        }

        const size_t numberOfLeadZerosL = posL - pos; // "pos" is pointing to first different (by code) digit
        const size_t numberOfLeadZerosR = posR - pos; // "pos" is pointing to first different (by code) digit

        // check if "left" and "right" have any digits after leading zeros
        if (leftIsDigit && rightIsDigit)
        {
          double numCompare = 0; // compare fist non-zero different digits
          do
          {
            if (numCompare == 0)
              numCompare = valL - valR;
            leftIsDigit = GetDigitValue(leftC[++posL], valL);
            rightIsDigit = GetDigitValue(rightC[++posR], valR);
          } while (leftIsDigit && rightIsDigit);

          if (leftIsDigit)
            return 1;   // "left" has more digits (excluding leading zeros), assuming that "left" is greater than "right"
          else if (rightIsDigit)
            return -1;  // "right" has more digits (excluding leading zeros), assuming that "right" is greater than "left"
          
          // "left" and "right" have equal number of digits (excluding leading zeros),
          if (numCompare != 0)
            return numCompare > 0 ? 1 : -1; // assuming that result is determined by first non-zero different digits

          // excluding leading zeros, "left" and "right" have equal number of digits, and all digits have same values
        }
        else if (leftIsDigit)
          return valL > 0 ? 1 : -1; // "right" has only zeros and "left" contains non-zero
        else if (rightIsDigit)
          return valR > 0 ? -1 : 1; // "left" has only zeros and "right" contains non-zero

        if (numberOfLeadZerosL > numberOfLeadZerosR)
          return -1; // "left" has more leading zeros, assume "left" is lower
        else if (numberOfLeadZerosL < numberOfLeadZerosR)
          return -1; // "right" has more leading zeros, assume "right" is lower

        // "left" and "right" have equal number of digits, and all digits have same values
        assert(posL == posR);
      }
      const std::wstring leftWChr(g_charsetConverter.charUtf32toWstr(leftC[pos]));   // wstring representation of "left[pos]"
      const std::wstring rightWChr(g_charsetConverter.charUtf32toWstr(rightC[pos])); // wstring representation of "right[pos]"
      const wchar_t* const leftWChrC = leftWChr.c_str();
      const wchar_t* const rightWChrC = rightWChr.c_str();
      const int compareRsl = cl.compare(leftWChrC, leftWChrC + leftWChr.length(), rightWChrC, rightWChrC + rightWChr.length());
      if (compareRsl != 0)
        return compareRsl;
      else
      { // bad conversion to wstring?
        // fallback: compare characters by code
        if (leftC[pos] > rightC[pos])
          return 1;
        else if (leftC[pos] < rightC[pos])
          return -1;
      }
    }
  }

  if (leftLen > rightLen)
    return 1;
  else if (leftLen < rightLen)
    return -1;

  return 0; // strings are equal
}

