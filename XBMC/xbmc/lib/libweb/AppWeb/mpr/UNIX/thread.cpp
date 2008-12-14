///
///	@file 	UNIX/thread.cpp
/// @brief 	Primitive multi-threading support for Linux
///	@overview This module provides threading, mutex and condition variable 
///		APIs for UNIX. It tracks MprMutex and MprCond allocations and leaks 
///		if BLD_DEBUG is defined.
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
/////////////////////////////////// Includes ///////////////////////////////////

#include	"mpr/mpr.h"

//////////////////////////////////// Locals ////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

#if BLD_DEBUG
MprList				mutexList;
MprList				condList;
pthread_mutex_t	 	listLock;
#endif

///////////////////////////// Forward Declarations /////////////////////////////

static void	*threadProcWrapper(void *data);

///////////////////////////////////// Code /////////////////////////////////////
//
//	Initialize the thread service
//

MprThreadService::MprThreadService()
{
#if BLD_DEBUG
	pthread_mutexattr_t		attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	memset(&listLock, 0, sizeof(listLock));
	pthread_mutex_init(&listLock, &attr);
	pthread_mutexattr_destroy(&attr);
#endif

	mutex = new MprMutex();
	//
	//	Don't actually create the thread. Just create a thead object for
	//	this main thread
	//
	mainThread = new MprThread(MPR_NORMAL_PRIORITY, "main");
	mainThread->setOsThread(pthread_self());
	insertThread(mainThread);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Terminate the thread service
// 

MprThreadService::~MprThreadService()
{
	lock();
	//
	//	We expect users who created the threads to delete them prior to this
	//
	delete mainThread;
	delete mutex;
	mutex = 0;

#if BLD_DEBUG
	if (threads.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d thread(s) unfreed", 
			threads.getNumItems());
	}
	if (condList.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d cond var(s) unfreed", 
			condList.getNumItems());
	}

	pthread_mutex_lock(&listLock);
	if (mutexList.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d mutex(es) unfreed", 
			mutexList.getNumItems() - 0);

		MprMutex *mp;
		mp = (MprMutex*) mutexList.getFirst();
		while (mp) {
			mprLog(0, "Mutex %x unfreed\n", mp);
			mp = (MprMutex*) mutexList.getNext(&mp->link);
		}
	}
	pthread_mutex_unlock(&listLock);
	pthread_mutex_destroy(&listLock);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a thread to the list
//

