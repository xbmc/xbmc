#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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

#include <vector>
#include <stdint.h>
#include <string>

#include "XBDateTime.h"
#include "utils/StdString.h"

class StringUtils
{
public:
  /*! \brief Get a formatted string similar to sprintf

  Beware that this does not support directly passing in
  std::string objects. You need to call c_str() to pass
  the const char* buffer representing the value of the
  std::string object.

  \param fmt Format of the resulting string
  \param ... variable number of value type arguments
  \return Formatted string
  */
  static std::string Format(const char *fmt, ...);
  static std::string FormatV(const char *fmt, va_list args);
  static void ToUpper(std::string &str);
  static void ToLower(std::string &str);
  static bool EqualsNoCase(const std::string &str1, const std::string &str2);
  static std::string Left(const std::string &str, size_t count);
  static std::string Mid(const std::string &str, size_t first, size_t count = std::string::npos);
  static std::string Right(const std::string &str, size_t count);
  static std::string& Trim(std::string &str);
  static std::string& TrimLeft(std::string &str);
  static std::string& TrimRight(std::string &str);
  static int Replace(std::string &str, char oldChar, char newChar);
  static int Replace(std::string &str, const std::string &oldStr, const std::string &newStr);
  static bool StartsWith(const std::string &str, const std::string &str2, bool useCase = false);
  static bool EndsWith(const std::string &str, const std::string &str2, bool useCase = false);

  static void JoinString(const CStdStringArray &strings, const CStdString& delimiter, CStdString& result);
  static CStdString JoinString(const CStdStringArray &strings, const CStdString& delimiter);
  static CStdString Join(const std::vector<std::string> &strings, const CStdString& delimiter);
  static int SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings = 0);
  static CStdStringArray SplitString(const CStdString& input, const CStdString& delimiter, unsigned int iMaxStrings = 0);
  static std::vector<std::string> Split(const CStdString& input, const CStdString& delimiter, unsigned int iMaxStrings = 0);
  static int FindNumber(const CStdString& strInput, const CStdString &strFind);
  static int64_t AlphaNumericCompare(const wchar_t *left, const wchar_t *right);
  static long TimeStringToSeconds(const CStdString &timeString);
  static void RemoveCRLF(CStdString& strLine);

  /*! \brief utf8 version of strlen - skips any non-starting bytes in the count, thus returning the number of utf8 characters
   \param s c-string to find the length of.
   \return the number of utf8 characters in the string.
   */
  static size_t utf8_strlen(const char *s);

  /*! \brief convert a time in seconds to a string based on the given time format
   \param seconds time in seconds
   \param format the format we want the time in.
   \return the formatted time
   \sa TIME_FORMAT
   */
  static CStdString SecondsToTimeString(long seconds, TIME_FORMAT format = TIME_FORMAT_GUESS);

  /*! \brief check whether a string is a natural number.
   Matches [ \t]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is a natural number, false otherwise.
   */
  static bool IsNaturalNumber(const CStdString& str);

  /*! \brief check whether a string is an integer.
   Matches [ \t]*[\-]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is an integer, false otherwise.
   */
  static bool IsInteger(const CStdString& str);
  static CStdString SizeToString(int64_t size);
  static const CStdString EmptyString;
  static size_t FindWords(const char *str, const char *wordLowerCase);
  static int FindEndBracket(const CStdString &str, char opener, char closer, int startPos = 0);
  static int DateStringToYYYYMMDD(const CStdString &dateString);
  static void WordToDigits(CStdString &word);
  static CStdString CreateUUID();
  static bool ValidateUUID(const CStdString &uuid); // NB only validates syntax
  static double CompareFuzzy(const CStdString &left, const CStdString &right);
  static int FindBestMatch(const CStdString &str, const CStdStringArray &strings, double &matchscore);

  static bool Test();
private:
  static CStdString m_lastUUID;
};
