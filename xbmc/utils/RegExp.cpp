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

#include <stdlib.h>
#include <string.h>
#include <algorithm> 
#include "RegExp.h"
#include "StdString.h"
#include "log.h"
#include "utils/StringUtils.h"
#include "utils/Utf8Utils.h"

using namespace PCRE;

#ifndef PCRE_UCP
#define PCRE_UCP 0
#endif // PCRE_UCP

#ifdef PCRE_CONFIG_JIT
#define PCRE_HAS_JIT_CODE 1
#endif

#ifndef PCRE_STUDY_JIT_COMPILE
#define PCRE_STUDY_JIT_COMPILE 0
#endif
#ifndef PCRE_INFO_JIT
// some unused number
#define PCRE_INFO_JIT 2048
#endif
#ifndef PCRE_HAS_JIT_CODE
#define pcre_free_study(x) pcre_free((x))
#endif

int CRegExp::m_Utf8Supported = -1;
int CRegExp::m_UcpSupported  = -1;
int CRegExp::m_JitSupported  = -1;


CRegExp::CRegExp(bool caseless /*= false*/, bool utf8 /*= false*/)
{
  InitValues(caseless, utf8);
}

void CRegExp::InitValues(bool caseless /*= false*/, bool utf8 /*= false*/)
{
  m_re          = NULL;
  m_sd          = NULL;
  m_iOptions    = PCRE_DOTALL | PCRE_NEWLINE_ANY;
  if(caseless)
    m_iOptions |= PCRE_CASELESS;
  if (utf8)
  {
    if (IsUtf8Supported())
      m_iOptions |= PCRE_UTF8;
    if (AreUnicodePropertiesSupported())
      m_iOptions |= PCRE_UCP;
  }

  m_offset      = 0;
  m_jitCompiled = false;
  m_bMatched    = false;
  m_iMatchCount = 0;
  m_jitStack    = NULL;

  memset(m_iOvector, 0, sizeof(m_iOvector));
}

CRegExp::CRegExp(bool caseless, bool utf8, const char *re, studyMode study /*= NoStudy*/)
{
  InitValues(caseless, utf8);
  RegComp(re, study);
}

CRegExp::CRegExp(const CRegExp& re)
{
  m_re = NULL;
  m_sd = NULL;
  m_jitStack = NULL;
  m_iOptions = re.m_iOptions;
  *this = re;
}

const CRegExp& CRegExp::operator=(const CRegExp& re)
{
  size_t size;
  Cleanup();
  m_jitCompiled = false;
  m_pattern = re.m_pattern;
  if (re.m_re)
  {
    if (pcre_fullinfo(re.m_re, NULL, PCRE_INFO_SIZE, &size) >= 0)
    {
      if ((m_re = (pcre*)malloc(size)))
      {
        memcpy(m_re, re.m_re, size);
        memcpy(m_iOvector, re.m_iOvector, OVECCOUNT*sizeof(int));
        m_offset = re.m_offset;
        m_iMatchCount = re.m_iMatchCount;
        m_bMatched = re.m_bMatched;
        m_subject = re.m_subject;
        m_iOptions = re.m_iOptions;
      }
      else
        CLog::Log(LOGSEVERE, "%s: Failed to allocate memory", __FUNCTION__);
    }
  }
  return *this;
}

CRegExp::~CRegExp()
{
  Cleanup();
}

