#include "PlexGlobalTimer.h"
#include "threads/SystemClock.h"
#include <boost/foreach.hpp>
#include "log.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexGlobalTimer::~CPlexGlobalTimer()
{
  StopAllTimers();
}

void CPlexGlobalTimer::StopAllTimers()
{
  m_running = false;
  m_timeouts.clear();
  m_timerEvent.Set();
  StopThread(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalTimer::SetTimeout(int64_t msec, IPlexGlobalTimeout *callback)
{
  int64_t absoluteTime = XbmcThreads::SystemClockMillis() + msec;
  timeoutPair newPair = std::make_pair<int64_t, IPlexGlobalTimeout*>(absoluteTime, callback);

  CSingleLock lk(m_timerLock);
  if (m_timeouts.size() == 0)
  {
    m_timeouts.push_back(newPair);
    Create(false);
    return;
  }

  int idx = 0;
  BOOST_FOREACH(timeoutPair tmout, m_timeouts)
  {
    if (tmout.second == callback)
    {
      m_timeouts.erase(m_timeouts.begin() + idx);
      break;
    }
    idx++;
  }

  idx = 0;
  BOOST_FOREACH(timeoutPair tmout, m_timeouts)
  {
    if (absoluteTime < tmout.first)
      break;

    idx ++;
  }

  CLog::Log(LOGDEBUG, "CPlexGlobalTimer::SetTimeout adding timeout at pos %d", idx);
  if (idx > m_timeouts.size())
    m_timeouts.push_back(newPair);
  else
    m_timeouts.insert(m_timeouts.begin() + idx, newPair);

  if (idx == 0)
    m_timerEvent.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalTimer::RemoveTimeout(IPlexGlobalTimeout *callback)
{
  CSingleLock lk(m_timerLock);

  if (m_timeouts.size() == 0)
    return;

  int idx = 0;
  BOOST_FOREACH(timeoutPair tmout, m_timeouts)
  {
    if (callback == tmout.second)
      break;
    idx ++;
  }

  if (idx > m_timeouts.size())
    return;

  m_timeouts.erase(m_timeouts.begin() + idx);

  if (idx == 0)
    m_timerEvent.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalTimer::RestartTimeout(int64_t msec, IPlexGlobalTimeout *callback)
{
  RemoveTimeout(callback);
  SetTimeout(msec, callback);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalTimer::Process()
{
  m_running = true;
  while (m_running)
  {
    CSingleLock lk(m_timerLock);
    if (m_timeouts.size() == 0)
    {
      CLog::Log(LOGDEBUG, "CPlexGlobalTimer::Process no more timeouts.");
      return;
    }

    timeoutPair p = m_timeouts.at(0);
    int64_t msecsToSleep = p.first - XbmcThreads::SystemClockMillis();

    m_timerEvent.Reset();
    lk.unlock();
    CLog::Log(LOGDEBUG, "CPlexGlobalTimer::Process waiting %lld milliseconds...", msecsToSleep);
    if (msecsToSleep <= 0 || !m_timerEvent.WaitMSec(msecsToSleep))
    {
      lk.lock();
      CLog::Log(LOGDEBUG, "CPlexGlobalTimer::Process firing callback");
      m_timeouts.erase(m_timeouts.begin());
      CJobManager::GetInstance().AddJob(new CPlexGlobalTimerJob(p.second), NULL, CJob::PRIORITY_HIGH);
    }
    else
    {
      lk.lock();
      if (!m_running)
        return;
    }
  }
}
