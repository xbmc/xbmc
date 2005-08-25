#include "../stdafx.h"
#include "SharedSection.h"

#define MAXSHAREDACCESSES 5000

CSharedSection::CSharedSection()
{
  m_sharedLock = 0;
  InitializeCriticalSection(&m_critSection);
  m_eventFree = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CSharedSection::~CSharedSection()
{
  DeleteCriticalSection(&m_critSection);
  CloseHandle(m_eventFree);
}

void CSharedSection::EnterShared()
{
  EnterCriticalSection(&m_critSection);
  ResetEvent(m_eventFree);
  InterlockedIncrement(&m_sharedLock);
  LeaveCriticalSection(&m_critSection);
}

void CSharedSection::LeaveShared()
{
  if( InterlockedDecrement(&m_sharedLock) == 0 ) 
    SetEvent(m_eventFree);
}

void CSharedSection::EnterExclusive()
{
  EnterCriticalSection(&m_critSection);
  if( InterlockedCompareExchange(&m_sharedLock, 0, 0) != 0 )
    WaitForSingleObject(m_eventFree, INFINITE);

}

void CSharedSection::LeaveExclusive()
{
  LeaveCriticalSection(&m_critSection);
}

//////////////////////////////////////////////////////////////////////
// SharedLock
//////////////////////////////////////////////////////////////////////

CSharedLock::CSharedLock(CSharedSection& cs)
    : m_cs( cs )
    , m_bIsOwner( false )
{
  Enter();
}

CSharedLock::CSharedLock(const CSharedSection& cs)
    : m_cs( const_cast<CSharedSection&>(cs) )
    , m_bIsOwner( false )
{
  Enter();
}

CSharedLock::~CSharedLock()
{
  Leave();
}

bool CSharedLock::IsOwner() const
{
  return m_bIsOwner;
}

bool CSharedLock::Enter()
{
  // Test if we already own the critical section
  if ( true == m_bIsOwner )
  {
    return true;
  }

  // Blocking call
  m_cs.EnterShared();
  m_bIsOwner = true;

  return m_bIsOwner;
}

void CSharedLock::Leave()
{
  if ( false == m_bIsOwner )
  {
    return ;
  }

  m_cs.LeaveShared();
  m_bIsOwner = false;
}

//////////////////////////////////////////////////////////////////////
// ExclusiveLock
//////////////////////////////////////////////////////////////////////

CExclusiveLock::CExclusiveLock(CSharedSection& cs)
    : m_cs( cs )
    , m_bIsOwner( false )
{
  Enter();
}

CExclusiveLock::CExclusiveLock(const CSharedSection& cs)
    : m_cs( const_cast<CSharedSection&>(cs) )
    , m_bIsOwner( false )
{
  Enter();
}

CExclusiveLock::~CExclusiveLock()
{
  Leave();
}

bool CExclusiveLock::IsOwner() const
{
  return m_bIsOwner;
}

bool CExclusiveLock::Enter()
{
  // Test if we already own the critical section
  if ( true == m_bIsOwner )
  {
    return true;
  }

  // Blocking call
  m_cs.EnterExclusive();
  m_bIsOwner = true;

  return m_bIsOwner;
}

void CExclusiveLock::Leave()
{
  if ( false == m_bIsOwner )
  {
    return ;
  }

  m_cs.LeaveExclusive();
  m_bIsOwner = false;
}