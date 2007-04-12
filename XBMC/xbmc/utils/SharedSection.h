#pragma once

class CSharedSection
{

public:
  CSharedSection();
  CSharedSection(const CSharedSection& src);
  CSharedSection& operator=(const CSharedSection& src);
  virtual ~CSharedSection();

  inline void EnterExclusive();
  inline void LeaveExclusive();

  inline void EnterShared();
  inline void LeaveShared();
  
private:

  CRITICAL_SECTION m_critSection;
  HANDLE m_eventFree;
  bool m_exclusive;
  long m_sharedLock;
};

class CSharedLock
{
public:
  CSharedLock(CSharedSection& cs);
  CSharedLock(const CSharedSection& cs);
  virtual ~CSharedLock();

  bool IsOwner() const;
  inline bool Enter();
  inline void Leave();

protected:
  CSharedLock(const CSharedLock& src);
  CSharedLock& operator=(const CSharedLock& src);

  // Reference to critical section object
  CSharedSection& m_cs;
  // Ownership flag
  bool m_bIsOwner;
};

class CExclusiveLock
{
public:
  CExclusiveLock(CSharedSection& cs);
  CExclusiveLock(const CSharedSection& cs);
  virtual ~CExclusiveLock();

  bool IsOwner() const;
  inline bool Enter();
  inline void Leave();

protected:
  CExclusiveLock(const CExclusiveLock& src);
  CExclusiveLock& operator=(const CExclusiveLock& src);

  // Reference to critical section object
  CSharedSection& m_cs;
  // Ownership flag
  bool m_bIsOwner;
};