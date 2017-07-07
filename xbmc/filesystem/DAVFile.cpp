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

#include "system.h"

#include "DAVFile.h"

#include "DAVCommon.h"
#include "URL.h"
#include "utils/log.h"
#include "DllLibCurl.h"
#include "utils/XBMCTinyXML.h"
#include "utils/RegExp.h"

using namespace XFILE;
using namespace XCURL;

CDAVFile::CDAVFile(void)
  : CCurlFile()
  , m_lastResponseCode(0)
{
}

CDAVFile::~CDAVFile(void) = default;

bool CDAVFile::Execute(const CURL& url)
{
  CURL url2(url);
  ParseAndCorrectUrl(url2);

  CLog::Log(LOGDEBUG, "CDAVFile::Execute(%p) %s", (void*)this, m_url.c_str());

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

    CXBMCTinyXML davResponse;
    davResponse.Parse(strResponse);

    if (!davResponse.Parse(strResponse))
    {
      CLog::Log(LOGERROR, "CDAVFile::Execute - Unable to process dav response (%s)", CURL(m_url).GetRedacted().c_str());
      Close();
      return false;
    }

    TiXmlNode *pChild;
    // Iterate over all responses
    for (pChild = davResponse.RootElement()->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
    {
      if (CDAVCommon::ValueWithoutNamespace(pChild, "response"))
      {
        std::string sRetCode = CDAVCommon::GetStatusTag(pChild->ToElement());
        CRegExp rxCode;
        rxCode.RegComp("HTTP/1\\.1\\s(\\d+)\\s.*"); 
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
 
  CLog::Log(LOGDEBUG, "CDAVFile::Delete - Execute DELETE (%s)", url.GetRedacted().c_str());
  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "CDAVFile::Delete - Unable to delete dav resource (%s)", url.GetRedacted().c_str());
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

  CLog::Log(LOGDEBUG, "CDAVFile::Rename - Execute MOVE (%s -> %s)", url.GetRedacted().c_str(), url2.GetRedacted().c_str());
  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "CDAVFile::Rename - Unable to rename dav resource (%s -> %s)", url.GetRedacted().c_str(), url2.GetRedacted().c_str());
    return false;
  }

  dav.Close();

  return true;
}
