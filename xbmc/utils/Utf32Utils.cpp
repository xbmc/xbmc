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

#include "Utf32Utils.h"

const CUtf32Utils::digitsMap CUtf32Utils::m_digitsMap(digitsMapFiller());

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

