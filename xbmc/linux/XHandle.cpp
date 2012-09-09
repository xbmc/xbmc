/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XHandle.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

int CXHandle::m_objectTracker[10] = {0};

HANDLE WINAPI GetCurrentProcess(void) {
  return (HANDLE)-1; // -1 a special value - pseudo handle
}

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
}

CXHandle::CXHandle(const CXHandle &src)
{
  // we shouldnt get here EVER. however, if we do - try to make the best. copy what we can
  // and most importantly - not share any pointer.
  CLog::Log(LOGWARNING,"%s, copy handle.", __FUNCTION__);

  Init();

  if (src.m_hMutex)
    m_hMutex = new CCriticalSection();

  fd = src.fd;
  m_bManualEvent = src.m_bManualEvent;
  m_tmCreation = time(NULL);
  m_FindFileResults = src.m_FindFileResults;
  m_nFindFileIterator = src.m_nFindFileIterator;
  m_FindFileDir = src.m_FindFileDir;
  m_iOffset = src.m_iOffset;
  m_bCDROM = src.m_bCDROM;
  m_objectTracker[m_type]++;
}

CXHandle::~CXHandle()
{

  m_objectTracker[m_type]--;

  if (RecursionCount > 0) {
    CLog::Log(LOGERROR,"%s, destroying handle with recursion count %d", __FUNCTION__, RecursionCount);
    assert(false);
  }

  if (m_nRefCount > 1) {
    CLog::Log(LOGERROR,"%s, destroying handle with ref count %d", __FUNCTION__, m_nRefCount);
    assert(false);
  }

  if (m_hMutex) {
    delete m_hMutex;
  }

  if (m_internalLock) {
    delete m_internalLock;
  }

  if (m_hCond) {
    delete m_hCond;
  }

  if ( fd != 0 ) {
    close(fd);
  }

}

void CXHandle::Init()
{
  fd=0;
  m_hMutex=NULL;
  m_hCond=NULL;
  m_type = HND_NULL;
  RecursionCount=0;
  m_bManualEvent=FALSE;
  m_bEventSet=FALSE;
  m_nFindFileIterator=0 ;
  m_nRefCount=1;
  m_tmCreation = time(NULL);
  m_internalLock = new CCriticalSection();
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
  {
    CSingleLock lock((*hObject->m_internalLock));
    if (--hObject->m_nRefCount == 0)
      bDelete = true;
  }

  if (bDelete)
    delete hObject;

  return true;
}

BOOL WINAPI DuplicateHandle(
  HANDLE hSourceProcessHandle,
  HANDLE hSourceHandle,
  HANDLE hTargetProcessHandle,
  LPHANDLE lpTargetHandle,
  DWORD dwDesiredAccess,
  BOOL bInheritHandle,
  DWORD dwOptions
)
{
  /* only a simple version of this is supported */
  ASSERT(hSourceProcessHandle == GetCurrentProcess()
      && hTargetProcessHandle == GetCurrentProcess()
      && dwOptions            == DUPLICATE_SAME_ACCESS);

  if (hSourceHandle == INVALID_HANDLE_VALUE)
    return FALSE;

  {
    CSingleLock lock(*(hSourceHandle->m_internalLock));
    hSourceHandle->m_nRefCount++;
  }

  if(lpTargetHandle)
    *lpTargetHandle = hSourceHandle;

  return TRUE;
}

