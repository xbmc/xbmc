#include "stdafx.h"

#include "Idle.h"


CIdleThread::CIdleThread()
{}

CIdleThread::~CIdleThread()
{}

void CIdleThread::OnStartup()
{
  SetPriority(THREAD_PRIORITY_IDLE);
  SetName("IdleThread");
}

void CIdleThread::OnExit()
{}

void CIdleThread::Process()
{
  while (!m_bStop)
  {
    __asm
    {
      hlt
      hlt
      hlt
      hlt
      hlt
      hlt
    }
  }
}