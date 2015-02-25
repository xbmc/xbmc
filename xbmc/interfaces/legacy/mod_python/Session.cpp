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

#include "Session.h"
#include "interfaces/legacy/Exception.h"
#include "network/httprequesthandler/python/HTTPSessionManager.h"

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    Session::Session(const String& sessionId)
      : m_request(NULL),
        m_sessionId(sessionId),
        m_isNew(true),
        m_created(CDateTime::GetUTCDateTime()),
        m_lastAccessed(m_created),
        m_timeout(SESSION_TIMEOUT_DEFAULT),
        m_map()
    { }

    Session::Session(const Session* session) throw(NullPointerException)
      : m_request(NULL),
        m_sessionId(),
        m_isNew(true),
        m_created(),
        m_lastAccessed(),
        m_timeout(SESSION_TIMEOUT_DEFAULT),
        m_map()
    {
      if (session == NULL)
        throw NullPointerException("session");

      Copy(session);
    }

    Session::Session(HttpRequest* request, const String& sid /* = "" */, int timeout /* = SESSION_TIMEOUT_DEFAULT */)  throw(NullPointerException)
      : m_request(request),
        m_sessionId(sid),
        m_isNew(true),
        m_created(CDateTime::GetUTCDateTime()),
        m_lastAccessed(m_created),
        m_timeout(timeout),
        m_map()
    {
      if (m_request == NULL || m_request->m_request == NULL || m_request->m_request->connection == NULL)
        throw NullPointerException("request");

      set_timeout(timeout);
    }

    Session::~Session()
    {
      m_request = NULL;
    }

    int Session::created()
    {
      time_t timestamp;
      m_created.GetAsTime(timestamp);

      return timestamp;
    }

    int Session::last_accessed()
    {
      time_t timestamp;
      m_lastAccessed.GetAsTime(timestamp);

      return timestamp;
    }

    void Session::set_timeout(int seconds) throw(InvalidArgumentException)
    {
      if (seconds < 0)
        throw InvalidArgumentException("seconds");

      m_timeout = seconds;
    }

    void Session::invalidate()
    {
      if (!IsValid(true))
        return;

      CHTTPSessionManager::Get().Remove(m_sessionId, true);
    }

    bool Session::load()
    {
      // check if the session already exists
      const Session* session = CHTTPSessionManager::Get().Get(m_request, m_sessionId);
      if (session != NULL && session->IsValid())
      {
        Copy(session);
        return true;
      }

      // create a new session id
      if (m_sessionId.empty())
        m_sessionId = CHTTPSessionManager::Get().GenerateSessionId(m_request);

      return false;
    }

    bool Session::save()
    {
      if (!IsValid(true))
        return false;

      // update last accessed and therefore the timeout
      m_lastAccessed = CDateTime::GetUTCDateTime();

      return CHTTPSessionManager::Get().Save(this);
    }

    void Session::del()
    {
      if (!IsValid(true))
        return;

      CHTTPSessionManager::Get().Remove(m_sessionId, false);
    }

    String Session::toString() const
    {
      return m_sessionId;
    }

    bool Session::IsValid(bool validateRequest /* = false */) const
    {
      // validate the request objects
      if (validateRequest &&
         (m_request == NULL || m_request->m_request == NULL || m_request->m_request->connection == NULL))
        return false;
     
      // validate the session id
      if (!CHTTPSessionManager::IsValidSessionId(m_sessionId))
        return false;

      // validate the last accessed timestamp with the timeout
      if (!m_lastAccessed.IsValid())
        return false;

      CDateTime expires = m_lastAccessed + CDateTimeSpan(0, 0, 0, m_timeout);
      if (expires <= CDateTime::GetUTCDateTime())
        return false;

      return true;
    }

    void Session::Copy(const Session* session)
    {
      if (session == NULL)
        return;

      m_sessionId = session->m_sessionId;
      m_isNew = session->m_isNew;

      m_created = session->m_created;
      m_lastAccessed = session->m_lastAccessed;
      m_timeout = session->m_timeout;

      m_map.clear();
      m_map.insert(session->m_map.begin(), session->m_map.end());
    }
  }
}
