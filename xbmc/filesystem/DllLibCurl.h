#pragma once
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

#include "DynamicDll.h"
#include "threads/CriticalSection.h"
#include <stdio.h>
#include <vector>

/* put types of curl in namespace to avoid namespace pollution */
namespace XCURL
{
  #define CURL CURL_HANDLE
  #include <curl/curl.h>
  #undef CURL

  class DllLibCurlInterface
  {
  public:
    virtual ~DllLibCurlInterface() {}
    virtual CURLcode global_init(long flags)=0;
    virtual void global_cleanup(void)=0;
    virtual CURL_HANDLE * easy_init(void)=0;
    //virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, ...)=0;
    virtual CURLcode easy_perform(CURL_HANDLE * handle )=0;
    virtual CURLcode easy_pause(CURL_HANDLE * handle, int bitmask )=0;
    virtual void easy_reset(CURL_HANDLE * handle)=0;
    //virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, ... )=0;
    virtual void easy_cleanup(CURL_HANDLE * handle )=0;
    virtual CURL_HANDLE *easy_duphandle(CURL_HANDLE *handle )=0;
    virtual CURLM * multi_init(void)=0;
    virtual CURLMcode multi_add_handle(CURLM *multi_handle, CURL_HANDLE *easy_handle)=0;
    virtual CURLMcode multi_perform(CURLM *multi_handle, int *running_handles)=0;
    virtual CURLMcode multi_remove_handle(CURLM *multi_handle, CURL_HANDLE *easy_handle)=0;
    virtual CURLMcode multi_fdset(CURLM *multi_handle, fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set, int *max_fd)=0;
    virtual CURLMcode multi_timeout(CURLM *multi_handle, long *timeout)=0;
    virtual CURLMsg*  multi_info_read(CURLM *multi_handle, int *msgs_in_queue)=0;
    virtual CURLMcode multi_cleanup(CURLM * handle )=0;
    virtual struct curl_slist* slist_append(struct curl_slist *, const char *)=0;
    virtual void  slist_free_all(struct curl_slist *)=0;
  };

