//-----------------------------------------------------------------------
//
//  File:      StringUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to use J O'Leary's CStdString class by kraqh3d
//
//------------------------------------------------------------------------


#include "stdafx.h"
#include "StringUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int StringUtils::SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results)
{
  int iPos = 0;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  //CArray positions;
  vector<unsigned int> positions;

  newPos = input.Find (delimiter, 0);

  if ( newPos < 0 )
  {
    results.push_back(input);
    return 1;
  }

  int numFound = 1;

  while ( newPos > iPos )
  {
    numFound++;
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.Find (delimiter, iPos + sizeS2 + 1);
  }

  for ( unsigned int i = 0; i <= positions.size(); i++ )
  {
    CStdString s;
    if ( i == 0 )
      s = input.Mid( i, positions[i] );
    else
    {
      int offset = positions[i - 1] + sizeS2;
      if ( offset < isize )
      {
        if ( i == positions.size() )
          s = input.Mid(offset);
        else if ( i > 0 )
          s = input.Mid( positions[i - 1] + sizeS2,
                         positions[i] - positions[i - 1] - sizeS2 );
      }
    }
    if ( s.GetLength() > 0 )
      results.push_back(s);
  }
  return numFound;
}

// returns the number of occurences of strFind in strInput.
int StringUtils::FindNumber(const CStdString& strInput, const CStdString &strFind)
{
  int pos = strInput.Find(strFind, 0);
  int numfound = 0;
  while (pos > 0)
  {
    numfound++;
    pos = strInput.Find(strFind, pos + 1);
  }
  return numfound;
}

// Compares separately the numeric and alphabetic parts of a string.
// returns true if left > right
bool StringUtils::AlphaNumericCompare(const char *left, const char *right)
{
  char *l = (char *)left;
  char *r = (char *)right;
  char *ld, *rd;
  char lc, rc;
  unsigned int lnum, rnum;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= '0' && *l <= '9' && *r >= '0' && *r <= '9')
    {
      ld = l;
      lnum = 0;
      while (*ld >= '0' && *ld <= '9')
      {
        lnum *= 10;
        lnum += *ld++ - '0';
      }
      rd = r;
      rnum = 0;
      while (*rd >= '0' && *rd <= '9')
      {
        rnum *= 10;
        rnum += *rd++ - '0';
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return lnum < rnum;
      }
      l = ld;
      r = rd;
      continue;
    }
    // do case less comparison
    lc = *l;
    if (lc >= 'A' && lc <= 'Z')
      lc += 'a'-'A';
    rc = *r;
    if (rc >= 'A' && rc <= 'Z')
      rc += 'a'-'A';
    // ok, do a normal comparison.  Add special case stuff (eg '(' characters)) in here later
    if (lc  != rc)
    {
      return lc < rc;
    }
    l++; r++;
  }
  return false; // files are the same
}
