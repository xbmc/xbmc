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

class StringUtils
{

public:
  static int SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results);
  static int FindNumber(const CStdString& strInput, const CStdString &strFind);
  static int AlphaNumericCompare(const char *left, const char *right);
  static int TimeStringToInt(const CStdString &timeString);
};

#endif
