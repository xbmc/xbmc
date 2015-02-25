/*
 *      Copyright (C) 2015 Team XBMC
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

#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "HTTPSessionManager.h"
#include "interfaces/legacy/mod_python/HttpRequest.h"
#include "interfaces/legacy/mod_python/Session.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/md5.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"

#define SESSION_SUFFIX  "sid"

using namespace XBMCAddon::xbmcmod_python;

CHTTPSessionManager::CHTTPSessionManager()
{ }

CHTTPSessionManager::~CHTTPSessionManager()
{
  CSingleLock lock(m_critSection);
  for (std::map<std::string, Session*>::const_iterator session = m_sessions.begin(); session != m_sessions.end(); ++session)
    delete session->second;
  m_sessions.clear();
}

CHTTPSessionManager& CHTTPSessionManager::Get()
{
  static CHTTPSessionManager m_instance;
  return m_instance;
}

bool CHTTPSessionManager::IsValidSessionId(const std::string& sessionId)
{
  return sessionId.size() == 32 &&
         sessionId.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") == std::string::npos;
}

std::string CHTTPSessionManager::GenerateSessionId(XBMCAddon::xbmcmod_python::HttpRequest* request)
{
  if (request == NULL || request->m_request == NULL || request->m_request->connection == NULL)
    return "";

  // get the ip address of the requester
  const MHD_ConnectionInfo *ipInfo = MHD_get_connection_info(request->m_request->connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

  // generate a new session id
  std::string sessionId;
  int count = 0;
  CSingleLock lock(m_critSection);
  do
  {
    std::string temp = StringUtils::Format("%ld%d%d%s", time(NULL) * (10000 + count), rand(), rand(),
                                           inet_ntoa(reinterpret_cast<struct sockaddr_in* const>(ipInfo->client_addr)->sin_addr));
    sessionId = XBMC::XBMC_MD5::GetMD5(temp);

    count += 1;
  } while (m_sessions.find(sessionId) != m_sessions.end());

  // add the session id with a new session to prevent the session id from being generated multiple times
  m_sessions.insert(std::make_pair(sessionId, new Session(sessionId)));

  return sessionId;
}

const XBMCAddon::xbmcmod_python::Session* CHTTPSessionManager::Get(XBMCAddon::xbmcmod_python::HttpRequest* request, const std::string& sessionId /* = "" */)
{
  CDateTime now = CDateTime::GetUTCDateTime();

  std::string sessionIdTmp = sessionId;
  if (!IsValidSessionId(sessionIdTmp))
  {
    if (!sessionIdTmp.empty())
      CLog::Log(LOGWARNING, "CHTTPSessionManager: invalid session id \"%s\" passed in", sessionIdTmp.c_str());

    sessionIdTmp.clear();
  }
  else
  {
    // check if we got a valid and known session id
    CSingleLock lock(m_critSection);
    std::map<std::string, Session*>::iterator it = m_sessions.find(sessionIdTmp);
    // the session has already been loaded before so let's just return it
    if (it != m_sessions.end())
    {
      if (it->second->IsValid())
      {
        it->second->m_isNew = false;
        return it->second;
      }
      else
      {
        CLog::Log(LOGWARNING, "CHTTPSessionManager: invalid session with id \"%s\" removed", sessionIdTmp.c_str());
        m_sessions.erase(it);
      }
    }
    else
      CLog::Log(LOGWARNING, "CHTTPSessionManager: unknown session id \"%s\" passed in", sessionIdTmp.c_str());
  }

  if (request == NULL || request->m_request == NULL || request->m_request->connection == NULL)
    return NULL;

  // support "sid=<session id>" as a GET parameter
  if (sessionIdTmp.empty())
  {
    std::map<std::string, std::string>::const_iterator getSid = request->m_request->getValues.find(SESSION_SUFFIX);
    if (getSid != request->m_request->getValues.end())
    {
      sessionIdTmp = getSid->second;
      if (!IsValidSessionId(sessionIdTmp))
      {
        CLog::Log(LOGWARNING, "CHTTPSessionManager: invalid session id \"%s\" in GET parameter " SESSION_SUFFIX " received", sessionIdTmp.c_str());
        sessionIdTmp.clear();
      }
    }
  }

  std::string sessionIdCookieName = getSessionIdCookieName();
  // support "<app name>sid" as a Cookie value
  if (sessionIdTmp.empty())
  {
    // get all cookies
    std::string cookieData;
    std::pair<std::multimap<std::string, std::string>::const_iterator, std::multimap<std::string, std::string>::const_iterator> range
      = request->m_request->headerValues.equal_range(MHD_HTTP_HEADER_COOKIE);
    for (std::multimap<std::string, std::string>::const_iterator it = range.first; it != range.second; it++)
    {
      cookieData.append(it->second);
      cookieData.push_back(';'); // make sure two different cookies are seperated
    }

    std::vector<std::string> parts = StringUtils::Split(cookieData, ";");
    for (std::vector<std::string>::const_iterator part = parts.begin(); part != parts.end(); ++part)
    {
      std::string data = *part;
      StringUtils::Trim(data);
      if (!StringUtils::StartsWith(data, sessionIdCookieName))
        continue;

      // extract the session id
      sessionIdTmp = data.substr(sessionIdCookieName.size() + 1);
      StringUtils::Trim(sessionIdTmp);

      // make sure the session id is valid
      if (IsValidSessionId(sessionIdTmp))
        break;
    }
  }

  // check if we got a valid and known session id
  if (!IsValidSessionId(sessionIdTmp))
  {
    if (!sessionIdTmp.empty())
      CLog::Log(LOGWARNING, "CHTTPSessionManager: invalid session id \"%s\" in Cookie received", sessionIdTmp.c_str());

    return NULL;
  }

  CSingleLock lock(m_critSection);
  std::map<std::string, Session*>::iterator it = m_sessions.find(sessionIdTmp);
  // the session has already been loaded before so let's just return it
  if (it == m_sessions.end())
  {
    CLog::Log(LOGWARNING, "CHTTPSessionManager: unknown session id \"%s\" received", sessionIdTmp.c_str());
    return NULL;
  }

  // check if the session is still valid and remove it if not
  if (!it->second->IsValid())
  {
    CLog::Log(LOGWARNING, "CHTTPSessionManager: invalid session with id \"%s\" removed", sessionIdTmp.c_str());
    m_sessions.erase(it);

    return NULL;
  }

  it->second->m_isNew = false;
  return it->second;
}

