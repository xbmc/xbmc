///
///	@file 	WIN/thread.cpp
/// @brief 	Primitive multi-threading support for Linux
/// @overview This module provides threading, mutex and condition 
///		variable APIs for Windows.
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ///////////////////////////////////

#include	"mpr/mpr.h"

//////////////////////////////////// Locals ///////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

#if BLD_DEBUG
CRITICAL_SECTION	debugCs;
static MprList		mutexList;
static int			numMutex = -1;		// Must initialize to -1 (see addMutex)
MprList				condList;

#define DEBUG_TIME	0					// Spare[0] == time
#define DEBUG_ID	1					// Spare[1] == thread id
#endif

//////////////////////////// Forward Declarations /////////////////////////////

static uint			__stdcall threadProcWrapper(void *arg);

#if BLD_DEBUG
#if UNUSED
static void			lockBuster(void *data, MprThread *tp);
#endif
static void			removeMutex(MprMutex *mp);
static void			addMutex(MprMutex *mp);
#endif

///////////////////////////////////// Code ////////////////////////////////////
//
//	Initialize thread service
//
//

MprThreadService::MprThreadService()
{
	mutex = new MprMutex();

	//
	//	Don't actually create the thread. Just create a thread object for this 
	//	main thread.
	//
	mainThread = new MprThread(MPR_NORMAL_PRIORITY, "main");
	mainThread->setOsThread(GetCurrentThreadId());

	insertThread(mainThread);
}

///////////////////////////////////////////////////////////////////////////////
//
//	Terminate the thread service
//

MprThreadService::~MprThreadService()
{
	delete mainThread;

#if BLD_DEBUG
	if (threads.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d thread(s) unfreed", 
			threads.getNumItems());
	}
	if (condList.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d cond var(s) unfreed", 
			condList.getNumItems());
	}
	//
	//	We allow one open mutex for the log service which has not yet shutdown, 
	//	and one for the libmpr DLL (malloc), we also need one for our mutex
	//	is freed below.
	//
	if (mutexList.getNumItems() > 3) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d mutex(es) unfreed", 
			mutexList.getNumItems() - 3);
	}
#endif
	delete mutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a thread to the list
//

