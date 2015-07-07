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

CDAVFile::~CDAVFile(void)
{
}

bool CDAVFile::Execute(const CURL& url)
{
  CURL url2(url);
  ParseAndCorrectUrl(url2);

  assert(!(!m_state->m_easyHandle ^ !m_state->m_multiHandle));
  if( m_state->m_easyHandle == NULL )
    g_curlInterface.easy_aquire(url2.GetProtocol().c_str(),
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

    m_davResponse.Clear();
    m_davResponse.Parse(strResponse);

    if (!m_davResponse.Parse(strResponse))
    {
      CLog::Log(LOGERROR, "CDAVFile::Execute - Unable to process dav response (%s)", CURL(m_url).GetRedacted().c_str());
      Close();
      return false;
    }

    TiXmlNode *pChild;
    // Iterate over all responses
    for (pChild = m_davResponse.RootElement()->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
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

/*
 * Parses a <response>
 *
 * <!ELEMENT response (href, ((href*, status)|(propstat+)), responsedescription?) >
 * <!ELEMENT propstat (prop, status, responsedescription?) >
 *
 */
bool CDAVFile::ParseResponse(const TiXmlElement *pElement, struct __stat64* statBuffer)
{
  const TiXmlElement *pResponseChild;
  const TiXmlNode *pPropstatChild;
  const TiXmlElement *pPropChild;

  /* Iterate response children elements */
  for (pResponseChild = pElement->FirstChildElement(); pResponseChild != 0; pResponseChild = pResponseChild->NextSiblingElement())
  {
    if (CDAVCommon::ValueWithoutNamespace(pResponseChild, "propstat"))
    {
      if (CDAVCommon::GetStatusTag(pResponseChild->ToElement()) == "HTTP/1.1 200 OK")
      {
        if (!statBuffer) // If caller is not interested in the rest, we're done
          return true;

        /* Iterate propstat children elements */
        for (pPropstatChild = pResponseChild->FirstChild(); pPropstatChild != 0; pPropstatChild = pPropstatChild->NextSibling())
        {
          if (CDAVCommon::ValueWithoutNamespace(pPropstatChild, "prop"))
          {
            memset(statBuffer, 0, sizeof(struct __stat64));

            /* Iterate all properties available */
            for (pPropChild = pPropstatChild->FirstChildElement(); pPropChild != 0; pPropChild = pPropChild->NextSiblingElement())
            {
              if (CDAVCommon::ValueWithoutNamespace(pPropChild, "getcontentlength") && !pPropChild->NoChildren())
              {
                statBuffer->st_size = strtoll(pPropChild->FirstChild()->Value(), NULL, 10);
              }
              else
              if (CDAVCommon::ValueWithoutNamespace(pPropChild, "getlastmodified") && !pPropChild->NoChildren())
              {
                struct tm timeDate = { 0 };
                strptime(pPropChild->FirstChild()->Value(), "%a, %d %b %Y %T", &timeDate);
                statBuffer->st_mtime = mktime(&timeDate);
              }
              else
              if (statBuffer->st_mtime == 0 && CDAVCommon::ValueWithoutNamespace(pPropChild, "creationdate") && !pPropChild->NoChildren())
              {
                struct tm timeDate = { 0 };
                strptime(pPropChild->FirstChild()->Value(), "%Y-%m-%dT%T", &timeDate);
                statBuffer->st_mtime = mktime(&timeDate);
              }
              else
              if (CDAVCommon::ValueWithoutNamespace(pPropChild, "resourcetype"))
              {
                if (CDAVCommon::ValueWithoutNamespace(pPropChild->FirstChild(), "collection"))
                  statBuffer->st_mode = _S_IFDIR;
                else
                  statBuffer->st_mode = _S_IFREG;
              }
            }

            return true;
          }
        }
      }
    }
  }

  return false;
}

int CDAVFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CDAVFile dav;
  std::string strRequest = "PROPFIND";
  dav.SetCustomRequest(strRequest);
  dav.SetRequestHeader("depth", 0);
  dav.SetMimeType("text/xml; charset=\"utf-8\"");

  if (buffer)
  {
    CLog::Log(LOGDEBUG, "CDAVFile::Stat - Execute STAT (%s)", url.GetRedacted().c_str());

    dav.SetPostData(
      "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
      " <D:propfind xmlns:D=\"DAV:\">"
      "   <D:prop>"
      "     <D:resourcetype/>"
      "     <D:getcontentlength/>"
      "     <D:getlastmodified/>"
      "     <D:creationdate/>"
      "     <D:displayname/>"
      "    </D:prop>"
      "  </D:propfind>");
  }
  else
  {
    dav.SetPostData(
      "<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
  }

  if (!dav.Execute(url))
  {
    if (buffer)
      CLog::Log(LOGERROR, "CDAVFile::Stat - Unable to stat resource (%s) with code %i", url.GetRedacted().c_str(), m_lastResponseCode);

    return -1;
  }

  dav.Close();

  // Iterate over all responses
  for (TiXmlNode *pChild = m_davResponse.RootElement()->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
  {
    if (CDAVCommon::ValueWithoutNamespace(pChild, "response"))
    {
      if (ParseResponse(pChild->ToElement(), buffer))
        return 0;
      else
        return -1;
    }
  }

  return -1;
}

bool CDAVFile::Exists(const CURL& url)
{
  if (Stat(url, NULL) != 0)
  {
    if (m_lastResponseCode >= 400 && m_lastResponseCode != 404)
      CLog::Log(LOGERROR, "CDAVFile::Exists - Unable to stat resource (%s) with code %i", url.GetRedacted().c_str(), m_lastResponseCode);

    return false;
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
