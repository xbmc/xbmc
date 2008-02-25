#include "stdafx.h"
#include "DllLibCMyth.h"
#include "CMythSession.h"

extern "C" {
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
}

using namespace XFILE;
using namespace std;

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64 "I64d"
#else
#define PRId64 "lld"
#endif
#endif

CCriticalSection            CCMythSession::m_section_session;
std::vector<CCMythSession*> CCMythSession::m_sessions;

void CCMythSession::CheckIdle()
{
  CSingleLock lock(m_section_session);

  std::vector<CCMythSession*>::iterator it;
  for(it = m_sessions.begin(); it != m_sessions.end(); it++)
  {
    CCMythSession* session = *it;
    if(session->m_timestamp + 5000 < GetTickCount())
    {
      CLog::Log(LOGINFO, "%s - Closing idle connection to mythtv backend %s", __FUNCTION__, session->m_hostname.c_str());
      delete session;
      it = m_sessions.erase(it)-1;
    }
  }
}

CCMythSession* CCMythSession::AquireSession(const CURL& url)
{
  CSingleLock lock(m_section_session);

  std::vector<CCMythSession*>::iterator it;
  for(it = m_sessions.begin(); it != m_sessions.end(); it++)
  {
    CCMythSession* session = *it;
    if(session->CanSupport(url))
    {
      m_sessions.erase(it);
      return session;
    }
  }
  return new CCMythSession(url);
}

void CCMythSession::ReleaseSession(CCMythSession* session)
{
    session->SetListener(NULL);
    session->m_timestamp = GetTickCount();
    CSingleLock lock(m_section_session);
    m_sessions.push_back(session);
}

CCMythSession::CCMythSession(const CURL& url)
{
  m_control   = NULL;
  m_event     = NULL;
  m_database  = NULL;
  m_hostname  = url.GetHostName();
  m_username  = url.GetUserName();
  m_password  = url.GetPassWord();
  m_port      = url.GetPort();
  m_timestamp = GetTickCount();
  if(m_port == 0)
    m_port = 6543;

  if(m_username == "")
    m_username = "mythtv";
  if(m_password == "")
    m_password = "mythtv";

  m_dll = new DllLibCMyth;
  m_dll->Load();
}

CCMythSession::~CCMythSession()
{
  Disconnect();
}

bool CCMythSession::CanSupport(const CURL& url)
{
  if(m_hostname != url.GetHostName())
    return false;

  int port;
  CStdString data;

  port = url.GetPort();
  if(port == 0) port = 6543;
  if(m_port != port)
    return false;

  data = url.GetUserName();
  if(data == "") data = "mythtv";
  if(m_username != data)
    return false;

  data = url.GetPassWord();
  if(data == "") data = "mythtv";
  if(m_password != data)
    return false;

  return true;
}

void CCMythSession::Process()
{
  char buf[128];
  int  next;

  struct timeval to;
  to.tv_sec = 1;
  to.tv_usec = 0;

  while(!m_bStop)
  {
    /* check if there are any new events */
    if(m_dll->event_select(m_event, &to) <= 0)
      continue;

    next = m_dll->event_get(m_event, buf, sizeof(buf));
    buf[sizeof(buf)-1] = 0;

    switch (next) {
    case CMYTH_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - MythTV unknown event (error?)", __FUNCTION__);
      break;
    case CMYTH_EVENT_CLOSE:
      CLog::Log(LOGDEBUG, "%s - MythTV event CMYTH_EVENT_CLOSE", __FUNCTION__);
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE:
      CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE", __FUNCTION__);
      break;
    case CMYTH_EVENT_SCHEDULE_CHANGE:
      CLog::Log(LOGDEBUG, "%s - MythTV event SCHEDULE_CHANGE", __FUNCTION__);
      break;
    case CMYTH_EVENT_DONE_RECORDING:
      CLog::Log(LOGDEBUG, "%s - MythTV event DONE_RECORDING", __FUNCTION__);
      break;
    case CMYTH_EVENT_QUIT_LIVETV:
      CLog::Log(LOGDEBUG, "%s - MythTV event QUIT_LIVETV", __FUNCTION__);
      break;
    case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
      CLog::Log(LOGDEBUG, "%s - MythTV event %s", __FUNCTION__, buf);
      break;
    case CMYTH_EVENT_SIGNAL:
      CLog::Log(LOGDEBUG, "%s - MythTV event SIGNAL", __FUNCTION__);
      break;
    case CMYTH_EVENT_ASK_RECORDING:
      CLog::Log(LOGDEBUG, "%s - MythTV event CMYTH_EVENT_ASK_RECORDING", __FUNCTION__);
      break;
    }

    { CSingleLock lock(m_section);
      if(m_listener)
        m_listener->OnEvent(next, buf);
    }
  }
}

void CCMythSession::Disconnect()
{
  if(!m_dll)
    return;

  StopThread();

  if(m_control)
    m_dll->ref_release(m_control);
  if(m_event)
    m_dll->ref_release(m_event);
  if(m_database)
    m_dll->ref_release(m_database);
}

cmyth_conn_t CCMythSession::GetControl()
{
  if(!m_control)
  {
    if(!m_dll->IsLoaded())
      return false;

    m_control = m_dll->conn_connect_ctrl((char*)m_hostname.c_str(), m_port, 16*1024, 4096);
    if(!m_control)
      CLog::Log(LOGERROR, "%s - unable to connect to server %s, port %d", __FUNCTION__, m_hostname.c_str(), m_port);
  }
  return m_control;
}

cmyth_database_t CCMythSession::GetDatabase()
{
  if(!m_database)
  {
    if(!m_dll->IsLoaded())
      return false;

    m_database = cmyth_database_init((char*)m_hostname.c_str(), (char*)"mythconverg", (char*)m_username.c_str(), (char*)m_password.c_str());
    if(!m_database)
      CLog::Log(LOGERROR, "%s - unable to connect to database %s, port %d", __FUNCTION__, m_hostname.c_str(), m_port);
  }
  return m_database;
}

bool CCMythSession::SetListener(IEventListener *listener)
{
  CSingleLock lock(m_section);
  if(!m_event && listener)
  {
    if(!m_dll->IsLoaded())
      return false;

    m_event = m_dll->conn_connect_event((char*)m_hostname.c_str(), m_port, 16*1024, 4096);
    if(!m_event)
    {
      CLog::Log(LOGERROR, "%s - unable to connect to server %s, port %d", __FUNCTION__, m_hostname.c_str(), m_port);
      return false;
    }
    /* start event handler thread */
    CThread::Create(false, THREAD_MINSTACKSIZE);
  }
  m_listener = listener;
  return true;
}
