#ifndef __XTHREAD_UTILS__H__
#define __XTHREAD_UTILS__H__

HANDLE WINAPI CreateThread(
		  LPSECURITY_ATTRIBUTES lpThreadAttributes,
		    SIZE_T dwStackSize,
		      LPTHREAD_START_ROUTINE lpStartAddress,
		        LPVOID lpParameter,
			  DWORD dwCreationFlags,
			    LPDWORD lpThreadId
		);

HANDLE _beginthreadex( 
   void *security,
   unsigned stack_size,
   int ( *start_address )( void * ),
   void *arglist,
   unsigned initflag,
   unsigned *thrdaddr 
);

uintptr_t _beginthread(
    void( *start_address )( void * ),
    unsigned stack_size,
    void *arglist
);


DWORD WINAPI GetCurrentThreadId(void);

HANDLE WINAPI GetCurrentThread(void);

BOOL WINAPI GetThreadTimes (
  HANDLE hThread,
  LPFILETIME lpCreationTime,
  LPFILETIME lpExitTime,
  LPFILETIME lpKernelTime,
  LPFILETIME lpUserTime
);

int GetThreadPriority(
  HANDLE hThread
);

BOOL WINAPI SetThreadPriority(
  HANDLE hThread,
  int nPriority
);

// thread local storage functions
LPVOID WINAPI TlsGetValue(DWORD dwTlsIndex);
BOOL WINAPI TlsSetValue(int dwTlsIndex, LPVOID lpTlsValue);
BOOL WINAPI TlsFree(DWORD dwTlsIndex);
DWORD WINAPI TlsAlloc();


#endif
