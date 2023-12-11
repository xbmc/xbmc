/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DllLibCurl.h"

#include "threads/SystemClock.h"
#include "utils/log.h"

#include <assert.h>
#include <mutex>

namespace XCURL
{
CURLcode DllLibCurl::global_init(long flags)
{
  return curl_global_init(flags);
}

void DllLibCurl::global_cleanup()
{
  curl_global_cleanup();
}

CURL_HANDLE* DllLibCurl::easy_init()
{
  return curl_easy_init();
}

CURLcode DllLibCurl::easy_perform(CURL_HANDLE* handle)
{
  return curl_easy_perform(handle);
}

CURLcode DllLibCurl::easy_pause(CURL_HANDLE* handle, int bitmask)
{
  return curl_easy_pause(handle, bitmask);
}

void DllLibCurl::easy_reset(CURL_HANDLE* handle)
{
  curl_easy_reset(handle);
}

void DllLibCurl::easy_cleanup(CURL_HANDLE* handle)
{
  curl_easy_cleanup(handle);
}

CURL_HANDLE* DllLibCurl::easy_duphandle(CURL_HANDLE* handle)
{
  return curl_easy_duphandle(handle);
}

CURLM* DllLibCurl::multi_init()
{
  return curl_multi_init();
}

CURLMcode DllLibCurl::multi_add_handle(CURLM* multi_handle, CURL_HANDLE* easy_handle)
{
  return curl_multi_add_handle(multi_handle, easy_handle);
}

CURLMcode DllLibCurl::multi_perform(CURLM* multi_handle, int* running_handles)
{
  return curl_multi_perform(multi_handle, running_handles);
}

CURLMcode DllLibCurl::multi_remove_handle(CURLM* multi_handle, CURL_HANDLE* easy_handle)
{
  return curl_multi_remove_handle(multi_handle, easy_handle);
}

CURLMcode DllLibCurl::multi_fdset(
    CURLM* multi_handle, fd_set* read_fd_set, fd_set* write_fd_set, fd_set* exc_fd_set, int* max_fd)
{
  return curl_multi_fdset(multi_handle, read_fd_set, write_fd_set, exc_fd_set, max_fd);
}

CURLMcode DllLibCurl::multi_timeout(CURLM* multi_handle, long* timeout)
{
  return curl_multi_timeout(multi_handle, timeout);
}

CURLMsg* DllLibCurl::multi_info_read(CURLM* multi_handle, int* msgs_in_queue)
{
  return curl_multi_info_read(multi_handle, msgs_in_queue);
}

CURLMcode DllLibCurl::multi_cleanup(CURLM* handle)
{
  return curl_multi_cleanup(handle);
}

curl_slist* DllLibCurl::slist_append(curl_slist* list, const char* to_append)
{
  return curl_slist_append(list, to_append);
}

void DllLibCurl::slist_free_all(curl_slist* list)
{
  curl_slist_free_all(list);
}

const char* DllLibCurl::easy_strerror(CURLcode code)
{
  return curl_easy_strerror(code);
}

DllLibCurlGlobal::DllLibCurlGlobal()
{
  /* we handle this ourself */
  if (curl_global_init(CURL_GLOBAL_ALL))
  {
    CLog::Log(LOGERROR, "Error initializing libcurl");
  }
}

DllLibCurlGlobal::~DllLibCurlGlobal()
{
  for (auto& session : m_sessions)
  {
    if (session.m_multi && session.m_easy)
      multi_remove_handle(session.m_multi, session.m_easy);
    if (session.m_easy)
      easy_cleanup(session.m_easy);
    if (session.m_multi)
      multi_cleanup(session.m_multi);
  }
  // close libcurl
  curl_global_cleanup();
}

void DllLibCurlGlobal::CheckIdle()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  /* 20 seconds idle time before closing handle */
  const unsigned int idletime = 30000;

  VEC_CURLSESSIONS::iterator it = m_sessions.begin();
  while (it != m_sessions.end())
  {
    auto now = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - it->m_idletimestamp);