bool CRegExp::RegComp(const char *re, studyMode study /*= NoStudy*/)
{
  if (!re)
    return false;

  m_offset           = 0;
  m_jitCompiled      = false;
  m_bMatched         = false;
  m_iMatchCount      = 0;
  const char *errMsg = NULL;
  int errOffset      = 0;

  Cleanup();

  m_re = pcre_compile(re, m_iOptions, &errMsg, &errOffset, NULL);
  if (!m_re)
  {
    m_pattern.clear();
    CLog::Log(LOGERROR, "PCRE: %s. Compilation failed at offset %d in expression '%s'",
              errMsg, errOffset, re);
    return false;
  }

  m_pattern = re;

  if (study)
  {
    const bool jitCompile = (study == StudyWithJitComp) && IsJitSupported();
    const int studyOptions = jitCompile ? PCRE_STUDY_JIT_COMPILE : 0;

    m_sd = pcre_study(m_re, studyOptions, &errMsg);
    if (errMsg != NULL)
    {
      CLog::Log(LOGWARNING, "%s: PCRE error \"%s\" while studying expression", __FUNCTION__, errMsg);
      if (m_sd != NULL)
      {
        pcre_free_study(m_sd);
        m_sd = NULL;
      }
    }
    else if (jitCompile)
    {
      int jitPresent = 0;
      m_jitCompiled = (pcre_fullinfo(m_re, m_sd, PCRE_INFO_JIT, &jitPresent) == 0 && jitPresent == 1);
    }
  }

  return true;
}

int CRegExp::RegFind(const char *str, unsigned int startoffset /*= 0*/, int maxNumberOfCharsToTest /*= -1*/)
{
  return PrivateRegFind(strlen(str), str, startoffset, maxNumberOfCharsToTest);
}

int CRegExp::PrivateRegFind(size_t bufferLen, const char *str, unsigned int startoffset /* = 0*/, int maxNumberOfCharsToTest /*= -1*/)
{
  m_offset      = 0;
  m_bMatched    = false;
  m_iMatchCount = 0;

  if (!m_re)
  {
    CLog::Log(LOGERROR, "PCRE: Called before compilation");
    return -1;
  }

  if (!str)
  {
    CLog::Log(LOGERROR, "PCRE: Called without a string to match");
    return -1;
  } 

  if (startoffset > bufferLen)
  {
    CLog::Log(LOGERROR, "%s: startoffset is beyond end of string to match", __FUNCTION__);
    return -1;
  }

#ifdef PCRE_HAS_JIT_CODE
  if (m_jitCompiled && !m_jitStack)
  {
    m_jitStack = pcre_jit_stack_alloc(32*1024, 512*1024);
    if (m_jitStack == NULL)
      CLog::Log(LOGWARNING, "%s: can't allocate address space for JIT stack", __FUNCTION__);

    pcre_assign_jit_stack(m_sd, NULL, m_jitStack);
  }
#endif

  if (maxNumberOfCharsToTest >= 0)
    bufferLen = std::min<size_t>(bufferLen, startoffset + maxNumberOfCharsToTest);

  m_subject.assign(str + startoffset, bufferLen - startoffset);
  int rc = pcre_exec(m_re, NULL, m_subject.c_str(), m_subject.length(), 0, 0, m_iOvector, OVECCOUNT);

  if (rc<1)
  {
    static const int fragmentLen = 80; // length of excerpt before erroneous char for log
    switch(rc)
    {
    case PCRE_ERROR_NOMATCH:
      return -1;

    case PCRE_ERROR_MATCHLIMIT:
      CLog::Log(LOGERROR, "PCRE: Match limit reached");
      return -1;

#ifdef PCRE_ERROR_SHORTUTF8 
    case PCRE_ERROR_SHORTUTF8:
      {
        const size_t startPos = (m_subject.length() > fragmentLen) ? CUtf8Utils::RFindValidUtf8Char(m_subject, m_subject.length() - fragmentLen) : 0;
        if (startPos != std::string::npos)
          CLog::Log(LOGERROR, "PCRE: Bad UTF-8 character at the end of string. Text before bad character: \"%s\"", m_subject.substr(startPos).c_str());
        else
          CLog::Log(LOGERROR, "PCRE: Bad UTF-8 character at the end of string");
        return -1;
      }
#endif
    case PCRE_ERROR_BADUTF8:
      {
        const size_t startPos = (m_iOvector[0] > fragmentLen) ? CUtf8Utils::RFindValidUtf8Char(m_subject, m_iOvector[0] - fragmentLen) : 0;
        if (m_iOvector[0] >= 0 && startPos != std::string::npos)
          CLog::Log(LOGERROR, "PCRE: Bad UTF-8 character, error code: %d, position: %d. Text before bad char: \"%s\"", m_iOvector[1], m_iOvector[0], m_subject.substr(startPos, m_iOvector[0] - startPos + 1).c_str());
        else
          CLog::Log(LOGERROR, "PCRE: Bad UTF-8 character, error code: %d, position: %d", m_iOvector[1], m_iOvector[0]);
        return -1;
      }
    case PCRE_ERROR_BADUTF8_OFFSET:
      CLog::Log(LOGERROR, "PCRE: Offset is pointing to the middle of UTF-8 character");
      return -1;

    default:
      CLog::Log(LOGERROR, "PCRE: Unknown error: %d", rc);
      return -1;
    }
  }
  m_offset = startoffset;
  m_bMatched = true;
  m_iMatchCount = rc;
  return m_iOvector[0] + m_offset;
}

