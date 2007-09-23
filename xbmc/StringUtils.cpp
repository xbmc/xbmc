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
#include <math.h>

/* empty string for use in returns by ref */
const CStdString StringUtils::EmptyString = "";

void StringUtils::JoinString(const CStdStringArray &strings, const CStdString& delimiter, CStdString& result)
{
  result = "";
  for(CStdStringArray::const_iterator it = strings.begin(); it != strings.end(); it++ )
    result += (*it) + delimiter;

  if(result != "")
    result.Delete(result.size()-delimiter.size(), delimiter.size());
}

// Splits the string input into pieces delimited by delimiter.
// if 2 delimiters are in a row, it will include the empty string between them.
int StringUtils::SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results)
{
  int iPos = -1;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  results.clear();

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
    newPos = input.Find (delimiter, iPos + sizeS2);
  }

  for ( unsigned int i = 0; i <= positions.size(); i++ )
  {
    CStdString s;
    if ( i == 0 )
	  {
		  if (i == positions.size())
			  s = input;
		  else
		    s = input.Mid( i, positions[i] );
	  }
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
// returns negative if left < right, positive if left > right
// and 0 if they are identical (essentially calculates left - right)
int StringUtils::AlphaNumericCompare(const char *left, const char *right)
{
  unsigned char *l = (unsigned char *)left;
  unsigned char *r = (unsigned char *)right;
  unsigned char *ld, *rd;
  unsigned char lc, rc;
  unsigned int lnum, rnum;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= '0' && *l <= '9' && *r >= '0' && *r <= '9')
    {
      ld = l;
      lnum = 0;
      while (*ld >= '0' && *ld <= '9' && ld < l + 8)
      { // compare only up to 8 digits
        lnum *= 10;
        lnum += *ld++ - '0';
      }
      rd = r;
      rnum = 0;
      while (*rd >= '0' && *rd <= '9' && rd < r + 8)
      { // compare only up to 8 digits
        rnum *= 10;
        rnum += *rd++ - '0';
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return lnum - rnum;
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
      return lc - rc;
    }
    l++; r++;
  }
  if (*r)
  { // r is longer
    return -1;
  }
  else if (*l)
  { // l is longer
    return 1;
  }
  return 0; // files are the same
}

long StringUtils::TimeStringToSeconds(const CStdString &timeString)
{  
  if(timeString.Right(4).Equals(" min"))
  {
    // this is imdb format of "XXX min"
    return 60 * atoi(timeString.c_str());
  }
  else
  {
    CStdStringArray secs;
    StringUtils::SplitString(timeString, ":", secs);
    int timeInSecs = 0;
    for (unsigned int i = 0; i < secs.size(); i++)
    {
      timeInSecs *= 60;
      timeInSecs += atoi(secs[i]);
    }
    return timeInSecs;
  }
}

void StringUtils::SecondsToTimeString(long lSeconds, CStdString& strHMS, TIME_FORMAT format)
{
  int hh = lSeconds / 3600;
  lSeconds = lSeconds % 3600;
  int mm = lSeconds / 60;
  int ss = lSeconds % 60;

  if (format == TIME_FORMAT_GUESS)
    format = (hh >= 1) ? TIME_FORMAT_HH_MM_SS : TIME_FORMAT_MM_SS;
  strHMS.Empty();
  if (format & TIME_FORMAT_HH)
    strHMS.AppendFormat("%02.2i", hh);
  if (format & TIME_FORMAT_MM)
    strHMS.AppendFormat(strHMS.IsEmpty() ? "%02.2i" : ":%02.2i", mm);
  if (format & TIME_FORMAT_SS)
    strHMS.AppendFormat(strHMS.IsEmpty() ? "%02.2i" : ":%02.2i", ss);
}

bool StringUtils::IsNaturalNumber(const CStdString& str)
{
  if (0 == (int)str.size())
    return false;
  for (int i = 0; i < (int)str.size(); i++)
  {
    if ((str[i] < '0') || (str[i] > '9')) return false;
  }
  return true;
}

void StringUtils::RemoveCRLF(CStdString& strLine)
{
  while ( strLine.size() && (strLine.Right(1) == "\n" || strLine.Right(1) == "\r") )
  {
    strLine = strLine.Left((int)strLine.size() - 1);
  }
}

CStdString StringUtils::SizeToString(__int64 size)
{
  CStdString strLabel;
  if (size == 0)
  {
    strLabel.Format("0.0 KB");
    return strLabel;
  }

  // file < 1 kbyte?
  if (size < 1024)
  {
    //  substract the integer part of the float value
    float fRemainder = (((float)size) / 1024.0f) - floor(((float)size) / 1024.0f);
    float fToAdd = 0.0f;
    if (fRemainder < 0.01f)
      fToAdd = 0.1f;
    strLabel.Format("%2.1f KB", (((float)size) / 1024.0f) + fToAdd);
    return strLabel;
  }
  const __int64 iOneMeg = 1024 * 1024;

  // file < 1 megabyte?
  if (size < iOneMeg)
  {
    strLabel.Format("%02.1f KB", ((float)size) / 1024.0f);
    return strLabel;
  }

  // file < 1 GByte?
  __int64 iOneGigabyte = iOneMeg;
  iOneGigabyte *= (__int64)1000;
  if (size < iOneGigabyte)
  {
    strLabel.Format("%02.1f MB", ((float)size) / ((float)iOneMeg));
    return strLabel;
  }
  //file > 1 GByte
  int iGigs = 0;
  __int64 dwFileSize = size;
  while (dwFileSize >= iOneGigabyte)
  {
    dwFileSize -= iOneGigabyte;
    iGigs++;
  }
  float fMegs = ((float)dwFileSize) / ((float)iOneMeg);
  fMegs /= 1000.0f;
  fMegs += iGigs;
  strLabel.Format("%02.1f GB", fMegs);

  return strLabel;
}

bool StringUtils::FindWords(const char *str, const char *wordLowerCase)
{
  // NOTE: This assumes word is lowercase!
  unsigned char *s = (unsigned char *)str;
  do
  {
    // start with a compare
    unsigned char *c = s;
    unsigned char *w = (unsigned char *)wordLowerCase;
    bool same = true;
    while (same && *c && *w)
    {
      unsigned char lc = *c++;
      if (lc >= 'A' && lc <= 'Z')
        lc += 'a'-'A';

      if (lc != *w++) // different
        same = false;
    }
    if (same && *w == 0)  // only the same if word has been exhausted
      return true;

    // otherwise, find a space and skip to the end of the whitespace
    while (*s && *s != ' ') s++;
    while (*s && *s == ' ') s++;

    // and repeat until we're done
  } while (*s);

  return false;
}
