#include "stdafx.h"

#include "Idle.h"


CIdleThread::CIdleThread()
{}

CIdleThread::~CIdleThread()
{}

void CIdleThread::OnStartup()
{
  SetPriority(THREAD_PRIORITY_IDLE);
}

void CIdleThread::OnExit()
{}

void CIdleThread::Process()
{
  while (!m_bStop)
  {
#ifndef _LINUX	  
    __asm
    {
      hlt
      hlt
      hlt
      hlt
      hlt
      hlt
    }
#else
    __asm__("hlt\n\t"
            "hlt\n\t"
            "hlt\n\t"
            "hlt\n\t"
            "hlt\n\t"
            "hlt\n\t");
#endif
  }
}
