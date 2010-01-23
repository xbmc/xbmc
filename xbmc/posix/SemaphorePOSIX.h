
#ifndef SEMAPHORE_POSIX_H__
#define SEMAPHORE_POSIX_H__

#include "ISemaphore.h"
#include <semaphore.h>

class CSemaphorePOSIX : public ISemaphore
{
    char*   m_szName;
    sem_t*  m_pSem;
  public:
                      CSemaphorePOSIX(uint32_t initialCount);
    virtual           ~CSemaphorePOSIX();
    virtual bool      Wait();
    virtual SEM_GRAB  TimedWait(uint32_t millis);
    virtual bool      TryWait();
    virtual bool      Post();
    virtual int       GetCount() const;
};

#endif // SEMAPHORE_POSIX_H__

