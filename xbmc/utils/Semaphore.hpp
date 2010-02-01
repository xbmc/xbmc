
#ifndef SEMAPHORE_H__
#define SEMAPHORE_H__

#include "ISemaphore.h"

class CSemaphore : public ISemaphore
{
    ISemaphore* m_pSemaphore;
  public:
                      CSemaphore(uint32_t initialCount=1);
                      CSemaphore(const CSemaphore& sem);
    virtual           ~CSemaphore();
    virtual bool      Wait();
    virtual SEM_GRAB  TimedWait(uint32_t millis);
    virtual bool      TryWait();
    virtual bool      Post();
    virtual int       GetCount() const;
};

#endif // SEMAPHORE_H__

