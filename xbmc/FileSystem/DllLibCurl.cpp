#include "stdafx.h"
#include "DllLibCurl.h"

using namespace XCURL;

/* okey this is damn ugly. our dll loader doesn't allow for postload, preunload functions */
static long g_curlReferences = 0;
DllLibCurlGlobal g_curlInterface;



bool DllLibCurlGlobal::Load()
{
  CSingleLock lock(m_critSection);
  if(++g_curlReferences > 1)
    return true;

  if (!DllDynamic::Load())
  {
    g_curlReferences = 0;
    return false;
  }

  if (global_init(CURL_GLOBAL_WIN32))
  {
    DllDynamic::Unload();
    CLog::Log(LOGERROR, "Error inializing libcurl");
    g_curlReferences = 0;
    return false;
  }

  return true;
}

void DllLibCurlGlobal::Unload()
{
  CSingleLock lock(m_critSection);
  if (--g_curlReferences == 0)
  {
    if (!IsLoaded())
      return;

    // close libcurl
    global_cleanup();

    DllDynamic::Unload();
  }
}

void DllLibCurlGlobal::CheckIdle()
{
  CSingleLock lock(m_critSection);
  /* 5 seconds idle time before closing handle */
  const DWORD idletime = 5000;

  VEC_CURLSESSIONS::iterator it = m_sessions.begin();
  while(it != m_sessions.end())
  {
    if( !it->m_busy && it->m_idletimestamp + idletime < GetTickCount())
    {
      CLog::Log(LOGINFO, "%s - Closing session to %s://%s\n", __FUNCTION__, it->m_protocol.c_str(), it->m_hostname.c_str());

      if(it->m_easy)
        easy_cleanup(it->m_easy);
      if(it->m_multi)
        multi_cleanup(it->m_multi);

      Unload();

      it = m_sessions.erase(it);
      continue;
    }
    it++;
  }
}

void DllLibCurlGlobal::easy_aquire(const char *protocol, const char *hostname, CURL_HANDLE** easy_handle, CURLM** multi_handle)
{
  assert(easy_handle != NULL);

  CSingleLock lock(m_critSection);

  VEC_CURLSESSIONS::iterator it;
  for(it = m_sessions.begin(); it != m_sessions.end(); it++)
  {
    if( !it->m_busy )
    {
      /* allow reuse of requester is trying to connect to same host */
      /* curl will take care of any differences in username/password */
      if( it->m_protocol.compare(protocol) == 0 && it->m_hostname.compare(hostname) == 0)
      {
        it->m_busy = true;
        if(easy_handle)
        {
          if(!it->m_easy)
            it->m_easy = easy_init();

          *easy_handle = it->m_easy;
        }

        if(multi_handle)
        {
          if(!it->m_multi)
            it->m_multi = multi_init();

          *multi_handle = it->m_multi;
        }

        return;
      }
    }
  }

  SSession session = {};
  session.m_busy = true;
  session.m_protocol = protocol;
  session.m_hostname = hostname;

  /* count up global interface counter */
  Load();

  if(easy_handle)
  {
    session.m_easy = easy_init();
    *easy_handle = session.m_easy;
  }

  if(multi_handle)
  {
    session.m_multi = multi_init();
    *multi_handle = session.m_multi;
  }

  m_sessions.push_back(session);


  CLog::Log(LOGINFO, "%s - Created session to %s://%s\n", __FUNCTION__, protocol, hostname);

  return;

}

void DllLibCurlGlobal::easy_release(CURL_HANDLE* easy_handle, CURLM* multi_handle)
{
  CSingleLock lock(m_critSection);

  VEC_CURLSESSIONS::iterator it;
  for(it = m_sessions.begin(); it != m_sessions.end(); it++)
  {
    if( it->m_easy == easy_handle && (multi_handle == NULL || it->m_multi == multi_handle) )
    {
      /* reset session so next caller doesn't reuse options, only connections */
      /* will reset verbose too so it won't print that it closed connections on cleanup*/
      easy_reset(easy_handle);
      it->m_busy = false;
      it->m_idletimestamp = GetTickCount();
      return;
    }
  }
}

CURL_HANDLE* DllLibCurlGlobal::easy_duphandle(CURL_HANDLE* easy_handle)
{
  CSingleLock lock(m_critSection);
  
  VEC_CURLSESSIONS::iterator it;
  for(it = m_sessions.begin(); it != m_sessions.end(); it++)
  {
    if( it->m_easy == easy_handle )
    {
      SSession session = *it;
      session.m_easy = DllLibCurl::easy_duphandle(easy_handle);
      Load();
      m_sessions.push_back(session);
      return session.m_easy;
    }
  }
  return DllLibCurl::easy_duphandle(easy_handle);
}
