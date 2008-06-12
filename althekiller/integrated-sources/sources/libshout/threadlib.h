#ifndef __THREADLIB_H__
#define __THREADLIB_H__

#include "srtypes.h"
#include "compat.h"

typedef struct THREAD_HANDLEst
{
	THANDLE thread_handle;
} THREAD_HANDLE;


/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code threadlib_beginthread (THREAD_HANDLE *thread, void (*callback)(void *), void* arg);
extern BOOL		threadlib_isrunning(THREAD_HANDLE *thread);
extern void		threadlib_waitforclose(THREAD_HANDLE *thread);
extern void		threadlib_endthread(THREAD_HANDLE *thread);
extern BOOL		threadlib_sem_signaled(HSEM *e);

extern HSEM		threadlib_create_sem();
extern error_code	threadlib_waitfor_sem(HSEM *e);
extern error_code	threadlib_signal_sem(HSEM *e);
extern void		threadlib_destroy_sem(HSEM *e);


#endif //__THREADLIB__
