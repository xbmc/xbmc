#ifndef __XTHREAD_UTILS__H__
#define __XTHREAD_UTILS__H__

HANDLE CreateThread(
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

DWORD GetCurrentThreadId(void);

HANDLE GetCurrentThread(void);

BOOL GetThreadTimes (
  HANDLE hThread,
  LPFILETIME lpCreationTime,
  LPFILETIME lpExitTime,
  LPFILETIME lpKernelTime,
  LPFILETIME lpUserTime
);

// thread local storage functions
LPVOID TlsGetValue(DWORD dwTlsIndex);
BOOL TlsSetValue(int dwTlsIndex, LPVOID lpTlsValue);
BOOL TlsFree(DWORD dwTlsIndex);
DWORD TlsAlloc();


#endif