void MprThreadService::insertThread(MprThread *tp)
{
	lock();
	threads.insert(tp);
	tp->setId(threads.getNumItems() - 1);

	if (threads.getNumItems() > MPR_MAX_SPINLOCK) {
		static int once = 0;

		if (once++ == 0) {
			mprError(MPR_L, MPR_LOG, "Too many threads, increase MPR_MAX_SPINLOCK");
		} 
	}
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
	MprThread		*tp;
	MprOsThread		osThreadId;

	osThreadId = pthread_self();
	lock();
	tp = (MprThread*) threads.getFirst();
	while (tp) {
		if (tp->getOsThread() == osThreadId) {
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
	mprLog(7, "Create Thread %s\n", name);

	osThreadId = 0;
	pid = getpid();
	this->priority = priority;
	entry = 0;
	data = 0;
	id = -1;
	this->name = mprStrdup(name);
	mutex = new MprMutex();

	stackSize = mprGetMpr()->getConfigInt("stackSize", MPR_DEFAULT_STACK);

	//
	//	Caller must insert into the thread list
	//
}

////////////////////////////////////////////////////////////////////////////////
//
//	Create a thread
//

MprThread::MprThread(MprThreadProc entry, int priority, void *data, char *name,
	int stackSize)
{
	Mpr		*mpr;

	mpr = mprGetMpr();
	mprLog(7, "Create Thread %s\n", name);

	osThreadId = 0;
	pid = 0;
	this->priority = priority;
	this->entry = entry;
	this->data = data;
	this->name = mprStrdup(name);
	this->stackSize = stackSize;
	mutex = new MprMutex();

	if (stackSize == 0) {
		this->stackSize = mpr->getConfigInt("stackSize", MPR_DEFAULT_STACK);
	} else {
		this->stackSize = stackSize;
	}

	if (mpr->threadService) {
		mpr->threadService->insertThread(this);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a thread
//

MprThread::~MprThread()
{
	lock();
	mprFree(name);
	mprGetMpr()->threadService->removeThread(this);
	delete mutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start a thread
//

int MprThread::start()
{
	pthread_attr_t	attr;
	pthread_t		h;
	int				stackSize;

	stackSize = mprGetMpr()->getConfigInt("stackSize", MPR_DEFAULT_STACK);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, stackSize);

	if (pthread_create(&h, &attr, threadProcWrapper, (void*) this) != 0) { 
		mprAssert(0);
		pthread_attr_destroy(&attr);
		return MPR_ERR_CANT_CREATE;
	}
	pthread_attr_destroy(&attr);

	mprLog(MPR_DEBUG, "Started thread %s, handle %x\n", name, h);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Entry thread function
// 

static void *threadProcWrapper(void *data) 
{
	MprThread	*tp;

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
	osThreadId = pthread_self();
	pid = getpid();
	(entry)(data, this);
}

////////////////////////////////////////////////////////////////////////////////

void MprThread::setOsThread(MprOsThread id)
{
	osThreadId = id;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Change the thread priority
// 

void MprThread::setPriority(int newPriority)
{
	int		osPri;

	lock();
	osPri = mapMprPriorityToOs(newPriority);
	setpriority(PRIO_PROCESS, pid, osPri);
	priority = newPriority;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Map MR priority to linux native priority. Unix priorities range from 
//	-19 to +19. Linux does -20 to +19. Our Priority mappings:
//

int MprThread::mapMprPriorityToOs(int mprPriority)
{
	mprAssert(mprPriority >= 0 && mprPriority < 100);

	if (mprPriority <= MPR_BACKGROUND_PRIORITY) {
		return 19;
	} else if (mprPriority <= MPR_LOW_PRIORITY) {
		return 10;
	} else if (mprPriority <= MPR_NORMAL_PRIORITY) {
		return 0;
	} else if (mprPriority <= MPR_HIGH_PRIORITY) {
		return -8;
	} else {
		return -19;
	}
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Map O/S priority to Mpr priority.
// 

int MprThread::mapOsPriorityToMpr(int nativePriority)
{
	int		priority;

	priority = (nativePriority + 19) * (100 / 40); 
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
	pthread_mutexattr_t		attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	memset(&cs, 0, sizeof(cs));
	pthread_mutex_init(&cs, &attr);
	pthread_mutexattr_destroy(&attr);

#if BLD_DEBUG
	pthread_mutex_lock(&listLock);
	mutexList.insert(&link);
	pthread_mutex_unlock(&listLock);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a mutex. Must be locked on entrance.
// 

MprMutex::~MprMutex()
{
	pthread_mutex_unlock(&cs);
	pthread_mutex_destroy(&cs);
#if BLD_DEBUG
	pthread_mutex_lock(&listLock);
	mutexList.remove(&link);
	pthread_mutex_unlock(&listLock);
#endif
}

////////////////////////////////////////////////////////////////////////////////
#if NOT_INLINE
//
//	Lock a mutex
// 

void MprMutex::lock()
{
	pthread_mutex_lock(&cs);
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Try to lock a mutex. Do not block!
// 

int MprMutex::tryLock()
{
	int		err;

	if ((err = pthread_mutex_trylock(&cs)) != 0) {
		if (err == EBUSY) {
			return MPR_ERR_BUSY;
		} else {
			return MPR_ERR_CANT_ACCESS;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if NOT_INLINE
//
//	Unlock a mutex
// 

void MprMutex::unlock()
{
	//
	//	FUTURE -- could add more debug tests here to ensure it is being unlocked
	//	by the owning thread
	//
	pthread_mutex_unlock(&cs);
}

#endif
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MprCond ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a condition variable for use by single or multiple waiters
// 

MprCond::MprCond()
{
	memset(&cv, 0, sizeof(cv));
	mutex = new MprMutex();
	triggered = 0;
	pthread_cond_init(&cv, NULL);
#if BLD_DEBUG
	condList.insert(&link);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a condition variable object
// 

MprCond::~MprCond()
{
	mutex->lock();
	pthread_cond_destroy(&cv);
#if BLD_DEBUG
	condList.remove(&link);
#endif
	delete mutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Wait for the event to be triggered. Should only be used when there are
//	single waiters. If the event is already triggered, then it will return 
//	immediately. Timeout of -1 means wait forever. Returns 0 if the event
//	was signalled. Returns < 0 if the timeout 
//

int MprCond::waitForCond(long timeout)
{
	struct timespec		waitTill;
	struct timeval		current;
	int					rc;

	mutex->lock();
	rc = 0;
	if (triggered == 0) {
		//
		//	The pthread_cond_wait routines will atomically unlock the mutex
		//	before sleeping and will relock on awakening.
		//
		if (timeout < 0) {
			rc = pthread_cond_wait(&cv, &mutex->cs);
		} else {
			gettimeofday(&current, NULL);
			waitTill.tv_sec = current.tv_sec + (timeout / 1000);
			waitTill.tv_nsec = current.tv_usec + (timeout % 1000) * 1000000;
			rc = pthread_cond_timedwait(&cv, &mutex->cs, &waitTill);
		}
	}
	triggered = 0;
	mutex->unlock();
	if (rc == ETIMEDOUT) {
		return MPR_ERR_TIMEOUT;
	} else if (rc != 0) {
		return MPR_ERR_GENERAL;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED
//
//	Wait for a condition variable to be signalled. Suitable for when there are
//	multiple waiters. It will work for single waiters, but is slower than 
//	waitForCond() above. Depending on whether signalCond() or signalAll() 
//	is used, a single waiter or multiple waiters may awake with one invocation 
//	of signal. If the condition is already triggered, this routine will not 
//	block for the first waiter. NOTE: the externalMutex must be defined and 
//	locked on entry to this routine
// 

int MprCond::multiWait(MprMutex *externalMutex, long timeout)
{
	struct timespec			waitTill;
	struct timeval			now;
	int						rc;

	rc = 0;
	if (triggered == 0) {
		//
		//	The pthread_cond_wait routines will atomically unlock the 
		//	externalMutex before sleeping and will relock on awakening.
		//
		if (timeout < 0) {
			rc = pthread_cond_wait(&cv, &externalMutex->cs);
		} else {
			gettimeofday(&now, NULL);
			waitTill.tv_sec = now.tv_sec + (timeout / 1000);
			waitTill.tv_nsec = (now.tv_usec + ((timeout % 1000) * 1000)) * 1000;
			rc = pthread_cond_timedwait(&cv, &externalMutex->cs, &waitTill);
		}
	}
	triggered = 0;
	return rc;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Signal a condition and wakeup a single waiter. Note: this may be called
//	prior to the waiter waiting.
// 

void MprCond::signalCond()
{
	mutex->lock();
	triggered = 1;
	pthread_cond_signal(&cv);
	mutex->unlock();
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED
//
//	Signal all waiters. Note: this may be called prior to anyone waiting.
// 

void MprCond::signalAll()
{
	mutex->lock();
	triggered = 1;
	pthread_cond_broadcast(&cv);
	mutex->unlock();
}

///////////////////////////////////////////////////////////////////////////////

void MprCond::reset()
{
	mutex->lock();
	triggered = 0;
	mutex->unlock();
}

#endif
///////////////////////////////////////////////////////////////////////////////
/*
 *	Array of threads interested in acquiring the spinlock
 */
#if __UCLIBC__ || CYGWIN
static volatile int interested[MPR_MAX_SPINLOCK + 1];
static volatile int flag1, flag2;
#endif

void mprSpinInit(MprSpinLock *sp)
{
#if !__UCLIBC__ && !MACOSX && !CYGWIN
	pthread_spin_init(sp, 0);
#endif
}


void mprSpinDestroy(MprSpinLock *sp)
{
#if !__UCLIBC__ && !MACOSX && !CYGWIN
	pthread_spin_destroy(sp);
#endif
}



void mprSpinLock(MprSpinLock *sp)
{
#if MACOSX
	while (OSAtomicTestAndSet(0, (void*) sp) != 0) {
		;
	}
#elif !__UCLIBC__ && !CYGWIN
	pthread_spin_lock(sp);
#else
	int		id, j;

	/*
	 * 	Wan't IDs to be origin 1
	 */
	id = mprGetCurrentThreadId() + 1;
	mprAssert(0 <= id && id < MPR_MAX_SPINLOCK);

	mprAssert(interested[id] == 0);

	/*
	 * 	Lamport's N-process mutual exclusion algorithm
	 */
	while (1) {

		interested[id] = 1;
		flag1 = id;
		if (flag2)  {
			interested[id] = 0;
			while (flag2) ;
			continue;
		}

		flag2 = id;
		if (flag1 == id) {
			break;
		}

		interested[id] = 0;
		for (j = 0; j < id; ) {
			while (interested[j]) ;
		}
		if (flag2 != id) {
			while (flag2) ;
		}
	}
#endif
}



void mprSpinUnlock(MprSpinLock *sp)
{
#if MACOSX
	OSAtomicTestAndClear(0, (void*) sp);
#elif !__UCLIBC__ && !CYGWIN
	pthread_spin_unlock(sp);
#else
	int 	id;
	
	id = mprGetCurrentThreadId() + 1;

	mprAssert(flag2);
	flag2 = 0;
	mprAssert(interested[id]);
	interested[id] = 0;
#endif
}

///////////////////////////////////////////////////////////////////////////////
#endif // BLD_FEATURE_MULTITHREAD

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
