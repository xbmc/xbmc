
#include "PlatformDefs.h"
#include "XHandle.h"
#include "XThreadUtils.h"
#include "XTimeUtils.h"

#ifdef _LINUX

// a variable which is defined __thread will be defined in thread local storage
// which means it will be different for each thread that accesses it.
LPVOID __thread tls = NULL;

HANDLE CreateThread(
		  LPSECURITY_ATTRIBUTES lpThreadAttributes,
		    SIZE_T dwStackSize,
		      LPTHREAD_START_ROUTINE lpStartAddress,
		        LPVOID lpParameter,
			  DWORD dwCreationFlags,
			    LPDWORD lpThreadId
		) {
			
	HANDLE h = new CXHandle(CXHandle::HND_THREAD);
	h->m_hThread = SDL_CreateThread(lpStartAddress, (void*)lpParameter);
	return h;
	
}


DWORD GetCurrentThreadId(void) {
	return SDL_ThreadID();
}

HANDLE GetCurrentThread(void) {
	return (HANDLE)-1; // -1 a special value - pseudo handle
}

HANDLE _beginthreadex( 
   void *security,
   unsigned stack_size,
   int ( *start_address )( void * ),
   void *arglist,
   unsigned initflag,
   unsigned *thrdaddr 
) {
	
  HANDLE h = new CXHandle(CXHandle::HND_THREAD);
  h->m_hThread = SDL_CreateThread(start_address, (void*)arglist);
  return h;
	
}

BOOL GetThreadTimes (
  HANDLE hThread,
  LPFILETIME lpCreationTime,
  LPFILETIME lpExitTime,
  LPFILETIME lpKernelTime,
  LPFILETIME lpUserTime
) {
	if (hThread == NULL)
		return false;
		
	if (hThread == (HANDLE)-1) {
		if (lpCreationTime)
			TimeTToFileTime(0,lpCreationTime);
		if (lpExitTime)
			TimeTToFileTime(time(NULL),lpExitTime);
		if (lpKernelTime)
			TimeTToFileTime(0,lpKernelTime);
		if (lpUserTime)
			TimeTToFileTime(0,lpUserTime);
			
		return true;
	}
	
	if (lpCreationTime)
		TimeTToFileTime(hThread->m_tmCreation,lpCreationTime);
	if (lpExitTime)
		TimeTToFileTime(time(NULL),lpExitTime);
	if (lpKernelTime)
		TimeTToFileTime(0,lpKernelTime);
	if (lpUserTime)
		TimeTToFileTime(0,lpUserTime);
		
	return true;
}

// thread local storage -
// we use different method than in windows. TlsAlloc has no meaning since
// we always take the __thread variable "tls".
// so we return static answer in TlsAlloc and do nothing in TlsFree.
LPVOID TlsGetValue(DWORD dwTlsIndex) {
   return tls;
}

BOOL TlsSetValue(int dwTlsIndex, LPVOID lpTlsValue) {
   tls=lpTlsValue;
   return true;
}

BOOL TlsFree(DWORD dwTlsIndex) {
   return true;
}

DWORD TlsAlloc() {
   return 1;
}


#endif

