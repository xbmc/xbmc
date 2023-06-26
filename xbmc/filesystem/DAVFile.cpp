/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DAVFile.h"

#include "DAVCommon.h"
#include "DllLibCurl.h"
#include "URL.h"
#include "utils/RegExp.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

using namespace XFILE;
using namespace XCURL;

CDAVFile::CDAVFile(void)
  : CCurlFile()
{
}

CDAVFile::~CDAVFile(void) = default;

bool CDAVFile::Execute(const CURL& url)
{
  CURL url2(url);
  ParseAndCorrectUrl(url2);

  CLog::Log(LOGDEBUG, "CDAVFile::Execute({}) {}", fmt::ptr(this), m_url);

  assert(!(!m_state->m_easyHandle ^ !m_state->m_multiHandle));
  if( m_state->m_easyHandle == NULL )
    g_curlInterface.easy_acquire(url2.GetProtocol().c_str(),
                                url2.GetHostName().c_str(),
                                &m_state->m_easyHandle,
                                &m_state->m_multiHandle);

  // setup common curl options
  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);

  m_lastResponseCode = m_state->Connect(m_bufferSize);
  if (m_lastResponseCode < 0 || m_lastResponseCode >= 400)
    return false;

  char* efurl;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_EFFECTIVE_URL,&efurl) && efurl)
    m_url = efurl;

  if (m_lastResponseCode == 207)
  {
    std::string strResponse;
    ReadData(strResponse);

    CXBMCTinyXML2 davResponse;
    davResponse.Parse(strResponse);

    if (!davResponse.Parse(strResponse))
    {
      CLog::Log(LOGERROR, "CDAVFile::Execute - Unable to process dav response ({})",
                CURL(m_url).GetRedacted());
      Close();
      return false;
    }

    // Iterate over all responses
    for (auto* child = davResponse.RootElement()->FirstChild(); child; child = child->NextSibling())
    {
      if (CDAVCommon::ValueWithoutNamespace(child, "response"))
      {
        std::string sRetCode = CDAVCommon::GetStatusTag(child->ToElement());
        CRegExp rxCode;
        rxCode.RegComp("HTTP/.+\\s(\\d+)\\s.*");
        if (rxCode.RegFind(sRetCode) >= 0)
        {
          if (rxCode.GetSubCount())
          {
            m_lastResponseCode = atoi(rxCode.GetMatch(1).c_str());
            if (m_lastResponseCode < 0 || m_lastResponseCode >= 400)
              return false;
          }
        }

      }
    }
  }

  return true;
}

bool CDAVFile::Delete(const CURL& url)
{
  if (m_opened)
    return false;

  CDAVFile dav;
  std::string strRequest = "DELETE";

  dav.SetCustomRequest(strRequest);

  CLog::Log(LOGDEBUG, "CDAVFile::Delete - Execute DELETE ({})", url.GetRedacted());
  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "CDAVFile::Delete - Unable to delete dav resource ({})", url.GetRedacted());
    return false;
  }

  dav.Close();

  return true;
}

bool CDAVFile::Rename(const CURL& url, const CURL& urlnew)
{
  if (m_opened)
    return false;

  CDAVFile dav;

  CURL url2(urlnew);
  std::string strProtocol = url2.GetTranslatedProtocol();
  url2.SetProtocol(strProtocol);

  std::string strRequest = "MOVE";
  dav.SetCustomRequest(strRequest);
  dav.SetRequestHeader("Destination", url2.GetWithoutUserDetails());

  CLog::Log(LOGDEBUG, "CDAVFile::Rename - Execute MOVE ({} -> {})", url.GetRedacted(),
            url2.GetRedacted());
  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "CDAVFile::Rename - Unable to rename dav resource ({} -> {})",
              url.GetRedacted(), url2.GetRedacted());
    return false;
  }

  dav.Close();

  return true;
}
