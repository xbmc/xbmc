#pragma once

class CMediaSourcedSection
{

public:
  CMediaSourcedSection();
  CMediaSourcedSection(const CMediaSourcedSection& src);
  CMediaSourcedSection& operator=(const CMediaSourcedSection& src);
  virtual ~CMediaSourcedSection();

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

class CMediaSourcedLock
{
public:
  CMediaSourcedLock(CMediaSourcedSection& cs);
  CMediaSourcedLock(const CMediaSourcedSection& cs);
  virtual ~CMediaSourcedLock();

  bool IsOwner() const;
  inline bool Enter();
  inline void Leave();

protected:
  CMediaSourcedLock(const CMediaSourcedLock& src);
  CMediaSourcedLock& operator=(const CMediaSourcedLock& src);

  // Reference to critical section object
  CMediaSourcedSection& m_cs;
  // Ownership flag
  bool m_bIsOwner;
};

class CExclusiveLock
{
public:
  CExclusiveLock(CMediaSourcedSection& cs);
  CExclusiveLock(const CMediaSourcedSection& cs);
  virtual ~CExclusiveLock();

  bool IsOwner() const;
  inline bool Enter();
  inline void Leave();

protected:
  CExclusiveLock(const CExclusiveLock& src);
  CExclusiveLock& operator=(const CExclusiveLock& src);

  // Reference to critical section object
  CMediaSourcedSection& m_cs;
  // Ownership flag
  bool m_bIsOwner;
};