  class DllLibCurl : public DllDynamic, DllLibCurlInterface
  {
    DECLARE_DLL_WRAPPER(DllLibCurl, DLL_PATH_LIBCURL)
    DEFINE_METHOD1(CURLcode, global_init, (long p1))
    DEFINE_METHOD0(void, global_cleanup)
    DEFINE_METHOD0(CURL_HANDLE *, easy_init)
    DEFINE_METHOD_FP(CURLcode, easy_setopt, (CURL_HANDLE *p1, CURLoption p2, ...))
    DEFINE_METHOD1(CURLcode, easy_perform, (CURL_HANDLE * p1 ))
    DEFINE_METHOD2(CURLcode, easy_pause, (CURL_HANDLE * p1, int p2 ))
    DEFINE_METHOD1(void, easy_reset, (CURL_HANDLE * p1 ))
    DEFINE_METHOD_FP(CURLcode, easy_getinfo, (CURL_HANDLE *p1, CURLINFO p2, ... ))
    DEFINE_METHOD1(void, easy_cleanup, (CURL_HANDLE * p1))
    DEFINE_METHOD1(CURL_HANDLE *, easy_duphandle, (CURL_HANDLE * p1))
    DEFINE_METHOD0(CURLM *, multi_init)
    DEFINE_METHOD2(CURLMcode, multi_add_handle, (CURLM *p1, CURL_HANDLE *p2))
    DEFINE_METHOD2(CURLMcode, multi_perform, (CURLM *p1, int *p2))
    DEFINE_METHOD2(CURLMcode, multi_remove_handle, (CURLM *p1, CURL_HANDLE *p2))
    DEFINE_METHOD5(CURLMcode, multi_fdset, (CURLM *p1, fd_set *p2, fd_set *p3, fd_set *p4, int *p5))
    DEFINE_METHOD2(CURLMcode, multi_timeout, (CURLM *p1, long *p2))
    DEFINE_METHOD2(CURLMsg*,  multi_info_read, (CURLM *p1, int *p2))
    DEFINE_METHOD1(CURLMcode, multi_cleanup, (CURLM *p1))
    DEFINE_METHOD2(struct curl_slist*, slist_append, (struct curl_slist * p1, const char * p2))
    DEFINE_METHOD1(void, slist_free_all, (struct curl_slist * p1))
    DEFINE_METHOD1(const char *, easy_strerror, (CURLcode p1))
#if defined(HAS_CURL_STATIC)
    DEFINE_METHOD1(void, crypto_set_id_callback, (unsigned long (*p1)(void)))
    DEFINE_METHOD1(void, crypto_set_locking_callback, (void (*p1)(int, int, const char *, int)))
#endif
    BEGIN_METHOD_RESOLVE()
      RESOLVE_METHOD_RENAME(curl_global_init, global_init)
      RESOLVE_METHOD_RENAME(curl_global_cleanup, global_cleanup)
      RESOLVE_METHOD_RENAME(curl_easy_init, easy_init)
      RESOLVE_METHOD_RENAME(curl_easy_strerror, easy_strerror)
      RESOLVE_METHOD_RENAME_FP(curl_easy_setopt, easy_setopt)
      RESOLVE_METHOD_RENAME(curl_easy_perform, easy_perform)
      RESOLVE_METHOD_RENAME(curl_easy_pause, easy_pause)
      RESOLVE_METHOD_RENAME(curl_easy_reset, easy_reset)
      RESOLVE_METHOD_RENAME_FP(curl_easy_getinfo, easy_getinfo)
      RESOLVE_METHOD_RENAME(curl_easy_cleanup, easy_cleanup)
      RESOLVE_METHOD_RENAME(curl_easy_duphandle, easy_duphandle)
      RESOLVE_METHOD_RENAME(curl_multi_init, multi_init)
      RESOLVE_METHOD_RENAME(curl_multi_add_handle, multi_add_handle)
      RESOLVE_METHOD_RENAME(curl_multi_perform, multi_perform)
      RESOLVE_METHOD_RENAME(curl_multi_remove_handle, multi_remove_handle)
      RESOLVE_METHOD_RENAME(curl_multi_fdset, multi_fdset)
      RESOLVE_METHOD_RENAME(curl_multi_timeout, multi_timeout)
      RESOLVE_METHOD_RENAME(curl_multi_info_read, multi_info_read)
      RESOLVE_METHOD_RENAME(curl_multi_cleanup, multi_cleanup)
      RESOLVE_METHOD_RENAME(curl_slist_append, slist_append)
      RESOLVE_METHOD_RENAME(curl_slist_free_all, slist_free_all)
#if defined(HAS_CURL_STATIC)
      RESOLVE_METHOD_RENAME(CRYPTO_set_id_callback, crypto_set_id_callback)
      RESOLVE_METHOD_RENAME(CRYPTO_set_locking_callback, crypto_set_locking_callback)
#endif
    END_METHOD_RESOLVE()

  };

  class DllLibCurlGlobal : public DllLibCurl
  {
  public:
    /* extend interface with buffered functions */
    void easy_aquire(const char *protocol, const char *hostname, CURL_HANDLE** easy_handle, CURLM** multi_handle);
    void easy_release(CURL_HANDLE** easy_handle, CURLM** multi_handle);
    void easy_duplicate(CURL_HANDLE* easy, CURLM* multi, CURL_HANDLE** easy_out, CURLM** multi_out);
    CURL_HANDLE* easy_duphandle(CURL_HANDLE* easy_handle);
    void CheckIdle();

    /* overloaded load and unload with reference counter */
    virtual bool Load();
    virtual void Unload();

    /* structure holding a session info */
    typedef struct SSession
    {
      unsigned int  m_idletimestamp;  // timestamp of when this object when idle
      std::string   m_protocol;
      std::string   m_hostname;
      bool          m_busy;
      CURL_HANDLE*  m_easy;
      CURLM*        m_multi;
    } SSession;

    typedef std::vector<SSession> VEC_CURLSESSIONS;

    VEC_CURLSESSIONS m_sessions;
    CCriticalSection m_critSection;
  };
}

extern XCURL::DllLibCurlGlobal g_curlInterface;
