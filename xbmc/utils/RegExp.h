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
struct real_pcre_jit_stack; // forward declaration for PCRE without JIT
typedef struct real_pcre_jit_stack pcre_jit_stack;
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
  enum studyMode
  {
    NoStudy          = 0, // do not study expression
    StudyRegExp      = 1, // study expression (slower compilation, faster find)
    StudyWithJitComp      // study expression and JIT-compile it, if possible (heavyweight optimization) 
  };
  enum utf8Mode
  {
    autoUtf8  = -1, // analyze regexp for UTF-8 multi-byte chars, for Unicode codes > 0xFF
                    // or explicit Unicode properties (\p, \P and \X), enable UTF-8 mode if any of them are found
    asciiOnly =  0, // process regexp and strings as single-byte encoded strings
    forceUtf8 =  1  // enable UTF-8 mode (with Unicode properties)
  };

  static const int m_MaxNumOfBackrefrences = 20;
  /**
   * @param caseless (optional) Matching will be case insensitive if set to true
   *                            or case sensitive if set to false
   * @param utf8 (optional) Control UTF-8 processing
   */
  CRegExp(bool caseless = false, utf8Mode utf8 = asciiOnly);
  /**
   * Create new CRegExp object and compile regexp expression in one step
   * @warning Use only with hardcoded regexp when you're sure that regexp is compiled without errors
   * @param caseless    Matching will be case insensitive if set to true 
   *                    or case sensitive if set to false
   * @param utf8        Control UTF-8 processing
   * @param re          The regular expression
   * @param study (optional) Controls study of expression, useful if expression will be used
   *                         several times
   */
  CRegExp(bool caseless, utf8Mode utf8, const char *re, studyMode study = NoStudy);

  CRegExp(const CRegExp& re);
  ~CRegExp();

  /**
   * Compile (prepare) regular expression
   * @param re          The regular expression
   * @param study (optional) Controls study of expression, useful if expression will be used 
   *                         several times
   * @return true on success, false on any error
   */
  bool RegComp(const char *re, studyMode study = NoStudy);

  /**
   * Compile (prepare) regular expression
   * @param re          The regular expression
   * @param study (optional) Controls study of expression, useful if expression will be used
   *                         several times
   * @return true on success, false on any error
   */
  bool RegComp(const std::string& re, studyMode study = NoStudy)
  { return RegComp(re.c_str(), study); }

  /**
   * Find first match of regular expression in given string
   * @param str         The string to match against regular expression
   * @param startoffset (optional) The string offset to start matching
   * @param maxNumberOfCharsToTest (optional) The maximum number of characters to test (match) in 
   *                                          string. If set to -1 string checked up to the end.
   * @return staring position of match in string, negative value in case of error or no match
   */
  int RegFind(const char* str, unsigned int startoffset = 0, int maxNumberOfCharsToTest = -1);
  /**
   * Find first match of regular expression in given string
   * @param str         The string to match against regular expression
   * @param startoffset (optional) The string offset to start matching
   * @param maxNumberOfCharsToTest (optional) The maximum number of characters to test (match) in
   *                                          string. If set to -1 string checked up to the end.
   * @return staring position of match in string, negative value in case of error or no match
   */
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
  /**
   * Check is RegExp object is ready for matching
   * @return true if RegExp object is ready for matching, false otherwise
   */
  inline bool IsCompiled(void) const
  { return !m_pattern.empty(); }
  CRegExp& operator= (const CRegExp& re);
  static bool IsUtf8Supported(void);
  static bool AreUnicodePropertiesSupported(void);
  static bool LogCheckUtf8Support(void);
  static bool IsJitSupported(void);

private:
  int PrivateRegFind(size_t bufferLen, const char *str, unsigned int startoffset = 0, int maxNumberOfCharsToTest = -1);
  void InitValues(bool caseless = false, CRegExp::utf8Mode utf8 = asciiOnly);
  static bool requireUtf8(const std::string& regexp);
  static int readCharXCode(const std::string& regexp, size_t& pos);
  static bool isCharClassWithUnicode(const std::string& regexp, size_t& pos);

  void Cleanup();
  inline bool IsValidSubNumber(int iSub) const;

  PCRE::pcre* m_re;
  PCRE::pcre_extra* m_sd;
  static const int OVECCOUNT=(m_MaxNumOfBackrefrences + 1) * 3;
  unsigned int m_offset;
  int         m_iOvector[OVECCOUNT];
  utf8Mode    m_utf8Mode;
  int         m_iMatchCount;
  int         m_iOptions;
  bool        m_jitCompiled;
  bool        m_bMatched;
  PCRE::pcre_jit_stack* m_jitStack;
  std::string m_subject;
  std::string m_pattern;
  static int  m_Utf8Supported;
  static int  m_UcpSupported;
  static int  m_JitSupported;
};

typedef std::vector<CRegExp> VECCREGEXP;

#endif

