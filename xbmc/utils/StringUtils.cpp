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
#include <locale>

#include <math.h>
#include <sstream>
#include <time.h>

#define FORMAT_BLOCK_SIZE 2048 // # of bytes to increment per try

using namespace std;

const char* ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";

/* empty string for use in returns by ref */
const CStdString StringUtils::EmptyString = "";
CStdString StringUtils::m_lastUUID = "";

string StringUtils::Format(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  string str = FormatV(fmt, args);
  va_end(args);

  return str;
}

string StringUtils::FormatV(const char *fmt, va_list args)
{
  if (fmt == NULL)
    return "";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  char *cstr = reinterpret_cast<char*>(malloc(sizeof(char) * size));
  if (cstr == NULL)
    return "";

  while (1) 
  {
    va_copy(argCopy, args);

    int nActual = vsnprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);

    if (nActual > -1 && nActual < size) // We got a valid result
    {
      string str(cstr, nActual);
      free(cstr);
      return str;
    }
    if (nActual > -1)                   // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else                                // Let's try to double the size (glibc 2.0)
      size *= 2;

    char *new_cstr = reinterpret_cast<char*>(realloc(cstr, sizeof(char) * size));
    if (new_cstr == NULL)
    {
      free(cstr);
      return "";
    }

    cstr = new_cstr;
  }

  return "";
}