int CRegExp::GetCaptureTotal() const
{
  int c = -1;
  if (m_re)
    pcre_fullinfo(m_re, NULL, PCRE_INFO_CAPTURECOUNT, &c);
  return c;
}

std::string CRegExp::GetReplaceString(const std::string& sReplaceExp) const
{
  if (!m_bMatched || sReplaceExp.empty())
    return "";

  const char* const expr = sReplaceExp.c_str();

  size_t pos = sReplaceExp.find_first_of("\\&");
  std::string result(sReplaceExp, 0, pos);
  result.reserve(sReplaceExp.size()); // very rough estimate

  while(pos != std::string::npos)
  {
    if (expr[pos] == '\\')
    {
      // string is null-terminated and current char isn't null, so it's safe to advance to next char
      pos++; // advance to next char
      const char nextChar = expr[pos];
      if (nextChar == '&' || nextChar == '\\')
      { // this is "\&" or "\\" combination
        result.push_back(nextChar); // add '&' or '\' to result 
        pos++; 
      }
      else if (isdigit(nextChar))
      { // this is "\0" - "\9" combination
        int subNum = nextChar - '0';
        pos++; // advance to second next char
        const char secondNextChar = expr[pos];
        if (isdigit(secondNextChar))
        { // this is "\00" - "\99" combination
          subNum = subNum * 10 + (secondNextChar - '0');
          pos++;
        }
        result.append(GetMatch(subNum));
      }
    }
    else
    { // '&' char
      result.append(GetMatch(0));
      pos++;
    }

    const size_t nextPos = sReplaceExp.find_first_of("\\&", pos);
    result.append(sReplaceExp, pos, nextPos - pos);
    pos = nextPos;
  }

  return result;
}

int CRegExp::GetSubStart(int iSub) const
{
  if (!IsValidSubNumber(iSub))
    return -1;

  return m_iOvector[iSub*2] + m_offset;
}

int CRegExp::GetSubStart(const std::string& subName) const
{
  return GetSubStart(GetNamedSubPatternNumber(subName.c_str()));
}

int CRegExp::GetSubLength(int iSub) const
{
  if (!IsValidSubNumber(iSub))
    return -1;

  return m_iOvector[(iSub*2)+1] - m_iOvector[(iSub*2)];
}

int CRegExp::GetSubLength(const std::string& subName) const
{
  return GetSubLength(GetNamedSubPatternNumber(subName.c_str()));
}

std::string CRegExp::GetMatch(int iSub /* = 0 */) const
{
  if (!IsValidSubNumber(iSub))
    return "";

  int pos = m_iOvector[(iSub*2)];
  int len = m_iOvector[(iSub*2)+1] - pos;
  if (pos < 0 || len <= 0)
    return "";

  return m_subject.substr(pos, len);
}

