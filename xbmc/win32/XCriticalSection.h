#ifndef _XCRITICAL_SECTION_H_
#define _XCRITICAL_SECTION_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

class XCriticalSection
{
 public:

  // Constructor/destructor.
  XCriticalSection();
  virtual ~XCriticalSection();
  
  // Initialize a critical section.
  void Initialize();
  
  // Destroy a critical section.
  void Destroy();
  
  // Enter a critical section.
  void Enter();
  
  // Leave a critical section.
  void Leave();
  
  // Checks if current thread owns the critical section.
  BOOL Owning();

  // Exits a critical section.
  DWORD Exit();
  
  // Restores critical section count.
  void Restore(DWORD count);

 private:

  CRITICAL_SECTION m_criticalSection;
  bool            m_isDestroyed;
  bool            m_isInitialized;
};

// Define the C API.
void  InitializeCriticalSection(XCriticalSection* section); 
void  DeleteCriticalSection(XCriticalSection* section);
BOOL  OwningCriticalSection(XCriticalSection* section);
DWORD ExitCriticalSection(XCriticalSection* section); 
void  RestoreCriticalSection(XCriticalSection* section, DWORD count);
void  EnterCriticalSection(XCriticalSection* section);
void  LeaveCriticalSection(XCriticalSection* section);

#endif