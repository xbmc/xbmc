#pragma once
#include "../stdafx.h"
#include "DllLibCurl.h"

using namespace XCURL;

/* okey this is damn ugly. our dll loader doesn't allow for postload, preunload functions */
static long g_curlReferences = 0;
DllLibCurlGlobal g_curlInterface;



bool DllLibCurlGlobal::Load()
{
  if(InterlockedIncrement(&g_curlReferences) < 2)
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

  InterlockedIncrement(&g_curlReferences);
  return true;
}

void DllLibCurlGlobal::Unload()
{
  if( InterlockedDecrement(&g_curlReferences) == 0 )
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
  /* 5 seconds idle time before closing handle */
  const DWORD idletime = 5000;

	VEC_CURLSESSIONS::iterator it = m_sessions.begin();
	while(it != m_sessions.end())
	{
    if( !it->m_busy && it->m_idletimestamp + idletime < GetTickCount())
    {
      CLog::Log(LOGINFO, __FUNCTION__" - Closing session to %s://%s\n", it->m_protocol.c_str(), it->m_hostname.c_str());

      g_curlInterface.easy_cleanup(it->m_session);
      g_curlInterface.Unload();

      it = m_sessions.erase(it);
      continue;
    }
    it++;
  }
}

CURL_HANDLE* DllLibCurlGlobal::easy_aquire(const char *protocol, const char *hostname)
{
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
        return it->m_session;
      }
		}
	}

  SSession session;
  session.m_busy = true;
  session.m_idletimestamp = 0;
  session.m_protocol = protocol;
  session.m_hostname = hostname;

  /* count up global interface counter */
  Load();
  session.m_session = g_curlInterface.easy_init();

  m_sessions.push_back(session);


  CLog::Log(LOGINFO, __FUNCTION__" - Created session to %s://%s\n", protocol, hostname);

  return session.m_session;

}

void DllLibCurlGlobal::easy_release(XCURL::CURL_HANDLE* easy_handle)
{
	VEC_CURLSESSIONS::iterator it;
	for(it = m_sessions.begin(); it != m_sessions.end(); it++)
	{
		if( it->m_session == easy_handle )
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
