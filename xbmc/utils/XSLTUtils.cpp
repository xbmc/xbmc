/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "XSLTUtils.h"
#include "log.h"
#include <libxslt/xslt.h>
#include <libxslt/transform.h>

#ifdef TARGET_WINDOWS
#pragma comment(lib, "libxslt.lib")
#pragma comment(lib, "libxml2.lib")
#else
#include <iostream>
#endif

#define TMP_BUF_SIZE 512
void err(void *ctx, const char *msg, ...) {
  char string[TMP_BUF_SIZE];
  va_list arg_ptr;
  va_start(arg_ptr, msg);
  vsnprintf(string, TMP_BUF_SIZE, msg, arg_ptr);
  va_end(arg_ptr);
  CLog::Log(LOGDEBUG, "XSLT: %s", string);
  return;
}

XSLTUtils::XSLTUtils() :
m_xmlInput(nullptr), m_xmlOutput(nullptr), m_xmlStylesheet(nullptr), m_xsltStylesheet(nullptr)
{
  // initialize libxslt
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 0;
  xsltSetGenericErrorFunc(NULL, err);
}

XSLTUtils::~XSLTUtils()
{
  if (m_xmlInput)
    xmlFreeDoc(m_xmlInput);
  if (m_xmlOutput)
    xmlFreeDoc(m_xmlOutput);
  if (m_xsltStylesheet)
    xsltFreeStylesheet(m_xsltStylesheet);
}

bool XSLTUtils::XSLTTransform(std::string& output)
{
  const char *params[16+1];
  params[0] = NULL;
  m_xmlOutput = xsltApplyStylesheet(m_xsltStylesheet, m_xmlInput, params);
  if (!m_xmlOutput)
  {
    CLog::Log(LOGDEBUG, "XSLT: xslt transformation failed");
    return false;
  }

  xmlChar *xmlResultBuffer = NULL;
  int xmlResultLength = 0;
  int res = xsltSaveResultToString(&xmlResultBuffer, &xmlResultLength, m_xmlOutput, m_xsltStylesheet);
  if (res == -1)
  {
    xmlFree(xmlResultBuffer);
    return false;
  }

  output.append((const char *)xmlResultBuffer, xmlResultLength);
  xmlFree(xmlResultBuffer);

  return true;
}

bool XSLTUtils::SetInput(const std::string& input)
{
  m_xmlInput = xmlParseMemory(input.c_str(), input.size());
  if (!m_xmlInput)
    return false;
  return true;
}

bool XSLTUtils::SetStylesheet(const std::string& stylesheet)
{
  if (m_xsltStylesheet) {
    xsltFreeStylesheet(m_xsltStylesheet);
    m_xsltStylesheet = NULL;
  }

  m_xmlStylesheet = xmlParseMemory(stylesheet.c_str(), stylesheet.size());
  if (!m_xmlStylesheet)
  {
    CLog::Log(LOGDEBUG, "could not xmlParseMemory stylesheetdoc");
    return false;
  }

  m_xsltStylesheet = xsltParseStylesheetDoc(m_xmlStylesheet);
  if (!m_xsltStylesheet) {
    CLog::Log(LOGDEBUG, "could not parse stylesheetdoc");
    xmlFree(m_xmlStylesheet);
    m_xmlStylesheet = NULL;
    return false;
  }
  
  return true;
}
