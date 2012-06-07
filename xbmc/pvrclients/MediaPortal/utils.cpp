/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifdef TARGET_WINDOWS
#pragma warning(disable:4244) //wchar to char = loss of data
#endif

#include "client.h"
#include "utils.h"
#include <string>
#include <stdio.h>
#include "platform/util/StdString.h"

using namespace std;
using namespace ADDON;

void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ")
{
  // Skip delimiters at beginning.
  //string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Don't skip delimiters at beginning.
  string::size_type start_pos = 0;
  // Find first "non-delimiter".
  string::size_type delim_pos = 0;

  while (string::npos != delim_pos)
  {
    delim_pos = str.find_first_of(delimiters, start_pos);
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(start_pos, delim_pos - start_pos));
    start_pos = delim_pos + 1;

    // Find next "non-delimiter"
  }
}


std::string WStringToString(const std::wstring& s)
{
  std::string temp(s.length(), ' ');
  std::copy(s.begin(), s.end(), temp.begin());
  return temp;
}

std::wstring StringToWString(const std::string& s)
{
  std::wstring temp(s.length(),L' ');
  std::copy(s.begin(), s.end(), temp.begin());
  return temp;
}

std::string lowercase(const std::string& s)
{
  std::string t;
  for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
  {
    t += tolower(*i);
  }
  return t;
}

bool stringtobool(const std::string& s)
{
  std::string temp = lowercase(s);

  if(temp.compare("false") == 0)
    return false;
  else if(temp.compare("0") == 0)
    return false;
  else
    return true;
}

const char* booltostring(const bool b)
{
  return (b==true) ? "True" : "False";
}