bool CHTTPSessionManager::Save(XBMCAddon::xbmcmod_python::Session* session)
{
  if (session == NULL || !session->IsValid(true))
    return false;

  const std::string& sessionId = session->id();

  CSingleLock lock(m_critSection);
  // check if we already know the session
  std::map<std::string, Session*>::iterator it = m_sessions.find(sessionId);
  if (it == m_sessions.end())
  {
    // we don't know about this session yet so store a copy of it
    m_sessions.insert(std::make_pair(sessionId, new Session(session)));
    it = m_sessions.find(sessionId);
  }
  else
    it->second->Copy(session);

  // set the "Set-Cookie" header field
  session->m_request->m_request->responseHeaders.insert(std::make_pair(MHD_HTTP_HEADER_SET_COOKIE, getSessionIdCookie(sessionId, it->second->timeout())));

  return true;
}

void CHTTPSessionManager::Remove(const std::string& sessionId, bool invalidate /* = false */)
{
  if (sessionId.empty())
    return;

  CSingleLock lock(m_critSection);
  std::map<std::string, Session*>::iterator it = m_sessions.find(sessionId);
  if (it == m_sessions.end())
    return;

  // make sure to delete the cookie
  if (invalidate && it->second->m_request != NULL && it->second->m_request->m_request != NULL)
    it->second->m_request->m_request->responseHeaders.insert(std::make_pair(MHD_HTTP_HEADER_SET_COOKIE, getSessionIdCookie(sessionId, 0)));

  m_sessions.erase(sessionId);
}

std::string CHTTPSessionManager::getSessionIdCookie(const std::string& sessionId, int maxAge /* = -1 */)
{
  if (sessionId.empty())
    return "";

  std::string cookie = getSessionIdCookieName() + "=" + sessionId;
  if (maxAge >= 0)
    cookie += StringUtils::Format(";Max-Age=%d", maxAge);

  return cookie;
}

std::string CHTTPSessionManager::getSessionIdCookieName()
{
  std::string sessionIdCookieName = CSysInfo::GetAppName() + SESSION_SUFFIX;
  StringUtils::ToLower(sessionIdCookieName);

  return sessionIdCookieName;
}
