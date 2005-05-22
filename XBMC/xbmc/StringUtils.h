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

typedef vector<CStdString> CStdStringArray;

class StringUtils
{

public:
  static int StringUtils::SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results);
  static int StringUtils::FindNumber(const CStdString& strInput, const CStdString &strFind);
  static bool StringUtils::AlphaNumericCompare(const char *left, const char *right);
};

#endif
