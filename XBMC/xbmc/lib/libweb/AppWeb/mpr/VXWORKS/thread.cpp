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
SEM_ID				listLock;
#endif

///////////////////////////// Forward Declarations /////////////////////////////

static int	threadProcWrapper(void *data);

///////////////////////////////////// Code /////////////////////////////////////
//
//	Initialize the thread service
//

MprThreadService::MprThreadService()
{
#if BLD_DEBUG
	
	listLock = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE);
#endif

	mutex = new MprMutex();
	//
	//	Don't actually create the thread. Just create a thead object for
	//	this main thread
	//
	mainThread = new MprThread(MPR_NORMAL_PRIORITY, "main");
	mainThread->setId(taskIdSelf());
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

	semTake(listLock, WAIT_FOREVER);
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
	semGive(listLock);
	semDelete(listLock);
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
	MprThread	*tp;
	int			osThreadId;

	osThreadId = taskIdSelf();
	lock();
	tp = (MprThread*) threads.getFirst();
	while (tp) {
		if (tp->getId() == osThreadId) {
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
	pid = osThreadId = 0;

	stackSize = mprGetMpr()->getConfigInt("stackSize", MPR_DEFAULT_STACK);

	this->priority = priority;
	this->name = mprStrdup(name);
	entry = 0;
	data = 0;

	mutex = new MprMutex();
	//
	//	Will be inserted into the thread list in MprThreadService()
	//
}

////////////////////////////////////////////////////////////////////////////////
//
//	Create a thread
//

MprThread::MprThread(MprThreadProc entry, int priority, void *data, 
	char *name, int stackSize)
{
	mprLog(7, "Create Thread %s\n", name);

	pid = osThreadId = 0;

	if (stackSize == 0) {
		this->stackSize = mprGetMpr()->getConfigInt("stackSize", MPR_DEFAULT_STACK);
	} else {
		this->stackSize = stackSize;
	}

	this->priority = priority;
	this->name = mprStrdup(name);
	this->entry = entry;
	this->data = data;

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
	int		taskHandle, pri;

	taskPriorityGet(taskIdSelf(), &pri);

	taskHandle = taskSpawn(name, pri, 0, stackSize, 
		(FUNCPTR) threadProcWrapper, 
		(int) this, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (taskHandle < 0) {
		mprError(MPR_L, MPR_FATAL, "Can't create task %s\n", name);
		return MPR_ERR_CANT_CREATE;
	}
	mprLog(MPR_DEBUG, "Started thread %s, taskHandle %x\n", name, taskHandle);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Must be called before start()
//

void MprThread::setStackSize(int size)
{
	stackSize = size;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Entry thread function
// 

static int threadProcWrapper(void *data) 
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
	pid = osThreadId = taskIdSelf();
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
	int		rc, rcGet, osPri, finalOsPri;

	lock();

	osPri = mapMprPriorityToOs(newPriority);

	rc = taskPrioritySet(osThreadId, osPri);
	rcGet = taskPriorityGet(osThreadId, &finalOsPri);

	if (rc != 0 || rcGet != 0) {
		mprLog(0, "ERROR Setting prioority rc %d, rcGet %d\n", rc, rcGet);
	}

	if (finalOsPri != osPri) {
		mprLog(0, "WARNING priority not changed. Wanted %d but got %d, rc %d\n",
			osPri, finalOsPri, rc);
		rc = taskPrioritySet(osThreadId, osPri);
		taskPriorityGet(osThreadId, &finalOsPri);
		mprLog(0, "AFTER RETRY. Wanted %d but got %d, rc %d\n",
			osPri, finalOsPri, rc);
	}
	priority = newPriority;

	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Map MR priority to VxWorks native priority. VxWorks priorities range from 
//	0 to 255 with zero being the highest. Appweb priorities range from 0 to 99
//	with 99 being the highest.
//

int MprThread::mapMprPriorityToOs(int mprPriority)
{
	int		nativePriority;

	mprAssert(mprPriority >= 0 && mprPriority < 100);

	nativePriority = (100 - mprPriority) * 5 / 2;

	if (nativePriority < 10) {
		nativePriority = 10;
	} else if (nativePriority > 255) {
		nativePriority = 255;
	}
	return nativePriority;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Map O/S priority to Mpr priority.
// 

int MprThread::mapOsPriorityToMpr(int nativePriority)
{
	int		priority;

	priority = (255 - nativePriority) * 2 / 5;
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
	cs = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE);
	if (cs == 0) {
		mprAssert(0);
		return;
	}

#if BLD_DEBUG
	semTake(listLock, WAIT_FOREVER);
	mutexList.insert(&link);
	semGive(listLock);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a mutex. Must be locked on entrance.
// 

MprMutex::~MprMutex()
{
	semDelete(cs);

#if BLD_DEBUG
	semTake(listLock, WAIT_FOREVER);
	mutexList.remove(&link);
	semGive(listLock);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Lock a mutex
// 

void MprMutex::lock()
{
	int		rc;

	rc = semTake(cs, WAIT_FOREVER);
	if (rc == -1) {
		mprAssert(0);
		mprLog("semTake error %d\n", rc);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Unlock a mutex
// 

void MprMutex::unlock()
{
	semGive(cs);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Try to lock a mutex. Do not block!
// 

int MprMutex::tryLock()
{
	int		rc;

	rc = semTake(cs, NO_WAIT);
	if (rc == -1) {
		if (rc == S_objLib_OBJ_UNAVAILABLE) {
			return MPR_ERR_BUSY;
		} else {
			return MPR_ERR_CANT_ACCESS;
		}
	}
	
	//	Success
	return 0;
}

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

	cv = semCCreate(SEM_Q_PRIORITY, SEM_EMPTY);

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

	semDelete(cv);

#if BLD_DEBUG
	condList.remove(&link);
#endif
	delete mutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Wait for the event to be triggered. Should only be used when there are
//	single waiters. If the event is already triggered, then it will return 
//	immediately. Timeout of -1 means wait forever. Timeout of 0 means no wait.
//	Returns 0 if the event was signalled. Returns < 0 if the timeout.
//

int MprCond::waitForCond(long timeout)
{
	int		rc;

	rc = 0;

	mutex->lock();
	if (triggered == 0) {
		mutex->unlock();
		if (timeout < 0) {
			rc = semTake(cv, WAIT_FOREVER);
		} else {
			rc = semTake(cv, timeout);
		}
		mutex->lock();
	}
	triggered = 0;
	mutex->unlock();

	if (rc == S_objLib_OBJ_UNAVAILABLE) {
		//	FUTURE -- should be consistent with return codes in tryLock
		return MPR_ERR_TIMEOUT;
	} else {
		return MPR_ERR_GENERAL;
	}
	return rc;
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
	int		rc;

	rc = 0;

	externalMutex->lock();
	if (triggered == 0) {
		externalMutex->unlock();
		if (timeout < 0) {
			rc = semTake(cv, WAIT_FOREVER);
		} else {
			rc = semTake(cv, timeout);
		}
		externalMutex->lock();
	}
	triggered = 0;
	externalMutex->unlock();

	if (rc == S_objLib_OBJ_UNAVAILABLE) {
		//	FUTURE -- should be consistent with return codes in tryLock
		return MPR_ERR_TIMEOUT;
	} else {
		return MPR_ERR_GENERAL;
	}
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
	if (triggered) {
		mutex->unlock();
		return;
	}

	triggered = 1;
	semGive(cv);
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

	//	WARNING: Not quite right
	rc = semGive(cv);
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
#endif // BLD_FEATURE_MULTITHREAD

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
