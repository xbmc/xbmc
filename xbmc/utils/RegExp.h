/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//! @todo - move to std::regex (after switching to gcc 4.9 or higher) and get rid of CRegExp

#include <string>
#include <vector>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

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
  int GetSubLength(int iSub) const;
  int GetCaptureTotal() const;
  std::string GetMatch(int iSub = 0) const;
  const std::string& GetPattern() const { return m_pattern; }
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

  pcre2_code* m_re;
  pcre2_match_context* m_ctxt;
  static const int OVECCOUNT=(m_MaxNumOfBackrefrences + 1) * 3;
  unsigned int m_offset;
  pcre2_match_data* m_matchData;
  PCRE2_SIZE* m_iOvector;
  utf8Mode    m_utf8Mode;
  int         m_iMatchCount;
  uint32_t m_iOptions;
  bool        m_jitCompiled;
  bool        m_bMatched;
  pcre2_jit_stack* m_jitStack;
  std::string m_subject;
  std::string m_pattern;
  static int  m_Utf8Supported;
  static int  m_UcpSupported;
  static int  m_JitSupported;
};

typedef std::vector<CRegExp> VECCREGEXP;

