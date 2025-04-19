/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XSLTUtils.h"
#include "log.h"
#include <libxslt/xslt.h>
#include <libxslt/transform.h>

#ifndef TARGET_WINDOWS
#include <iostream>
#endif

#define TMP_BUF_SIZE 512
void err(void *ctx, const char *msg, ...) {
  char string[TMP_BUF_SIZE];
  va_list arg_ptr;
  va_start(arg_ptr, msg);
  vsnprintf(string, TMP_BUF_SIZE, msg, arg_ptr);
  va_end(arg_ptr);
  CLog::Log(LOGDEBUG, "XSLT: {}", string);
}

XSLTUtils::XSLTUtils()
{
  // initialize libxslt
  // The following two functions are deprecated
#if LIBXML_VERSION <= 21200
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 0;
#endif
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
  m_xmlInput = xmlReadMemory(input.c_str(), input.size(), NULL, NULL, XML_PARSE_NOENT);

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

  m_xmlStylesheet =
      xmlReadMemory(stylesheet.c_str(), stylesheet.size(), NULL, NULL, XML_PARSE_NOENT);

  if (!m_xmlStylesheet)
  {
    CLog::Log(LOGDEBUG, "could not read stylesheetdoc");
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
