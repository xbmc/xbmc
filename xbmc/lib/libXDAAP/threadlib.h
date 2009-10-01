#ifndef __THREADLIB_H__
#define __THREADLIB_H__

#include <process.h>
#include "types.h"
#include "compat.h"

#if _XBOX
	#define beginthread(thread, callback) \
		_beginthread(callback, 0, (void *)NULL)
#elif __UNIX__
    #include <unistd.h>
	#define Sleep(x) usleep(x)
	#define beginthread(thread, callback) \
		pthread_create(thread, NULL, \
                          (void *)callback, (void *)NULL)
#endif

typedef struct THREAD_HANDLEst
{
	THANDLE thread_handle;
} THREAD_HANDLE;

typedef void (__cdecl *CALLBACKPROC)(void *);

/*********************************************************************************
 * Public functions
 *********************************************************************************/
/*
extern error_code	threadlib_beginthread(THREAD_HANDLE *thread, void (*callback)(void *));
extern BOOL		threadlib_isrunning(THREAD_HANDLE *thread);
extern void		threadlib_waitforclose(THREAD_HANDLE *thread);
extern void		threadlib_endthread(THREAD_HANDLE *thread);
extern BOOL		threadlib_sem_signaled(HSEM *e);

extern HSEM		threadlib_create_sem();
extern error_code	threadlib_waitfor_sem(HSEM *e);
extern error_code	threadlib_signel_sem(HSEM *e);
extern void		threadlib_destroy_sem(HSEM *e);
*/

error_code	threadlib_beginthread(THREAD_HANDLE *thread, void (*callback)(void *));
BOOL		threadlib_isrunning(THREAD_HANDLE *thread);
void		threadlib_waitforclose(THREAD_HANDLE *thread);
void		threadlib_endthread(THREAD_HANDLE *thread);

HSEM		threadlib_create_sem();
error_code	threadlib_waitfor_sem(HSEM *e);
error_code	threadlib_signel_sem(HSEM *e);
void		threadlib_destroy_sem(HSEM *e);

#endif //__THREADLIB__
