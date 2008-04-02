#include "stdafx.h"
#include "SharedSection.h"

#define MAXSHAREDACCESSES 5000

CMediaSourcedSection::CMediaSourcedSection()
{
  m_sharedLock = 0;
  m_exclusive = false;
  InitializeCriticalSection(&m_critSection);
  m_eventFree = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CMediaSourcedSection::~CMediaSourcedSection()
{
  DeleteCriticalSection(&m_critSection);
  CloseHandle(m_eventFree);
}

void CMediaSourcedSection::EnterShared()
{
  EnterCriticalSection(&m_critSection);
  if( !m_exclusive )
  { //exclusve will be set if this thread already owns this object
    ResetEvent(m_eventFree);
    InterlockedIncrement(&m_sharedLock);
  }
  LeaveCriticalSection(&m_critSection);
}

void CMediaSourcedSection::LeaveShared()
{
  if( !m_exclusive )
  {
    if( InterlockedDecrement(&m_sharedLock) == 0 ) 
      SetEvent(m_eventFree);
  }
}

void CMediaSourcedSection::EnterExclusive()
{
  EnterCriticalSection(&m_critSection);
  if( InterlockedCompareExchange(&m_sharedLock, 0, 0) != 0 )
    WaitForSingleObject(m_eventFree, INFINITE);
  m_exclusive = true;
}

void CMediaSourcedSection::LeaveExclusive()
{
  m_exclusive = false;
  LeaveCriticalSection(&m_critSection);
}

//////////////////////////////////////////////////////////////////////
// SharedLock
//////////////////////////////////////////////////////////////////////

CMediaSourcedLock::CMediaSourcedLock(CMediaSourcedSection& cs)
    : m_cs( cs )
    , m_bIsOwner( false )
{
  Enter();
}

CMediaSourcedLock::CMediaSourcedLock(const CMediaSourcedSection& cs)
    : m_cs( const_cast<CMediaSourcedSection&>(cs) )
    , m_bIsOwner( false )
{
  Enter();
}

CMediaSourcedLock::~CMediaSourcedLock()
{
  Leave();
}

bool CMediaSourcedLock::IsOwner() const
{
  return m_bIsOwner;
}

bool CMediaSourcedLock::Enter()
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

void CMediaSourcedLock::Leave()
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

CExclusiveLock::CExclusiveLock(CMediaSourcedSection& cs)
    : m_cs( cs )
    , m_bIsOwner( false )
{
  Enter();
}

CExclusiveLock::CExclusiveLock(const CMediaSourcedSection& cs)
    : m_cs( const_cast<CMediaSourcedSection&>(cs) )
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