void MprThreadService::insertThread(MprThread *tp)
{
	lock();
	threads.insert(tp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Remove a thread from the list
//

void MprThreadService::removeThread(MprThread *tp)
{
	lock();
	threads.remove(tp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Nothing to do
//

int MprThreadService::start()
{
#if BLD_DEBUG && UNUSED
	MprThread	*tp;

	if (!mprGetDebugMode()) {
		tp = new MprThread(lockBuster, MPR_LOW_PRIORITY, 0, "watch");
		tp->start();
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Nothing to do. We expect the threads to be self terminating.
//

int MprThreadService::stop(int timeout)
{
	//
	//  Wait until all threads (except main thread) have exited
	//
	while (threads.getNumItems() > 1 && timeout > 0) {
		mprSleep(10);
		timeout -= 10;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the current thread object
//

MprThread *MprThreadService::getCurrentThread()
{
	MprThread	*tp;
	int			id;

	lock();
	id = (int) GetCurrentThreadId();
	tp = (MprThread*) threads.getFirst();
	while (tp) {
		if (tp->getOsThread() == id) {
			unlock();
			return tp;
		}
		tp = (MprThread*) threads.getNext(tp);
	}
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the current thread id
//

int MprThreadService::getCurrentThreadId()
{
	MprThread	*tp;

	tp = getCurrentThread();
	if (tp) {
		return tp->getId();
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MprThread ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a main thread
//

MprThread::MprThread(int priority, char *name)
{
	osThreadId = 0;
	pid = getpid();
	threadHandle = 0;
	this->priority = priority;
	entry = 0;
	data = 0;
	this->name = mprStrdup(name);

	mutex = new MprMutex();
	//
	//	Inserted into the thread list in MprThreadService()
	//
}

////////////////////////////////////////////////////////////////////////////////
//
//	Create a thread
//

MprThread::MprThread(MprThreadProc entry, int priority, void *data, 
	char *name, int stackSize)
{
	osThreadId = 0;
	pid = getpid();
	threadHandle = 0;
	this->priority = priority;
	this->entry = entry;
	this->data = data;
	this->name = mprStrdup(name);

	mutex = new MprMutex();
	mprGetMpr()->threadService->insertThread(this);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a thread
//

MprThread::~MprThread()
{
	lock();
	mprLog(MPR_INFO, "Thread exiting %s (%x)\n", name, osThreadId);

	mprGetMpr()->threadService->removeThread(this);
	mprFree(name);
	if (threadHandle) {
		CloseHandle(threadHandle);
	}
	delete mutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start a thread
//

int MprThread::start()
{
	HANDLE			h;
	uint			threadId;

	lock();
	h = (HANDLE) _beginthreadex(NULL, 0, threadProcWrapper, (void*) this, 
		0, &threadId);
	if (h == NULL) {
		unlock();
		return MPR_ERR_CANT_INITIALIZE;
	}
	osThreadId = (ulong) threadId;
	threadHandle = (HANDLE) h;

	setPriority(priority);
	mprLog(MPR_INFO, "Created thread %s (%x)\n", name, threadId);
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Thread entry function
//

static uint __stdcall threadProcWrapper(void *data) 
{
	MprThread		*tp;

	tp = (MprThread*) data;
	tp->threadProc();
	delete tp;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Thread procedure
//

void MprThread::threadProc()
{
	osThreadId = GetCurrentThreadId();
	pid = getpid();
	(entry)(data, this);
}

////////////////////////////////////////////////////////////////////////////////

void MprThread::setPriority(int newPriority)
{
	int		osPri;

	lock();
	osPri = mapMprPriorityToOs(newPriority);
	SetThreadPriority(threadHandle, osPri);
	priority = newPriority;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprThread::setOsThread(MprOsThread id)
{
	osThreadId = id;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Map Mpr priority to Windows native priority. Windows priorities range from
//	-15 to +15 (zero is normal). Warning: +15 will not yield the CPU, -15 may
//	get starved. We should be very wary going above +11.
//

int MprThread::mapMprPriorityToOs(int mprPriority)
{
	mprAssert(mprPriority >= 0 && mprPriority <= 100);
 
	if (mprPriority <= MPR_BACKGROUND_PRIORITY) {
		return THREAD_PRIORITY_LOWEST;
	} else if (mprPriority <= MPR_LOW_PRIORITY) {
		return THREAD_PRIORITY_BELOW_NORMAL;
	} else if (mprPriority <= MPR_NORMAL_PRIORITY) {
		return THREAD_PRIORITY_NORMAL;
	} else if (mprPriority <= MPR_HIGH_PRIORITY) {
		return THREAD_PRIORITY_ABOVE_NORMAL;
	} else {
		return THREAD_PRIORITY_HIGHEST;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//	Map Windows priority to Mpr priority
// 

int MprThread::mapOsPriorityToMpr(int nativePriority)
{
	int		priority;

	priority = (45 * nativePriority) + 50;
	if (priority < 0) {
		priority = 0;
	}
	if (priority >= 100) {
		priority = 99;
	}
	return priority;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MprMutex ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a mutex
// 

MprMutex::MprMutex()
{
	memset(&cs, 0, sizeof(cs));
	InitializeCriticalSectionAndSpinCount(&cs, 5000);

#if BLD_DEBUG
	addMutex(this);
	cs.DebugInfo->Spare[DEBUG_ID] = -1;				// Thread ID goes here
	cs.DebugInfo->Spare[DEBUG_TIME] = 0;			// Time goes here
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//	Destroy a mutex. Must be locked on entrance.
// 

MprMutex::~MprMutex()
{
	DeleteCriticalSection(&cs);
#if BLD_DEBUG
	removeMutex(this);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//	Lock a mutex
// 

void MprMutex::lock()
{
	EnterCriticalSection(&cs);

#if BLD_DEBUG 
	mprAssert(cs.RecursionCount >= 0);
	mprAssert(cs.RecursionCount < MPR_MAX_RECURSION);
	mprAssert(cs.LockCount < MPR_MAX_BLOCKED_LOCKS);

	cs.DebugInfo->Spare[DEBUG_ID] = (ulong) GetCurrentThreadId();
	cs.DebugInfo->Spare[DEBUG_TIME] = GetTickCount();
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//	Try to lock a mutex. Do not block!
// 

int MprMutex::tryLock()
{
	mprAssert(cs.RecursionCount >= 0);
	if (TryEnterCriticalSection(&cs) == 0) {
		return MPR_ERR_BUSY;
	}
#if BLD_DEBUG
	cs.DebugInfo->Spare[DEBUG_ID] = (ulong) GetCurrentThreadId();
	cs.DebugInfo->Spare[DEBUG_TIME] = GetTickCount();
#endif
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
//	Unlock a mutex.  structure
// 

void MprMutex::unlock()
{
#if BLD_DEBUG
	mprAssert(cs.DebugInfo->Spare[DEBUG_ID] == (ulong) GetCurrentThreadId());
	mprAssert(cs.RecursionCount > 0);
	mprAssert(cs.RecursionCount < MPR_MAX_RECURSION);
	mprAssert(cs.LockCount < MPR_MAX_LOCKS);
	cs.DebugInfo->Spare[DEBUG_TIME] = GetTickCount();
#endif
	LeaveCriticalSection(&cs);
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MprCond ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a condition variable for use by single or multiple waiters
// 

MprCond::MprCond()
{
#if UNUSED
	wakeAll = 0;
	numWaiting = 0;
#endif
	triggered = 0;
	mutex = new MprMutex();
	cv = CreateEvent(NULL, FALSE, FALSE, NULL);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a condition variable
// 

MprCond::~MprCond()
{
	mutex->lock();
	CloseHandle(cv);
	delete mutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Wait for the event to be triggered. Should only be used when there are
//	single waiters. If the event is already triggered, then it will return 
//	immediately.
//

int MprCond::waitForCond(long timeout)
{
	if (timeout < 0) {
		timeout = MAXINT;
	}

	if (WaitForSingleObject(cv, timeout) != WAIT_OBJECT_0) {
		return -1;
	}

	//
	//	Reset the event
	//
	mutex->lock();
	mprAssert(triggered != 0);
	triggered = 0;
	ResetEvent(cv);
	mutex->unlock();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED
//
//	Wait for a condition variable to be signalled. Suitable for when there are
//	multiple waiters. It will work for single waiters, but is slower than 
//	waitForCond() above. Depending on whether signalCond() or signalAll() is 
//	used, a single waiter or multiple waiters may awake with one invocation of 
//	signal. If the condition is already triggered, this routine will not block 
//	for the first waiter. NOTE: the externalMutex must be defined and locked on 
//	entry to this routine
// 

int MprCond::multiWait(MprMutex *externalMutex, long timeout)
{
	int		now, deadline;

	if (timeout < 0) {
		timeout = MAXINT;
	}
	now = mprGetTime(0);
	deadline = now + timeout;

	while (now <= deadline) {
		mutex->lock();
		numWaiting++;
		mutex->unlock();

		externalMutex->unlock();

		if (WaitForSingleObject(cv, timeout) != WAIT_OBJECT_0) {
			return -1;
		}

		mutex->lock();
		--numWaiting;
		if (wakeAll) {
			//
			//	Last thread to awake must reset the event
			//
			if (numWaiting == 0) {
				wakeAll = 0;
				ResetEvent(cv);
			}
			mutex->unlock();
			break;
		}
		if (triggered) {
			triggered = 0;
			ResetEvent(cv);
			mutex->unlock();
			break;
		}
		mutex->unlock();
		now = mprGetTime(0);
	}
	externalMutex->lock();
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Signal a condition and wakeup the waiter. Note: this may be called 
//	prior to the waiter waiting.
//
 
void MprCond::signalCond()
{
	mutex->lock();
	triggered = 1;
#if UNUSED
	wakeAll = 0;
#endif
	SetEvent(cv);
	mutex->unlock();
}

///////////////////////////////////////////////////////////////////////////////
#if UNUSED
//
//	Signal all waiters. Note: this may be called prior to anyone waiting.
//

void MprCond::signalAll()
{
	mutex->lock();
	triggered = 1;
#if UNUSED
	wakeAll = 1;
#endif
	SetEvent(cv);
	mutex->unlock();
}

///////////////////////////////////////////////////////////////////////////////
//
//	Use very carefully when you really know what you are doing. More dangerous
//	than it looks
//

void MprCond::reset()
{
	mutex->lock();
	triggered = 0;
#if UNUSED
	wakeAll = 0;
#endif
	ResetEvent(cv);
	mutex->unlock();
}

#endif
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Debug //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if BLD_DEBUG
#if UNUSED

static void	lockBuster(void *data, MprThread *tp)
{
	MprMutex		*mp;
	int			duration;
	uint		tid, whenLocked, ticks;

	tid = (int) GetCurrentThreadId();
	while (! mprGetMpr()->isExiting()) {
		if (numMutex > MPR_MAX_LOCKS) {
			mprError(MPR_L, MPR_LOG, "Too many mutexes");
		}

		mp = (MprMutex*) mutexList.getFirst();
		while (mp) {
			whenLocked = mp->cs.DebugInfo->Spare[DEBUG_TIME];

			if (mp->cs.RecursionCount > 0) {
				ticks = GetTickCount();
				if (ticks < whenLocked) {
					duration = 0xFFFFFFFF - whenLocked + 1 + ticks;
				} else {
					duration = ticks - whenLocked;
				}
				if (duration > MPR_MAX_LOCK_TIME) {
					mprError(MPR_L, MPR_LOG, "Mutex held too long");
					//	FUTURE -- could bust the lock here ... mp->unlock();
				}
			}
			mp = (MprMutex*) mutexList.getNext(&mp->link);
		}
		mprSleep(500);
	}
}

#endif // UNUSED
///////////////////////////////////////////////////////////////////////////////
//
//	Add this mutex to the list
// 

static void addMutex(MprMutex *mp)
{
	//
	//	First time initialization
	//
	if (numMutex < 0) {
		memset(&debugCs, 0, sizeof(debugCs));
		InitializeCriticalSectionAndSpinCount(&debugCs, 4000);
	}
	EnterCriticalSection(&debugCs);
	mutexList.insert(&mp->link);
	numMutex++;
	LeaveCriticalSection(&debugCs);
}

///////////////////////////////////////////////////////////////////////////////
//
//	Remove this mutex from the list
// 

static void removeMutex(MprMutex *mp)
{
	EnterCriticalSection(&debugCs);
	numMutex--;
	mutexList.remove(&mp->link);
	LeaveCriticalSection(&debugCs);
}

///////////////////////////////////////////////////////////////////////////////
//
//	Return the number of mutexes owned by this thread
// 

int MprDebug::getMutexNum()
{
	MprMutex		*mp;
	int			num;
	uint		tid;

	tid = (uint) GetCurrentThreadId();

	num = 0;
	mp = (MprMutex*) mutexList.getFirst();
	while (mp) {

		if (mp->cs.DebugInfo->Spare[DEBUG_ID] == tid) {
			num++;
		}
		mp = (MprMutex*) mutexList.getNext(&mp->link);
	}
	return num;
}

#endif // BLD_DEBUG
#endif // BLD_FEATURE_MULTITHREAD
///////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
