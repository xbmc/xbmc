#ifndef __IDLE_H__
#define __IDLE_H__

#include "Thread.h" 
///////////////////////////////// Classes //////////////////////////////

class CIdleThread : public CThread
{
public:
  //Constructors / Destructors
  CIdleThread();
  virtual ~CIdleThread();
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
};


#endif //__IDLE_H__
