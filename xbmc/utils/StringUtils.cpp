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
//  File:      StringUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to use J O'Leary's CStdString class by kraqh3d
//
//------------------------------------------------------------------------


#include "StringUtils.h"
#include "utils/RegExp.h"
#include "utils/fstrcmp.h"
#include "LangInfo.h"
#include <locale>

#include <math.h>
#include <sstream>
#include <time.h>

using namespace std;

const char* ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";

/* empty string for use in returns by ref */
const CStdString StringUtils::EmptyString = "";
CStdString StringUtils::m_lastUUID = "";

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
// added MaxStrings parameter to restrict the number of returned substrings (like perl and python)
int StringUtils::SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings /* = 0 */)
{
  int iPos = -1;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  results.clear();

  vector<unsigned int> positions;

  newPos = input.Find (delimiter, 0);

  if ( newPos < 0 )
  {
    results.push_back(input);
    return 1;
  }

  while ( newPos > iPos )
  {
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.Find (delimiter, iPos + sizeS2);
  }

  // numFound is the number of delimeters which is one less
  // than the number of substrings
  unsigned int numFound = positions.size();
  if (iMaxStrings > 0 && numFound >= iMaxStrings)
    numFound = iMaxStrings - 1;

  for ( unsigned int i = 0; i <= numFound; i++ )
  {
    CStdString s;
    if ( i == 0 )
    {
      if ( i == numFound )
        s = input;
      else
        s = input.Mid( i, positions[i] );
    }
    else
    {
      int offset = positions[i - 1] + sizeS2;
      if ( offset < isize )
      {
        if ( i == numFound )
          s = input.Mid(offset);
        else if ( i > 0 )
          s = input.Mid( positions[i - 1] + sizeS2,
                         positions[i] - positions[i - 1] - sizeS2 );
      }
    }
    results.push_back(s);
  }
  // return the number of substrings
  return results.size();
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
int64_t StringUtils::AlphaNumericCompare(const wchar_t *left, const wchar_t *right)
{
  wchar_t *l = (wchar_t *)left;
  wchar_t *r = (wchar_t *)right;
  wchar_t *ld, *rd;
  wchar_t lc, rc;
  int64_t lnum, rnum;
  const collate<wchar_t>& coll = use_facet< collate<wchar_t> >( locale() );
  int cmp_res = 0;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= L'0' && *l <= L'9' && *r >= L'0' && *r <= L'9')
    {
      ld = l;
      lnum = 0;
      while (*ld >= L'0' && *ld <= L'9' && ld < l + 15)
      { // compare only up to 15 digits
        lnum *= 10;
        lnum += *ld++ - '0';
      }
      rd = r;
      rnum = 0;
      while (*rd >= L'0' && *rd <= L'9' && rd < r + 15)
      { // compare only up to 15 digits
        rnum *= 10;
        rnum += *rd++ - L'0';
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
    if (lc >= L'A' && lc <= L'Z')
      lc += L'a'-L'A';
    rc = *r;
    if (rc >= L'A' && rc <= L'Z')
      rc += L'a'- L'A';

    // ok, do a normal comparison, taking current locale into account. Add special case stuff (eg '(' characters)) in here later
    if ((cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1)) != 0)
    {
      return cmp_res;
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

int StringUtils::DateStringToYYYYMMDD(const CStdString &dateString)
{
  CStdStringArray days;
  int splitCount = StringUtils::SplitString(dateString, "-", days);
  if (splitCount == 1)
    return atoi(days[0].c_str());
  else if (splitCount == 2)
    return atoi(days[0].c_str())*100+atoi(days[1].c_str());
  else if (splitCount == 3)
    return atoi(days[0].c_str())*10000+atoi(days[1].c_str())*100+atoi(days[2].c_str());
  else
    return -1;
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

CStdString StringUtils::SecondsToTimeString(long lSeconds, TIME_FORMAT format)
{
  int hh = lSeconds / 3600;
  lSeconds = lSeconds % 3600;
  int mm = lSeconds / 60;
  int ss = lSeconds % 60;

  if (format == TIME_FORMAT_GUESS)
    format = (hh >= 1) ? TIME_FORMAT_HH_MM_SS : TIME_FORMAT_MM_SS;
  CStdString strHMS;
  if (format & TIME_FORMAT_HH)
    strHMS.AppendFormat("%02.2i", hh);
  else if (format & TIME_FORMAT_H)
    strHMS.AppendFormat("%i", hh);
  if (format & TIME_FORMAT_MM)
    strHMS.AppendFormat(strHMS.IsEmpty() ? "%02.2i" : ":%02.2i", mm);
  if (format & TIME_FORMAT_SS)
    strHMS.AppendFormat(strHMS.IsEmpty() ? "%02.2i" : ":%02.2i", ss);
  return strHMS;
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

bool StringUtils::IsInteger(const CStdString& str)
{
  if (str.size() > 0 && str[0] == '-')
    return IsNaturalNumber(str.Mid(1));
  else
    return IsNaturalNumber(str);
}

void StringUtils::RemoveCRLF(CStdString& strLine)
{
  while ( strLine.size() && (strLine.Right(1) == "\n" || strLine.Right(1) == "\r") )
  {
    strLine = strLine.Left(std::max(0, (int)strLine.size() - 1));
  }
}

CStdString StringUtils::SizeToString(int64_t size)
{
  CStdString strLabel;
  const char prefixes[] = {' ','k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
  unsigned int i = 0;
  double s = (double)size;
  while (i < sizeof(prefixes)/sizeof(prefixes[0]) && s >= 1000.0)
  {
    s /= 1024.0;
    i++;
  }

  if (!i)
    strLabel.Format("%.0lf %cB ", s, prefixes[i]);
  else if (s >= 100.0)
    strLabel.Format("%.1lf %cB", s, prefixes[i]);
  else
    strLabel.Format("%.2lf %cB", s, prefixes[i]);

  return strLabel;
}

size_t StringUtils::FindWords(const char *str, const char *wordLowerCase)
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
      return (const char *)s - str;

    // otherwise, find a space and skip to the end of the whitespace
    while (*s && *s != ' ') s++;
    while (*s && *s == ' ') s++;

    // and repeat until we're done
  } while (*s);

  return CStdString::npos;
}

// assumes it is called from after the first open bracket is found
int StringUtils::FindEndBracket(const CStdString &str, char opener, char closer, int startPos)
{
  int blocks = 1;
  for (unsigned int i = startPos; i < str.size(); i++)
  {
    if (str[i] == opener)
      blocks++;
    else if (str[i] == closer)
    {
      blocks--;
      if (!blocks)
        return i;
    }
  }

  return (int)CStdString::npos;
}

void StringUtils::WordToDigits(CStdString &word)
{
  static const char word_to_letter[] = "22233344455566677778889999";
  word.ToLower();
  for (unsigned int i = 0; i < word.size(); ++i)
  { // NB: This assumes ascii, which probably needs extending at some  point.
    char letter = word[i];
    if ((letter >= 'a' && letter <= 'z')) // assume contiguous letter range
    {
      word[i] = word_to_letter[letter-'a'];
    }
    else if (letter < '0' || letter > '9') // We want to keep 0-9!
    {
      word[i] = ' ';  // replace everything else with a space
    }
  }
}

CStdString StringUtils::CreateUUID()
{
  /* This function generate a DCE 1.1, ISO/IEC 11578:1996 and IETF RFC-4122
  * Version 4 conform local unique UUID based upon random number generation.
  */
  char UuidStrTmp[40];
  char *pUuidStr = UuidStrTmp;
  int i;

  static bool m_uuidInitialized = false;
  if (!m_uuidInitialized)
  {
    /* use current time as the seed for rand()*/
    srand(time(NULL));
    m_uuidInitialized = true;
  }

  /*Data1 - 8 characters.*/
  for(i = 0; i < 8; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data2 - 4 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 4; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data3 - 4 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 4; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data4 - 4 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 4; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data5 - 12 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 12; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  *pUuidStr = '\0';

  m_lastUUID = UuidStrTmp;
  return UuidStrTmp;
}

bool StringUtils::ValidateUUID(const CStdString &uuid)
{
  CRegExp guidRE;
  guidRE.RegComp(ADDON_GUID_RE);
  return (guidRE.RegFind(uuid.c_str()) == 0);
}

double StringUtils::CompareFuzzy(const CStdString &left, const CStdString &right)
{
  return (0.5 + fstrcmp(left.c_str(), right.c_str(), 0.0) * (left.length() + right.length())) / 2.0;
}

int StringUtils::FindBestMatch(const CStdString &str, const CStdStringArray &strings, double &matchscore)
{
  int best = -1;
  matchscore = 0;

  int i = 0;
  for (CStdStringArray::const_iterator it = strings.begin(); it != strings.end(); it++, i++)
  {
    int maxlength = max(str.length(), it->length());
    double score = StringUtils::CompareFuzzy(str, *it) / maxlength;
    if (score > matchscore)
    {
      matchscore = score;
      best = i;
    }
  }
  return best;
}
