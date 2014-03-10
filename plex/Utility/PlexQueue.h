#ifndef PLEXQUEUE_H
#define PLEXQUEUE_H

#include "log.h"
#include "threads/Event.h"
#include "threads/CriticalSection.h"

#include <queue>

template <class T>
class CPlexQueue
{
  public:
    CPlexQueue(int maxsize = 20) : m_maxSize(maxsize), m_abort(false) {}

    bool empty() const;
    bool tryEnqueue(const T& item);
    bool tryPop(T& item);
    bool waitPop(T& item, int msec = -1);
    void cancel();

  private:
    CEvent m_fillEvent;
    CCriticalSection m_critical;
    std::queue<T> m_queue;

    int m_maxSize;
    bool m_abort;
};

#endif // PLEXQUEUE_H
