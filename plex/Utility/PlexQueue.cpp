#include "PlexQueue.h"
#include "Client/PlexTimeline.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
bool CPlexQueue<T>::empty() const
{
  CSingleLock lock(m_critical);
  return m_queue.empty();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
bool CPlexQueue<T>::tryEnqueue(const T &item)
{
  CSingleLock lock(m_critical);

  if (m_queue.size() >= m_maxSize)
    return false;

  m_queue.push(item);

  if (m_fillEvent.getNumWaits() > 0)
    m_fillEvent.Set();

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
bool CPlexQueue<T>::tryPop(T &item)
{
  CSingleLock lock(m_critical);

  if (m_queue.empty())
    return false;

  item = m_queue.front();
  m_queue.pop();

  return item;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
bool CPlexQueue<T>::waitPop(T &item, int msec)
{
  CSingleLock lock(m_critical);
  if (m_abort)
    return false;

  if (m_queue.empty())
  {
    if (m_abort)
      return false;

    m_fillEvent.Reset();

    /* don't hold this lock while waiting, it's rude! */
    lock.Leave();

    if (msec == -1)
    {
      if (!m_fillEvent.Wait())
        return false;
    }
    else
    {
      if (!m_fillEvent.WaitMSec(msec))
        return false;
    }

    lock.Enter();
    if (m_abort)
      return false;

    if (m_queue.empty())
      return false;
  }

  return tryPop(item);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
void CPlexQueue<T>::cancel()
{
  CSingleLock lock(m_critical);
  m_abort = true;
  m_fillEvent.Set();
}

template class CPlexQueue<CPlexTimelineCollectionPtr>;
