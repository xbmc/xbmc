/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "threads/CriticalSection.h"

#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <type_traits>
#include <vector>

/* put types of curl in namespace to avoid namespace pollution */
namespace XCURL
{
#define CURL CURL_HANDLE
#include <curl/curl.h>
#undef CURL

class DllLibCurl
{
public:
  CURLcode global_init(long flags);
  void global_cleanup();
  CURL_HANDLE* easy_init();
  template<typename... Args>
  CURLcode easy_setopt(CURL_HANDLE* handle, CURLoption option, Args... args)
  {
    return curl_easy_setopt(handle, option, std::forward<Args>(args)...);
  }
  CURLcode easy_perform(CURL_HANDLE* handle);
  CURLcode easy_pause(CURL_HANDLE* handle, int bitmask);
  void easy_reset(CURL_HANDLE* handle);
  template<typename... Args>
  CURLcode easy_getinfo(CURL_HANDLE* curl, CURLINFO info, Args... args)
  {
    return curl_easy_getinfo(curl, info, std::forward<Args>(args)...);
  }
  void easy_cleanup(CURL_HANDLE* handle);
  virtual CURL_HANDLE* easy_duphandle(CURL_HANDLE* handle);
  CURLM* multi_init(void);
  CURLMcode multi_add_handle(CURLM* multi_handle, CURL_HANDLE* easy_handle);
  CURLMcode multi_perform(CURLM* multi_handle, int* running_handles);
  CURLMcode multi_remove_handle(CURLM* multi_handle, CURL_HANDLE* easy_handle);
  CURLMcode multi_fdset(CURLM* multi_handle,
                        fd_set* read_fd_set,
                        fd_set* write_fd_set,
                        fd_set* exc_fd_set,
                        int* max_fd);
  CURLMcode multi_timeout(CURLM* multi_handle, long* timeout);
  CURLMsg* multi_info_read(CURLM* multi_handle, int* msgs_in_queue);
  CURLMcode multi_cleanup(CURLM* handle);
  struct curl_slist* slist_append(struct curl_slist* list, const char* to_append);
  void slist_free_all(struct curl_slist* list);
  const char* easy_strerror(CURLcode code);
};

class DllLibCurlGlobal : public DllLibCurl
{
public:
  DllLibCurlGlobal();
  ~DllLibCurlGlobal();
  /* extend interface with buffered functions */
  void easy_acquire(const char* protocol,
                    const char* hostname,
                    CURL_HANDLE** easy_handle,
                    CURLM** multi_handle);
  void easy_release(CURL_HANDLE** easy_handle, CURLM** multi_handle);
  void easy_duplicate(CURL_HANDLE* easy, CURLM* multi, CURL_HANDLE** easy_out, CURLM** multi_out);
  CURL_HANDLE* easy_duphandle(CURL_HANDLE* easy_handle) override;
  void CheckIdle();

  /* overloaded load and unload with reference counter */

  /* structure holding a session info */
  typedef struct SSession
  {
    unsigned int m_idletimestamp; // timestamp of when this object when idle
    std::string m_protocol;
    std::string m_hostname;
    bool m_busy;
    CURL_HANDLE* m_easy;
    CURLM* m_multi;
  } SSession;

  typedef std::vector<SSession> VEC_CURLSESSIONS;

  VEC_CURLSESSIONS m_sessions;
  CCriticalSection m_critSection;
};
} // namespace XCURL

extern XCURL::DllLibCurlGlobal g_curlInterface;
