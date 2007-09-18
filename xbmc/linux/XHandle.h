#ifndef __X_HANDLE__
#define __X_HANDLE__

#ifndef _WIN32

#include "../../guilib/StdString.h"
#include <SDL/SDL_mutex.h>
#include <SDL/SDL_thread.h>

#include "PlatformDefs.h"
#include "StringUtils.h"

struct CXHandle {

public:
  typedef enum { HND_NULL = 0, HND_FILE, HND_EVENT, HND_MUTEX, HND_THREAD, HND_FIND_FILE } HandleType;

  CXHandle();
  CXHandle(HandleType nType);
  virtual ~CXHandle();
  void Init();
  inline HandleType GetType() { return m_type; }
  void ChangeType(HandleType newType);
  
  SDL_sem    *m_hSem;
  SDL_Thread  *m_hThread;

  // simulate mutex and critical section
  SDL_mutex  *m_hMutex;
  int      RecursionCount;  // for mutex - for compatibility with WIN32 critical section
  DWORD    OwningThread;
  int      fd;
  bool     m_bManualEvent;
  time_t    m_tmCreation;
  CStdStringArray  m_FindFileResults;
  int              m_nFindFileIterator;  
  CStdString       m_FindFileDir;
  off64_t          m_iOffset;
  bool             m_bCDROM;

  int              m_nRefCount;
  SDL_mutex *m_internalLock; 

  static void DumpObjectTracker();

protected:
  HandleType  m_type;
  static int m_objectTracker[10];

};

#define HANDLE CXHandle*

bool CloseHandle(HANDLE hObject);

#endif

#endif

