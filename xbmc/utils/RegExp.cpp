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
#include "RegExp.h"
#include "StdString.h"
#include "log.h"

using namespace PCRE;


int CRegExp::m_Utf8Supported = -1;
int CRegExp::m_UcpSupported  = -1;


CRegExp::CRegExp(bool caseless /*= false*/, bool utf8 /*= false*/)
{
  m_re          = NULL;
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

  m_bMatched    = false;
  m_iMatchCount = 0;

  memset(m_iOvector, 0, sizeof(m_iOvector));
}

CRegExp::CRegExp(const CRegExp& re)
{
  m_re = NULL;
  m_iOptions = re.m_iOptions;
  *this = re;
}

const CRegExp& CRegExp::operator=(const CRegExp& re)
{
  size_t size;
  Cleanup();
  m_pattern = re.m_pattern;
  if (re.m_re)
  {
    if (pcre_fullinfo(re.m_re, NULL, PCRE_INFO_SIZE, &size) >= 0)
    {
      if ((m_re = (pcre*)malloc(size)))
      {
        memcpy(m_re, re.m_re, size);
        memcpy(m_iOvector, re.m_iOvector, OVECCOUNT*sizeof(int));
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

CRegExp* CRegExp::RegComp(const char *re)
{
  if (!re)
    return NULL;

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
    return NULL;
  }

  m_pattern = re;

  return this;
}

int CRegExp::RegFind(const char* str, int startoffset)
{
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

  m_subject = str;
  int rc = pcre_exec(m_re, NULL, str, strlen(str), startoffset, 0, m_iOvector, OVECCOUNT);

  if (rc<1)
  {
    switch(rc)
    {
    case PCRE_ERROR_NOMATCH:
      return -1;

    case PCRE_ERROR_MATCHLIMIT:
      CLog::Log(LOGERROR, "PCRE: Match limit reached");
      return -1;

#ifdef PCRE_ERROR_SHORTUTF8 
    case PCRE_ERROR_SHORTUTF8:
#endif
    case PCRE_ERROR_BADUTF8:
      CLog::Log(LOGERROR, "PCRE: Bad UTF-8 character");
      return -1;

    case PCRE_ERROR_BADUTF8_OFFSET:
      CLog::Log(LOGERROR, "PCRE: Offset (%d) is pointing to the middle of UTF-8 character", startoffset);
      return -1;

    default:
      CLog::Log(LOGERROR, "PCRE: Unknown error: %d", rc);
      return -1;
    }
  }
  m_bMatched = true;
  m_iMatchCount = rc;
  return m_iOvector[0];
}

int CRegExp::GetCaptureTotal() const
{
  int c = -1;
  if (m_re)
    pcre_fullinfo(m_re, NULL, PCRE_INFO_CAPTURECOUNT, &c);
  return c;
}

std::string CRegExp::GetReplaceString( const char* sReplaceExp ) const
{
  char *src = (char *)sReplaceExp;
  char *buf;
  char c;
  int no;
  size_t len;

  if( sReplaceExp == NULL || !m_bMatched )
    return std::string();

  // First compute the length of the string
  int replacelen = 0;
  while ((c = *src++) != '\0')
  {
    if (c == '&')
      no = 0;
    else if (c == '\\' && isdigit(*src))
      no = *src++ - '0';
    else
      no = -1;

    if (no < 0)
    {
      // Ordinary character.
      if (c == '\\' && (*src == '\\' || *src == '&'))
        c = *src++;
      replacelen++;
    }
    else if (no < m_iMatchCount && (m_iOvector[no*2]>=0))
    {
      // Get tagged expression
      len = m_iOvector[no*2+1] - m_iOvector[no*2];
      replacelen += len;
    }
  }

  // Now allocate buf
  buf = (char *)malloc((replacelen + 1)*sizeof(char));
  if( buf == NULL )
  {
    CLog::Log(LOGSEVERE, "%s: Failed to allocate memory", __FUNCTION__);
    return std::string();
  }

  char* sReplaceStr = buf;

  // Add null termination
  buf[replacelen] = '\0';

  // Now we can create the string
  src = (char *)sReplaceExp;
  while ((c = *src++) != '\0')
  {
    if (c == '&')
      no = 0;
    else if (c == '\\' && isdigit(*src))
      no = *src++ - '0';
    else
      no = -1;

    if (no < 0)
    {
      // Ordinary character.
      if (c == '\\' && (*src == '\\' || *src == '&'))
        c = *src++;
      *buf++ = c;
    }
    else if (no < m_iMatchCount && (m_iOvector[no*2]>=0))
    {
      // Get tagged expression
      len = m_iOvector[no*2+1] - m_iOvector[no*2];
      strncpy(buf, m_subject.c_str()+m_iOvector[no*2], len);
      buf += len;
    }
  }

  std::string replaceStr(sReplaceStr);
  free(sReplaceStr);

  return replaceStr;
}

int CRegExp::GetSubStart(int iSub) const
{
  if (!IsValidSubNumber(iSub))
    return -1;

  return m_iOvector[iSub*2];
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
    CStdString t;
    t.Format("[%i,%i]", m_iOvector[(i*2)], m_iOvector[(i*2)+1]);
    if (i != size)
      t += ",";
    str += t;
  }
  str += "}";
  CLog::Log(iLog, "regexp ovector=%s", str.c_str());
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

  return m_Utf8Supported != 0;
}

bool CRegExp::AreUnicodePropertiesSupported(void)
{
  if (m_UcpSupported == -1)
  {
    if (pcre_config(PCRE_CONFIG_UNICODE_PROPERTIES, &m_UcpSupported) != 0)
      m_UcpSupported = 0;
  }

  return m_UcpSupported != 0;
}
