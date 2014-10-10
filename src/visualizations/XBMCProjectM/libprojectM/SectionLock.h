
#ifndef SECTION_LOCK_H__
#define SECTION_LOCK_H__

#include <pthread.h>
#include <assert.h>

class CSectionLock
{
  pthread_mutex_t* m_lock;
  public:
    CSectionLock(pthread_mutex_t* lock) : m_lock(lock)
    {
#ifdef _USE_THREADS
      assert(lock);
      pthread_mutex_lock(&m_lock);
#endif
    }
    ~CSectionLock()
    {
#ifdef _USE_THREADS
      pthread_mutex_unlock(&m_lock);
#endif
    }
};

#endif // SECTION_LOCK_H__