    if (!it->m_busy && duration.count() > idletime)
    {
      CLog::Log(LOGDEBUG, "{} - Closing session to {}://{} (easy={}, multi={})", __FUNCTION__,
                it->m_protocol, it->m_hostname, fmt::ptr(it->m_easy), fmt::ptr(it->m_multi));

      if (it->m_multi && it->m_easy)
        multi_remove_handle(it->m_multi, it->m_easy);
      if (it->m_easy)
        easy_cleanup(it->m_easy);
      if (it->m_multi)
        multi_cleanup(it->m_multi);

      it = m_sessions.erase(it);
      continue;
    }
    ++it;
  }
}

void DllLibCurlGlobal::easy_acquire(const char* protocol,
                                    const char* hostname,
                                    CURL_HANDLE** easy_handle,
                                    CURLM** multi_handle)
{
  assert(easy_handle != NULL);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (auto& it : m_sessions)
  {
    if (!it.m_busy)
    {
      /* allow reuse of requester is trying to connect to same host */
      /* curl will take care of any differences in username/password */
      if (it.m_protocol.compare(protocol) == 0 && it.m_hostname.compare(hostname) == 0)
      {
        it.m_busy = true;
        if (easy_handle)
        {
          if (!it.m_easy)
            it.m_easy = easy_init();

          *easy_handle = it.m_easy;
        }

        if (multi_handle)
        {
          if (!it.m_multi)
            it.m_multi = multi_init();

          *multi_handle = it.m_multi;
        }

        return;
      }
    }
  }

  SSession session = {};
  session.m_busy = true;
  session.m_protocol = protocol;
  session.m_hostname = hostname;

  if (easy_handle)
  {
    session.m_easy = easy_init();
    *easy_handle = session.m_easy;
  }

  if (multi_handle)
  {
    session.m_multi = multi_init();
    *multi_handle = session.m_multi;
  }

  m_sessions.push_back(session);

  CLog::Log(LOGDEBUG, "{} - Created session to {}://{}", __FUNCTION__, protocol, hostname);
}

void DllLibCurlGlobal::easy_release(CURL_HANDLE** easy_handle, CURLM** multi_handle)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CURL_HANDLE* easy = NULL;
  CURLM* multi = NULL;

  if (easy_handle)
  {
    easy = *easy_handle;
    *easy_handle = NULL;
  }

  if (multi_handle)
  {
    multi = *multi_handle;
    *multi_handle = NULL;
  }

  for (auto& it : m_sessions)
  {
    if (it.m_easy == easy && (multi == nullptr || it.m_multi == multi))
    {
      /* reset session so next caller doesn't reuse options, only connections */
      /* will reset verbose too so it won't print that it closed connections on cleanup*/
      easy_reset(easy);
      it.m_busy = false;
      it.m_idletimestamp = std::chrono::steady_clock::now();
      return;
    }
  }
}

CURL_HANDLE* DllLibCurlGlobal::easy_duphandle(CURL_HANDLE* easy_handle)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& it : m_sessions)
  {
    if (it.m_easy == easy_handle)
    {
      SSession session = it;
      session.m_easy = DllLibCurl::easy_duphandle(easy_handle);
      m_sessions.push_back(session);
      return session.m_easy;
    }
  }
  return DllLibCurl::easy_duphandle(easy_handle);
}

void DllLibCurlGlobal::easy_duplicate(CURL_HANDLE* easy,
                                      const CURLM* multi,
                                      CURL_HANDLE** easy_out,
                                      CURLM** multi_out)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (easy_out && easy)
    *easy_out = DllLibCurl::easy_duphandle(easy);

  if (multi_out && multi)
    *multi_out = DllLibCurl::multi_init();

  for (const auto& it : m_sessions)
  {
    if (it.m_easy == easy)
    {
      SSession session = it;
      if (easy_out && easy)
        session.m_easy = *easy_out;
      else
        session.m_easy = NULL;

      if (multi_out && multi)
        session.m_multi = *multi_out;
      else
        session.m_multi = NULL;

      m_sessions.push_back(session);
      return;
    }
  }
}
} // namespace XCURL
