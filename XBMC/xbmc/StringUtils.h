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

#include "..\guilib\StdString.h"
#include <vector>

using namespace std;

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
  static int SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results);
  static int FindNumber(const CStdString& strInput, const CStdString &strFind);
  static int AlphaNumericCompare(const char *left, const char *right);
  static long TimeStringToSeconds(const CStdString &timeString);
  static void RemoveCRLF(CStdString& strLine);
  static void SecondsToTimeString( long lSeconds, CStdString& strHMS, TIME_FORMAT format = TIME_FORMAT_GUESS);
  static bool IsNaturalNumber(const CStdString& str);
  static CStdString StringUtils::SizeToString(__int64 size);
  static const CStdString EmptyString;
  static bool FindWords(const char *str, const char *wordLowerCase);
  static int FindEndBracket(const CStdString &str, char opener, char closer, int startPos = 0);
};

#endif
