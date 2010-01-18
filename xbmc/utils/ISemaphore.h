
#ifndef ISEMAPHORE_H__
#define ISEMAPHORE_H__

#include <stdint.h>

enum SEM_GRAB
{
  SEM_GRAB_SUCCESS,
  SEM_GRAB_FAILED,
  SEM_GRAB_TIMEOUT
};

class ISemaphore
{
  public:
                      ISemaphore()  {}
    virtual           ~ISemaphore() {}
    virtual bool      Wait()                      = 0;
    virtual SEM_GRAB  TimedWait(uint32_t millis)  = 0;
    virtual bool      TryWait()                   = 0;
    virtual bool      Post()                      = 0;
    virtual int       GetCount() const            = 0;
};

#endif // ISEMAPHORE_H__

