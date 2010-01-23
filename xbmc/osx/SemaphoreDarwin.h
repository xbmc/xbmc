
#ifndef SEMAPHORE_DARWIN_H__
#define SEMAPHORE_DARWIN_H__

#include "ISemaphore.h"
#include <Semaphore.h>

class CSemaphoreDarwin : public ISemaphore
{
    char*   m_szName;
    sem_t*  m_pSem;
  public:
                      CSemaphoreDarwin(uint32_t initialCount);
    virtual           ~CSemaphoreDarwin();
    virtual bool      Wait();
    virtual SEM_GRAB  TimedWait(uint32_t millis);
    virtual bool      TryWait();
    virtual bool      Post();
    virtual int       GetCount() const;
};

#endif // SEMAPHORE_DARWIN_H__

