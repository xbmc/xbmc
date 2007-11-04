#ifndef __X_SYNC_UTILS_
#define __X_SYNC_UTILS_

#include "XHandle.h"

#ifdef _LINUX

#define STATUS_WAIT_0	((DWORD   )0x00000000L)    
#define WAIT_FAILED		((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0	((STATUS_WAIT_0 ) + 0 )
#define WAIT_TIMEOUT	258L
#define INFINITE		0xFFFFFFFF
#define STATUS_ABANDONED_WAIT_0 0x00000080
#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 )

HANDLE	WINAPI CreateMutex( LPSECURITY_ATTRIBUTES lpMutexAttributes,  BOOL bInitialOwner,  LPCTSTR lpName );
bool	InitializeRecursiveMutex(HANDLE hMutex, BOOL bInitialOwner);
bool	DestroyRecursiveMutex(HANDLE hMutex);
bool	WINAPI ReleaseMutex( HANDLE hMutex );

void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);

void GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer);

DWORD WINAPI WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds );
DWORD WINAPI WaitForMultipleObjects( DWORD nCount, HANDLE* lpHandles, BOOL bWaitAll,  DWORD dwMilliseconds);

LONG InterlockedIncrement(  LONG * Addend );
LONG InterlockedDecrement(  LONG * Addend );
LONG InterlockedCompareExchange(
  LONG * Destination,
  LONG Exchange,
  LONG Comparand
);

LONG InterlockedExchange(
  LONG volatile* Target,
  LONG Value
);

int SDL_SemWaitTimeout2(SDL_sem *sem, Uint32 ms);

#endif 

#endif

