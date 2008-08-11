/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
//-----------------------------------------------------------------------
//
//  File:      StringUtils.h
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to support J O'Leary's CStdString class by kraqh3d
//
//------------------------------------------------------------------------

#ifndef __STRINGUTILS_H_
#define __STRINGUTILS_H_

#include "StdString.h"
#include <vector>

typedef std::vector<CStdString> CStdStringArray;

enum TIME_FORMAT { TIME_FORMAT_GUESS = 0,
                   TIME_FORMAT_SS,
                   TIME_FORMAT_MM,
                   TIME_FORMAT_MM_SS,
                   TIME_FORMAT_HH,
                   TIME_FORMAT_HH_SS, // not particularly useful, but included so that they can be bit-tested
                   TIME_FORMAT_HH_MM,
                   TIME_FORMAT_HH_MM_SS };

class StringUtils
{
public:
  static void JoinString(const CStdStringArray &strings, const CStdString& delimiter, CStdString& result);
  static int SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings = 0);
  static int FindNumber(const CStdString& strInput, const CStdString &strFind);
  static int AlphaNumericCompare(const char *left, const char *right);
  static long TimeStringToSeconds(const CStdString &timeString);
  static void RemoveCRLF(CStdString& strLine);
  static void SecondsToTimeString( long lSeconds, CStdString& strHMS, TIME_FORMAT format = TIME_FORMAT_GUESS);
  static bool IsNaturalNumber(const CStdString& str);
  static CStdString SizeToString(__int64 size);
  static const CStdString EmptyString;
  static size_t FindWords(const char *str, const char *wordLowerCase);
  static int FindEndBracket(const CStdString &str, char opener, char closer, int startPos = 0);
  static int DateStringToYYYYMMDD(const CStdString &dateString);
  static void WordToDigits(CStdString &word);
};

#endif