void StringUtils::ToUpper(string &str)
{
  transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void StringUtils::ToLower(string &str)
{
  transform(str.begin(), str.end(), str.begin(), ::tolower);
}

bool StringUtils::EqualsNoCase(const std::string &str1, const std::string &str2)
{
  string tmp1 = str1;
  string tmp2 = str2;
  ToLower(tmp1);
  ToLower(tmp2);
  
  return tmp1.compare(tmp2) == 0;
}

string StringUtils::Left(const string &str, size_t count)
{
  count = max((size_t)0, min(count, str.size()));
  return str.substr(0, count);
}

string StringUtils::Mid(const string &str, size_t first, size_t count /* = string::npos */)
{
  if (first + count > str.size())
    count = str.size() - first;
  
  if (first > str.size())
    return string();
  
  ASSERT(first + count <= str.size());
  
  return str.substr(first, count);
}

string StringUtils::Right(const string &str, size_t count)
{
  count = max((size_t)0, min(count, str.size()));
  return str.substr(str.size() - count);
}

std::string& StringUtils::Trim(std::string &str)
{
  TrimLeft(str);
  return TrimRight(str);
}

std::string& StringUtils::TrimLeft(std::string &str)
{
  str.erase(str.begin(), ::find_if(str.begin(), str.end(), ::not1(::ptr_fun<int, int>(::isspace))));
  return str;
}

std::string& StringUtils::TrimRight(std::string &str)
{
  str.erase(::find_if(str.rbegin(), str.rend(), ::not1(::ptr_fun<int, int>(::isspace))).base(), str.end());
  return str;
}

int StringUtils::Replace(string &str, char oldChar, char newChar)
{
  int replacedChars = 0;
  for (string::iterator it = str.begin(); it != str.end(); it++)
  {
    if (*it == oldChar)
    {
      *it = newChar;
      replacedChars++;
    }
  }
  
  return replacedChars;
}

int StringUtils::Replace(std::string &str, const std::string &oldStr, const std::string &newStr)
{
  int replacedChars = 0;
  size_t index = 0;
  
  while (index < str.size() && (index = str.find(oldStr, index)) != string::npos)
  {
    str.replace(index, oldStr.size(), newStr);
    index += newStr.size();
    replacedChars++;
  }
  
  return replacedChars;
}

bool StringUtils::StartsWith(const std::string &str, const std::string &str2, bool useCase /* = false */)
{
  std::string left = StringUtils::Left(str, str2.size());
  
  if (useCase)
    return left.compare(str2) == 0;

  return StringUtils::EqualsNoCase(left, str2);
}

bool StringUtils::EndsWith(const std::string &str, const std::string &str2, bool useCase /* = false */)
{
  std::string right = StringUtils::Right(str, str2.size());
  
  if (useCase)
    return right.compare(str2) == 0;

  return StringUtils::EqualsNoCase(right, str2);
}

void StringUtils::JoinString(const CStdStringArray &strings, const CStdString& delimiter, CStdString& result)
{
  result = "";
  for(CStdStringArray::const_iterator it = strings.begin(); it != strings.end(); it++ )
    result += (*it) + delimiter;

  if(result != "")
    result.Delete(result.size()-delimiter.size(), delimiter.size());
}

CStdString StringUtils::JoinString(const CStdStringArray &strings, const CStdString& delimiter)
{
  CStdString result;
  JoinString(strings, delimiter, result);
  return result;
}

CStdString StringUtils::Join(const vector<string> &strings, const CStdString& delimiter)
{
  CStdStringArray strArray;
  for (unsigned int index = 0; index < strings.size(); index++)
    strArray.push_back(strings.at(index));

  return JoinString(strArray, delimiter);
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

  // numFound is the number of delimiters which is one less
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

CStdStringArray StringUtils::SplitString(const CStdString& input, const CStdString& delimiter, unsigned int iMaxStrings /* = 0 */)
{
  CStdStringArray result;
  SplitString(input, delimiter, result, iMaxStrings);
  return result;
}

vector<string> StringUtils::Split(const CStdString& input, const CStdString& delimiter, unsigned int iMaxStrings /* = 0 */)
{
  CStdStringArray result;
  SplitString(input, delimiter, result, iMaxStrings);

  vector<string> strArray;
  for (unsigned int index = 0; index < result.size(); index++)
    strArray.push_back(result.at(index));

  return strArray;
}

// returns the number of occurrences of strFind in strInput.
int StringUtils::FindNumber(const CStdString& strInput, const CStdString &strFind)
{
  int pos = strInput.Find(strFind, 0);
  int numfound = 0;
  while (pos >= 0)
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
  CStdString strCopy(timeString);
  strCopy.TrimLeft(" \n\r\t");
  strCopy.TrimRight(" \n\r\t");
  if(strCopy.Right(4).Equals(" min"))
  {
    // this is imdb format of "XXX min"
    return 60 * atoi(strCopy.c_str());
  }
  else
  {
    CStdStringArray secs;
    StringUtils::SplitString(strCopy, ":", secs);
    int timeInSecs = 0;
    for (unsigned int i = 0; i < 3 && i < secs.size(); i++)
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
  size_t i = 0, n = 0;
  // allow whitespace,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

bool StringUtils::IsInteger(const CStdString& str)
{
  size_t i = 0, n = 0;
  // allow whitespace,-,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  if (i < str.size() && str[i] == '-')
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

bool StringUtils::Test()
{
  bool ret = true;

  ret |= IsNaturalNumber("10");
  ret |= IsNaturalNumber(" 10");
  ret |= IsNaturalNumber("0");
  ret |= !IsNaturalNumber(" 1 0");
  ret |= !IsNaturalNumber("1.0");
  ret |= !IsNaturalNumber("1.1");
  ret |= !IsNaturalNumber("0x1");
  ret |= !IsNaturalNumber("blah");
  ret |= !IsNaturalNumber("120 h");
  ret |= !IsNaturalNumber(" ");
  ret |= !IsNaturalNumber("");
  ret |= !IsNaturalNumber("ייטט");

  ret |= IsInteger("10");
  ret |= IsInteger(" -10");
  ret |= IsInteger("0");
  ret |= !IsInteger(" 1 0");
  ret |= !IsInteger("1.0");
  ret |= !IsInteger("1.1");
  ret |= !IsInteger("0x1");
  ret |= !IsInteger("blah");
  ret |= !IsInteger("120 h");
  ret |= !IsInteger(" ");
  ret |= !IsInteger("");
  ret |= !IsInteger("ייטט");

  return ret;
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

size_t StringUtils::utf8_strlen(const char *s)
{
  size_t length = 0;
  while (*s)
  {
    if ((*s++ & 0xC0) != 0x80)
      length++;
  }
  return length;
}
