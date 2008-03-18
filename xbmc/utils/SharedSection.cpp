#include "stdafx.h"
#include "SharedSection.h"

#define MAXSHAREDACCESSES 5000

CSharedSection::CSharedSection()
{
  m_sharedLock = 0;
  m_exclusive = false;
  InitializeCriticalSection(&m_critSection);
  InitializeCriticalSection(&m_helperLock);
  m_eventFree = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CSharedSection::~CSharedSection()
{
  DeleteCriticalSection(&m_critSection);
  DeleteCriticalSection(&m_helperLock);
  CloseHandle(m_eventFree);
}

void CSharedSection::EnterShared()
{
  EnterCriticalSection(&m_critSection);
  if( !m_exclusive )
  { //exclusve will be set if this thread already owns this object
    ResetEvent(m_eventFree);
    EnterCriticalSection(&m_helperLock);
    m_sharedLock++;
    LeaveCriticalSection(&m_helperLock);
  }
  LeaveCriticalSection(&m_critSection);
}

void CSharedSection::LeaveShared()
{
  // NO! We always need to do this to keep the count right and set the event.
  //if( !m_exclusive )
  {
    EnterCriticalSection(&m_helperLock);
    m_sharedLock--;
    if (m_sharedLock == 0)
      SetEvent(m_eventFree);
    
    LeaveCriticalSection(&m_helperLock);
  }
}

void CSharedSection::EnterExclusive()
{
  EnterCriticalSection(&m_critSection);
  if( m_sharedLock != 0 )
    WaitForSingleObject(m_eventFree, INFINITE);

  m_exclusive = true;
}

void CSharedSection::LeaveExclusive()
{
  m_exclusive = false;
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