time_t DateTimeToTimeT(const std::string& datetime)
{
  struct tm timeinfo;
  int year, month ,day;
  int hour, minute, second;
  int count;
  time_t retval;

  count = sscanf(datetime.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

  if(count != 6)
    return -1;

  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month - 1;
  timeinfo.tm_mday = day;
  // Make the other fields empty:
  timeinfo.tm_isdst = -1;
  timeinfo.tm_wday = 0;
  timeinfo.tm_yday = 0;

  retval = mktime (&timeinfo);

  if(retval < 0)
    retval = 0;

  return retval;
}

std::string ToThumbFileName(const char* strChannelName)
{
  CStdString strThumbName = strChannelName;

  strThumbName.Replace(":","_");
  strThumbName.Replace("/","_");
  strThumbName.Replace("\\","_");

  return strThumbName;
}

#if defined(TARGET_WINDOWS)
//////////////////////////////////////////////////////////////////////////////
//
// *** Routines to convert between Unicode UTF-8 and Unicode UTF-16 ***
//
// By Giovanni Dicanio <giovanni.dicanio AT gmail.com>
//
// Last update: 2010, January 2nd
//
//
// These routines use ::MultiByteToWideChar and ::WideCharToMultiByte
// Win32 API functions to convert between Unicode UTF-8 and UTF-16.
//
// UTF-16 strings are stored in instances of CStringW.
// UTF-8 strings are stored in instances of CStringA.
//
// On error, the conversion routines use AtlThrow to signal the
// error condition.
//
// If input string pointers are NULL, empty strings are returned.
//
//
// Prefixes used in these routines:
// --------------------------------
//
//  - cch  : count of characters (CHAR's or WCHAR's)
//  - cb   : count of bytes
//  - psz  : pointer to a NUL-terminated string (CHAR* or WCHAR*)
//  - str  : instance of CString(A/W) class
//
//
//
// Useful Web References:
// ----------------------
//
// WideCharToMultiByte Function
// http://msdn.microsoft.com/en-us/library/dd374130.aspx
//
// MultiByteToWideChar Function
// http://msdn.microsoft.com/en-us/library/dd319072.aspx
//
// AtlThrow
// http://msdn.microsoft.com/en-us/library/z325eyx0.aspx
//
//
// Developed on VC9 (Visual Studio 2008 SP1)
//
//
//////////////////////////////////////////////////////////////////////////////



namespace UTF8Util
{
  //----------------------------------------------------------------------------
  // FUNCTION: ConvertUTF8ToUTF16
  // DESC: Converts Unicode UTF-8 text to Unicode UTF-16 (Windows default).
  //----------------------------------------------------------------------------
  CStdStringW ConvertUTF8ToUTF16(const char* pszTextUTF8)
  {
    //
    // Special case of NULL or empty input string
    //
    if ( (pszTextUTF8 == NULL) || (*pszTextUTF8 == '\0') )
    {
      // Return empty string
      return L"";
    }

    //
    // Consider CHAR's count corresponding to total input string length,
    // including end-of-string (\0) character
    //
    const size_t cchUTF8Max = INT_MAX - 1;
    size_t cchUTF8 = strlen(pszTextUTF8);

    // Consider also terminating \0
    ++cchUTF8;

    // Convert to 'int' for use with MultiByteToWideChar API
    int cbUTF8 = static_cast<int>( cchUTF8 );

    //
    // Get size of destination UTF-16 buffer, in WCHAR's
    //
    int cchUTF16 = ::MultiByteToWideChar(
      CP_UTF8,                // convert from UTF-8
      MB_ERR_INVALID_CHARS,   // error on invalid chars
      pszTextUTF8,            // source UTF-8 string
      cbUTF8,                 // total length of source UTF-8 string,
      // in CHAR's (= bytes), including end-of-string \0
      NULL,                   // unused - no conversion done in this step
      0                       // request size of destination buffer, in WCHAR's
      );

    if ( cchUTF16 == 0 )
    {
      DWORD dwErr = GetLastError();
      XBMC->Log(LOG_ERROR, "ConvertUTF8ToUTF16 failed lasterror == 0x%X.", dwErr);
      return L"";
    }


    //
    // Allocate destination buffer to store UTF-16 string
    //
    CStdStringW strUTF16;
    WCHAR * pszUTF16 = strUTF16.GetBuffer( cchUTF16 );

    //
    // Do the conversion from UTF-8 to UTF-16
    //
    int result = ::MultiByteToWideChar(
      CP_UTF8,                // convert from UTF-8
      MB_ERR_INVALID_CHARS,   // error on invalid chars
      pszTextUTF8,            // source UTF-8 string
      cbUTF8,                 // total length of source UTF-8 string,
      // in CHAR's (= bytes), including end-of-string \0
      pszUTF16,               // destination buffer
      cchUTF16                // size of destination buffer, in WCHAR's
      );

    if ( result == 0 )
    {
      DWORD dwErr = GetLastError();
      XBMC->Log(LOG_ERROR, "ConvertUTF8ToUTF16 failed lasterror == 0x%X.", dwErr);
      return L"";
    }

    // Release internal CString buffer
    strUTF16.ReleaseBuffer();

    // Return resulting UTF16 string
    return strUTF16;
  }


  //----------------------------------------------------------------------------
  // FUNCTION: ConvertUTF16ToUTF8
  // DESC: Converts Unicode UTF-16 (Windows default) text to Unicode UTF-8.
  //----------------------------------------------------------------------------
  CStdStringA ConvertUTF16ToUTF8(const WCHAR * pszTextUTF16)
  {
    //
    // Special case of NULL or empty input string
    //
    if ( (pszTextUTF16 == NULL) || (*pszTextUTF16 == L'\0') )
    {
      // Return empty string
      return "";
    }

    //
    // Consider WCHAR's count corresponding to total input string length,
    // including end-of-string (L'\0') character.
    //
    const size_t cchUTF16Max = INT_MAX - 1;
    size_t cchUTF16 = wcslen(pszTextUTF16);

    // Consider also terminating \0
    ++cchUTF16;

    //
    // WC_ERR_INVALID_CHARS flag is set to fail if invalid input character
    // is encountered.
    // This flag is supported on Windows Vista and later.
    // Don't use it on Windows XP and previous.
    //
#if (WINVER >= 0x0600)
    DWORD dwConversionFlags = WC_ERR_INVALID_CHARS;
#else
    DWORD dwConversionFlags = 0;
#endif

    //
    // Get size of destination UTF-8 buffer, in CHAR's (= bytes)
    //
    int cbUTF8 = ::WideCharToMultiByte(
      CP_UTF8,                // convert to UTF-8
      dwConversionFlags,      // specify conversion behavior
      pszTextUTF16,           // source UTF-16 string
      static_cast<int>( cchUTF16 ),   // total source string length, in WCHAR's,
      // including end-of-string \0
      NULL,                   // unused - no conversion required in this step
      0,                      // request buffer size
      NULL, NULL              // unused
      );

    if ( cbUTF8 == 0 )
    {
      DWORD dwErr = GetLastError();
      XBMC->Log(LOG_ERROR, "ConvertUTF16ToUTF8 failed lasterror == 0x%X.", dwErr);
      return L"";
    }

    //
    // Allocate destination buffer for UTF-8 string
    //
    CStdStringA strUTF8;
    int cchUTF8 = cbUTF8; // sizeof(CHAR) = 1 byte
    CHAR * pszUTF8 = strUTF8.GetBuffer( cchUTF8 );

    //
    // Do the conversion from UTF-16 to UTF-8
    //
    int result = ::WideCharToMultiByte(
      CP_UTF8,                // convert to UTF-8
      dwConversionFlags,      // specify conversion behavior
      pszTextUTF16,           // source UTF-16 string
      static_cast<int>( cchUTF16 ),   // total source string length, in WCHAR's,
      // including end-of-string \0
      pszUTF8,                // destination buffer
      cbUTF8,                 // destination buffer size, in bytes
      NULL, NULL              // unused
      ); 

    if ( result == 0 )
    {
      DWORD dwErr = GetLastError();
      XBMC->Log(LOG_ERROR, "ConvertUTF16ToUTF8 failed lasterror == 0x%X.", dwErr);
      return L"";
    }

    // Release internal CString buffer
    strUTF8.ReleaseBuffer();

    // Return resulting UTF-8 string
    return strUTF8;
  }
} // namespace UTF8Util
#endif

//////////////////////////////////////////////////////////////////////////////