std::string CRegExp::GetMatch(const std::string& subName) const
{
  return GetMatch(GetNamedSubPatternNumber(subName.c_str()));
}

bool CRegExp::GetNamedSubPattern(const char* strName, std::string& strMatch) const
{
  strMatch.clear();
  int iSub = pcre_get_stringnumber(m_re, strName);
  if (!IsValidSubNumber(iSub))
    return false;
  strMatch = GetMatch(iSub);
  return true;
}

int CRegExp::GetNamedSubPatternNumber(const char* strName) const
{
  return pcre_get_stringnumber(m_re, strName);
}

void CRegExp::DumpOvector(int iLog /* = LOGDEBUG */)
{
  if (iLog < LOGDEBUG || iLog > LOGNONE)
    return;

  CStdString str = "{";
  int size = GetSubCount(); // past the subpatterns is junk
  for (int i = 0; i <= size; i++)
  {
    CStdString t = StringUtils::Format("[%i,%i]", m_iOvector[(i*2)], m_iOvector[(i*2)+1]);
    if (i != size)
      t += ",";
    str += t;
  }
  str += "}";
  CLog::Log(iLog, "regexp ovector=%s", str.c_str());
}

void CRegExp::Cleanup()
{
  if (m_re)
  {
    pcre_free(m_re); 
    m_re = NULL; 
  }

  if (m_sd)
  {
    pcre_free_study(m_sd);
    m_sd = NULL;
  }

#ifdef PCRE_HAS_JIT_CODE
  if (m_jitStack)
  {
    pcre_jit_stack_free(m_jitStack);
    m_jitStack = NULL;
  }
#endif
}

inline bool CRegExp::IsValidSubNumber(int iSub) const
{
  return iSub >= 0 && iSub <= m_iMatchCount && iSub <= m_MaxNumOfBackrefrences;
}


bool CRegExp::IsUtf8Supported(void)
{
  if (m_Utf8Supported == -1)
  {
    if (pcre_config(PCRE_CONFIG_UTF8, &m_Utf8Supported) != 0)
      m_Utf8Supported = 0;
  }

  return m_Utf8Supported == 1;
}

bool CRegExp::AreUnicodePropertiesSupported(void)
{
#if defined(PCRE_CONFIG_UNICODE_PROPERTIES) && PCRE_UCP != 0
  if (m_UcpSupported == -1)
  {
    if (pcre_config(PCRE_CONFIG_UNICODE_PROPERTIES, &m_UcpSupported) != 0)
      m_UcpSupported = 0;
  }
#endif

  return m_UcpSupported == 1;
}

bool CRegExp::LogCheckUtf8Support(void)
{
  bool utf8FullSupport = true;

  if (!CRegExp::IsUtf8Supported())
  {
    utf8FullSupport = false;
    CLog::Log(LOGWARNING, "UTF-8 is not supported in PCRE lib, support for national symbols is limited!");
  }

  if (!CRegExp::AreUnicodePropertiesSupported())
  {
    utf8FullSupport = false;
    CLog::Log(LOGWARNING, "Unicode properties are not enabled in PCRE lib, support for national symbols may be limited!");
  }

  if (!utf8FullSupport)
  {
    CLog::Log(LOGNOTICE, "Consider installing PCRE lib version 8.10 or later with enabled Unicode properties and UTF-8 support. Your PCRE lib version: %s", PCRE::pcre_version());
#if PCRE_UCP == 0
    CLog::Log(LOGNOTICE, "You will need to rebuild XBMC after PCRE lib update.");
#endif
  }

  return utf8FullSupport;
}

bool CRegExp::IsJitSupported(void)
{
  if (m_JitSupported == -1)
  {
#ifdef PCRE_HAS_JIT_CODE
    if (pcre_config(PCRE_CONFIG_JIT, &m_JitSupported) != 0)
#endif
      m_JitSupported = 0;
  }

  return m_JitSupported == 1;
}
