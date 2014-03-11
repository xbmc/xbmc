#include "PlexGlobalTimer.h"
#include "threads/SystemClock.h"
#include <boost/foreach.hpp>
#include "log.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexGlobalTimer::~CPlexGlobalTimer()
{
  if (m_running)
    StopAllTimers();
}

void CPlexGlobalTimer::StopAllTimers()
{
  CSingleLock lk(m_timerLock);

  m_running = false;
  m_timeouts.clear();
  CLog::Log(LOGDEBUG, "CPlexGlobalTimer::StopAllTimers signaling the timer thread to quit");
  m_timerEvent.Set();

  lk.unlock();

  StopThread(true);
  CLog::Log(LOGDEBUG, "CPlexGlobalTimer::StopAllTimers timer thread dead");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalTimer::SetTimeout(int64_t msec, IPlexGlobalTimeout *callback)
{
  int64_t absoluteTime = XbmcThreads::SystemClockMillis() + msec;
  timeoutPair newPair = std::make_pair(absoluteTime, callback);

  CSingleLock lk(m_timerLock);
  if (m_timeouts.size() == 0)
  {
    m_timeouts.push_back(newPair);
    m_timerEvent.Set();
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

  CLog::Log(LOGDEBUG, "CPlexGlobalTimer::SetTimeout adding timeout: %s at pos %d [%lld]", callback->TimerName().c_str(), idx, msec);
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

  CLog::Log(LOGDEBUG, "CPlexGlobalTimer::RemoveTimeout want to remove %s", callback->TimerName().c_str());

  int idx = -1, i = 0;
  BOOST_FOREACH(timeoutPair tmout, m_timeouts)
  {
    if (callback == tmout.second)
    {
      idx = i;
      break;
    }
    i ++;
  }

  CLog::Log(LOGDEBUG, "CPlexGlobaltimer::RemoveTimeout idx is %d for %s", idx, callback->TimerName().c_str());

  if (idx == -1)
    return;

  if (idx > m_timeouts.size())
    return;

  CLog::Log(LOGDEBUG, "CPlexGlobaltimer::RemoveTimeout removing %s %d", callback->TimerName().c_str(), idx);
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
void CPlexGlobalTimer::RemoveAllTimeoutsByName(const CStdString &name)
{
  CSingleLock lk(m_timerLock);
  std::vector<timeoutPair> toRemove;

  BOOST_FOREACH(timeoutPair p, m_timeouts)
  {
    if (name == p.second->TimerName())
      toRemove.push_back(p);
  }

  BOOST_FOREACH(timeoutPair p, toRemove)
    RemoveTimeout(p.second);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalTimer::DumpDebug()
{
#ifdef _DEBUG
  CLog::Log(LOGDEBUG, "CPlexGlobalTimer::DumpDebug ******");
  int i = 0;
  BOOST_FOREACH(timeoutPair p, m_timeouts)
  {
    CLog::Log(LOGDEBUG, "CPlexGlobalTimer::DumpDebug %d - %s (%lld)", i, p.second->TimerName().c_str(), p.first - XbmcThreads::SystemClockMillis());
    i++;
  }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalTimer::Process()
{
  m_running = true;
  while (m_running)
  {
    CSingleLock lk(m_timerLock);

    while (m_timeouts.size() == 0)
    {
      CLog::Log(LOGDEBUG, "CPlexGlobalTimer::Process no more timeouts, waiting for them.");
      lk.unlock();
      m_timerEvent.Wait();
      CLog::Log(LOGDEBUG, "CPlexGlobalTimer::StopAllTimers timer thread waking up");

      if (!m_running)
        break;

      lk.lock();
      if (!m_running)
        return;
    }

    timeoutPair p = m_timeouts.at(0);
    int64_t msecsToSleep = p.first - XbmcThreads::SystemClockMillis();

    m_timerEvent.Reset();
    lk.unlock();
    CLog::Log(LOGDEBUG, "CPlexGlobalTimer::Process waiting %lld milliseconds for %s...", msecsToSleep, p.second->TimerName().c_str());
    if (msecsToSleep <= 0 || !m_timerEvent.WaitMSec(msecsToSleep))
    {
      lk.lock();
      if (!m_running)
        return;

      CLog::Log(LOGDEBUG, "CPlexGlobalTimer::Process firing callback %s", p.second->TimerName().c_str());
      m_timeouts.erase(m_timeouts.begin());
      DumpDebug();
      CJobManager::GetInstance().AddJob(new CPlexGlobalTimerJob(p.second), NULL, CJob::PRIORITY_HIGH);
    }
  }
  CLog::Log(LOGDEBUG, "CPlexGlobalTimer::StopAllTimers timer thread quitting");
}
