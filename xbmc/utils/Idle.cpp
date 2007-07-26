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
   Sleep(0);
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
