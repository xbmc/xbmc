#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef REGEXP_H
#define REGEXP_H

#include <string>
#include <vector>

namespace PCRE {
#ifdef TARGET_WINDOWS
#define PCRE_STATIC 1
#ifdef _DEBUG
#pragma comment(lib, "pcred.lib")
#else  // ! _DEBUG
#pragma comment(lib, "pcre.lib")
#endif // ! _DEBUG
#endif // TARGET_WINDOWS
#include <pcre.h>
}

class CRegExp
{
public:
  static const int m_MaxNumOfBackrefrences = 20;
  CRegExp(bool caseless = false, bool utf8 = false);
  CRegExp(const CRegExp& re);
  ~CRegExp();

  bool RegComp(const char *re);
  bool RegComp(const std::string& re) { return RegComp(re.c_str()); }
  int RegFind(const char* str, unsigned int startoffset = 0, int maxNumberOfCharsToTest = -1);
  int RegFind(const std::string& str, unsigned int startoffset = 0, int maxNumberOfCharsToTest = -1)
  { return PrivateRegFind(str.length(), str.c_str(), startoffset, maxNumberOfCharsToTest); }
  std::string GetReplaceString(const std::string& sReplaceExp) const;
  int GetFindLen() const
  {
    if (!m_re || !m_bMatched)
      return 0;

    return (m_iOvector[1] - m_iOvector[0]);
  };
  int GetSubCount() const { return m_iMatchCount - 1; } // PCRE returns the number of sub-patterns + 1
  int GetSubStart(int iSub) const;
  int GetSubStart(const std::string& subName) const;
  int GetSubLength(int iSub) const;
  int GetSubLength(const std::string& subName) const;
  int GetCaptureTotal() const;
  std::string GetMatch(int iSub = 0) const;
  std::string GetMatch(const std::string& subName) const;
  const std::string& GetPattern() const { return m_pattern; }
  bool GetNamedSubPattern(const char* strName, std::string& strMatch) const;
  int GetNamedSubPatternNumber(const char* strName) const;
  void DumpOvector(int iLog);
  const CRegExp& operator= (const CRegExp& re);
  static bool IsUtf8Supported(void);
  static bool AreUnicodePropertiesSupported(void);
  static bool LogCheckUtf8Support(void);
  static bool IsJitSupported(void);

private:
  int PrivateRegFind(size_t bufferLen, const char *str, unsigned int startoffset = 0, int maxNumberOfCharsToTest = -1);

  void Cleanup();
  inline bool IsValidSubNumber(int iSub) const;

  PCRE::pcre* m_re;
  PCRE::pcre_extra* m_sd;
  static const int OVECCOUNT=(m_MaxNumOfBackrefrences + 1) * 3;
  unsigned int m_offset;
  int         m_iOvector[OVECCOUNT];
  int         m_iMatchCount;
  int         m_iOptions;
  bool        m_bMatched;
  std::string m_subject;
  std::string m_pattern;
  static int  m_Utf8Supported;
  static int  m_UcpSupported;
  static int  m_JitSupported;
};

typedef std::vector<CRegExp> VECCREGEXP;

#endif

