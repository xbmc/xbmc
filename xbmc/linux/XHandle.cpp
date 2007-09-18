
#include <SDL/SDL.h>

#include "XHandle.h"
#include "../utils/log.h"

int CXHandle::m_objectTracker[10] = {0};

CXHandle::CXHandle()
{ 
  Init();
  m_objectTracker[m_type]++;
}

CXHandle::CXHandle(HandleType nType)
{ 
  Init();
  m_type=nType; 
  m_objectTracker[m_type]++;
};

CXHandle::~CXHandle() {

  m_objectTracker[m_type]--;

  if (RecursionCount > 0) {
    CLog::Log(LOGERROR,"%s, destroying handle with recursion count %d", __FUNCTION__, RecursionCount);
    assert(false);
  }

  if (m_nRefCount > 1) {
    CLog::Log(LOGERROR,"%s, destroying handle with ref count %d", __FUNCTION__, m_nRefCount);
    assert(false);
  }

  if (m_hSem) {
    SDL_DestroySemaphore(m_hSem);
  }

  if (m_hMutex) {
    SDL_DestroyMutex(m_hMutex);
  }

  if (m_internalLock) {
    SDL_DestroyMutex(m_internalLock);
  }

  if (m_hThread) {
    //SDL_WaitThread should only be called once (here, when destructing) since it will destroy the thread object.
    SDL_WaitThread(m_hThread,NULL);
  }

  if ( fd != 0 ) {
    close(fd);
  }

}

void CXHandle::Init() 
{
  fd=0; 
  m_hSem=NULL; 
  m_hMutex=NULL; 
  m_hThread=NULL; 
  m_type = HND_NULL;
  RecursionCount=0;
  OwningThread=0;
  m_bManualEvent=FALSE;
  m_nFindFileIterator=0 ;
  m_nRefCount=1;
  m_tmCreation = time(NULL);
  m_internalLock = SDL_CreateMutex();
}

void CXHandle::ChangeType(HandleType newType) {
  m_objectTracker[m_type]--;
  m_type = newType;
  m_objectTracker[m_type]++;
}

void CXHandle::DumpObjectTracker() {
  for (int i=0; i< 10; i++) {
    CLog::Log(LOGDEBUG,"object %d --> %d instances\n", i, m_objectTracker[i]);
  }
}

bool CloseHandle(HANDLE hObject) {
  if (!hObject)
    return false;

  if (hObject == INVALID_HANDLE_VALUE || hObject == (HANDLE)-1)
    return true;

  bool bDelete = false;
  SDL_LockMutex(hObject->m_internalLock);
  if (--hObject->m_nRefCount == 0)
    bDelete = true;
  SDL_UnlockMutex(hObject->m_internalLock);

  if (bDelete) 
    delete hObject;

  return true;
}

