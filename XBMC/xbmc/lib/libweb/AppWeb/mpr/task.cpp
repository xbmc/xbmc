///
///	@file 	task.cpp
/// @brief 	Task and thread pool service
/// @overview The MPR provides a high peformance thread service 
///		where tasks can be queued and dispatched by a pool of 
///		preallocated threads.
///	@remarks This module is thread-safe.
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
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr.h"

/////////////////////////// Forward Declarations ///////////////////////////////

#if BLD_FEATURE_MULTITHREAD
static void	threadMainWrapper(void *arg, MprThread *tp);
static void	pruner(void *arg, MprTimer *timer);
#endif

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MprTask ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Constructor for a thread pool
//

MprPoolService::MprPoolService(char *taskName)
{
	mprAssert(taskName && *taskName);

#if BLD_FEATURE_LOG
	log = new MprLogModule("pool");
#endif

	name = mprStrdup(taskName);
	nextTaskNum = 0;

#if BLD_FEATURE_MULTITHREAD
	incMutex = new MprMutex();
	maxUseThreads = 0;
	maxThreads = MPR_DEFAULT_MIN_THREADS;
	minThreads = MPR_DEFAULT_MIN_THREADS;
	mutex = new MprMutex();
	nextThreadNum = 0;
	numThreads = 0;
	pruneHighWater = 0;
	pruneTimer = 0;
	stackSize = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Close the thread service
//

MprPoolService::~MprPoolService()
{
	lock();
	mprFree(name);

#if BLD_FEATURE_MULTITHREAD
	mprLog(MPR_INFO, log, "Pool thread usage: used %d, max limit %d\n", 
		maxUseThreads, maxThreads);

	if (pruneTimer) {
		pruneTimer->stop(MPR_TIMEOUT_STOP);
		pruneTimer->dispose();
		pruneTimer = 0;
	}
	delete mutex;

	incMutex->lock();
	delete incMutex;

#if BLD_DEBUG
	if (idleThreads.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d idle pool threads unfreed",
			idleThreads.getNumItems());
	}
	if (busyThreads.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d busy pool threads unfreed",
			busyThreads.getNumItems());
	}
	if (tasks.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d tasks unfreed",
			tasks.getNumItems());
	}
	if (runningTasks.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d running tasks unfreed",
			runningTasks.getNumItems());
	}
#endif
#endif
#if BLD_FEATURE_LOG
	delete log;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the thread service
//

int MprPoolService::start()
{
#if BLD_FEATURE_MULTITHREAD
	MprPoolThread	*pt;

	//
	//	Pre-create the threads in the task up to the min number
	//
	lock();
	while (numThreads < minThreads) {
		pt = new MprPoolThread(this, stackSize);
		idleThreads.insert(pt); 
		numThreads++;
		maxUseThreads = max(numThreads, maxUseThreads);
		pruneHighWater = max(numThreads, pruneHighWater);
		pt->start();
	}

	//
	//	Create a timer to trim excess threads in the pool
	//
	if (!mprGetDebugMode()) {
		pruneTimer = new MprTimer(MPR_TIMEOUT_PRUNER, pruner, (void*) this);
	}
	unlock();
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the thread service. We stop all threads. Users are responsible for 
//	tasks. FUTURE BUG: but if the tasks have not yet run, the users can't delete
//	them because we tell them they can only delete them in the callbacks.
//

int MprPoolService::stop(int timeout)
{
#if BLD_FEATURE_MULTITHREAD
	MprPoolThread		*thread, *next;

	lock();
	if (pruneTimer) {
		pruneTimer->stop(MPR_TIMEOUT_STOP);
		pruneTimer->dispose();
		pruneTimer = 0;
	}

	//
	//	Wake up all idle threads. Busy threads take care of themselves.
	//	An idle thread will wakeup, exit and be removed from the busy
	//	list and then delete the thread.
	//
	thread = (MprPoolThread*) idleThreads.getFirst();
	while (thread) {
		next = (MprPoolThread*) idleThreads.getNext(thread);
		//
		//	Must be in the busy queue when we wake it up
		//
		mprAssert(thread->head == &idleThreads);
		idleThreads.remove(thread);
		busyThreads.insert(thread);
		mprLog(6, log, "MprPoolService::stop: wakeup thread %x\n", thread);
		thread->wakeupThread();
		thread = next;
	}

	//
	//  Wait until all tasks and threads have exited 
	//
	while (timeout > 0 && (runningTasks.getNumItems() > 0 || numThreads > 0)) {
		unlock();
		mprLog(6, log, "stop: waiting for %d tasks(s) or %d thread(s)\n", 
			runningTasks.getNumItems(), numThreads);
		mprSleep(50);
		timeout -= 10;
		lock();
	}

	mprAssert(idleThreads.getNumItems() == 0);
	mprAssert(busyThreads.getNumItems() == 0);
	mprAssert(tasks.getNumItems() == 0);
	mprAssert(runningTasks.getNumItems() == 0);

	unlock();
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD
//
//	Define the new minimum number of threads. Pre-allocate the minimum.
//

void MprPoolService::setMinPoolThreads(int n)
{ 
	MprPoolThread	*pt;

	lock();
	minThreads = n; 
	if (numThreads < minThreads) {
		while (numThreads < minThreads) {
			pt = new MprPoolThread(this, stackSize);
			idleThreads.insert(pt); 
			numThreads++;
			maxUseThreads = max(numThreads, maxUseThreads);
			pruneHighWater = max(numThreads, pruneHighWater);
			pt->start();
		}
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Define a new maximum number of theads. Prune if currently over the max
//

void MprPoolService::setMaxPoolThreads(int n)
{
	lock();
	maxThreads = n; 
	if (numThreads > maxThreads) {
		prune();
	}
	if (minThreads > maxThreads) {
		minThreads = maxThreads;
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Define a new stack size for new threads. Existing threads unaffected.
//

void MprPoolService::setStackSize(int n)
{
	stackSize = n; 
}

#endif // BLD_FEATURE_MULTITHREAD
////////////////////////////////////////////////////////////////////////////////
//
//	Run a task. Queue the task in priority order and wakeup a thread if 
//	one is available. Otherwise the task will be serviced when a pool thread
//	becomes idle.
//

void MprPoolService::queueTask(MprTask *tp)
{
    MprTask	*np;
	Mpr		*mpr;

	mprLog(6, log, "queueTask: %x\n", tp);
	mpr = mprGetMpr();

	lock();
	if (mpr->isExiting()) {
		unlock();
		return;
	}

	//
	//	Add the task to the task list in priority order. Scan from the back
	//	of the queue
	//
	np = (MprTask*) tasks.getLast();
	while (np) {
		if (np->priority >= tp->priority) {
			break;
		}
		np = (MprTask*) tasks.getPrev(np);
	}
	if (np) {
		np->insertAfter(tp);
	} else {
		tasks.insert(tp);
	}

#if BLD_FEATURE_MULTITHREAD
	if (maxThreads == 0) {
		//
		//	If we are running single-threaded, let the task be run by the 
		//	main event loop. If running a service thread we must wake it 
		//	up to service the task.
		//	
		unlock();
		if (mpr->isRunningEventsThread()) {
			mpr->selectService->awaken(0);
		}

	} else {
		//
		//	Multi-threaded
		//
		unlock();
		dispatchTasks();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MprPoolService::queueRunningTask(MprTask *tp)
{
	lock();
	mprAssert(tp->head == &tasks);
	tasks.remove(tp);
	runningTasks.insert(tp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprPoolService::dequeueTask(MprTask *tp)
{
	lock();
	mprAssert(tp->head == &runningTasks);
	runningTasks.remove(tp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD
//
//	Called only from queueTasks
//

void MprPoolService::dispatchTasks()
{
	MprPoolThread	*pt;
	MprTask			*tp;
	int				i;

	mprLog(6, log, "dispatchTasks\n");
	lock();
	for (i = tasks.getNumItems(); i > 0; i--) {
		tp = (MprTask*) tasks.getFirst();
		mprAssert(tp);

		//
		//	Try to find an idle thread and wake it up. It will wakeup in 
		//	threadMain(). If not any available, then add another thread to the 
		//	pool. Must account for threads we've already created but have not
		//	yet gone to work and inserted themselves in the idle/busy queues.
		//
		pt = (MprPoolThread*) idleThreads.getFirst();
		if (pt) {
			mprAssert(pt->head == &idleThreads);
			idleThreads.remove(pt); 
			busyThreads.insert(pt);
			queueRunningTask(tp);
			pt->setTask(tp);
			mprLog(6, log, "dispatchTasks: wakeup thread %x\n", pt);
			pt->wakeupThread();

		} else if (numThreads < maxThreads) {
			//
			//	Can't find an idle thread. Try to create more threads in the 
			//	pool. Otherwise, we will have to wait. No need to wakeup the 
			//	thread -- it will immediately go to work.
			// 
			mprLog(5, log, "dispatchTasks: new thread %x %d %d\n", pt,
				numThreads, maxThreads);
			pt = new MprPoolThread(this, stackSize);
			numThreads++;
			maxUseThreads = max(numThreads, maxUseThreads);
			pruneHighWater = max(numThreads, pruneHighWater);
			busyThreads.insert(pt);
			queueRunningTask(tp);
			pt->setTask(tp);
			pt->start();

		} else {
			//
			//	No free threads and can't create anymore. We will have to 
			//	wait. Busy threads will call dispatch when they become idle
			//
			mprLog(5, log, "dispatchTasks: no free threads\n");
			break;
		}
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprPoolService::assignNextTask(MprPoolThread *pt)
{
	MprTask	*tp;

	lock();
	if ((tp = (MprTask*) tasks.getFirst()) == 0) {
		unlock();
		return 0;
	}
	mprLog(6, log, "assignNextTask: task %x to thread %x\n", tp, pt);

	queueRunningTask(tp);
	pt->setTask(tp);

	unlock();
	return 1;
}

#endif // BLD_FEATURE_MULTITHREAD
////////////////////////////////////////////////////////////////////////////////
//
//	Run pending tasks. Only used if single-threaded. If multi-threaded, tasks 
//	run using threads from the thread pool. Return > 0 if there are more tasks 
//	to run on this thread. While this implementation runs all tasks, we return 
//	non-zero to allow for a design when we only service a portion of the tasks.
//

int MprPoolService::runTasks()
{
	MprTask	*tp, *next;
	
#if BLD_FEATURE_MULTITHREAD
	//
	//	If multi-threaded, let the pool threads run the tasks
	//
	if (maxThreads > 0) {
		return 0;
	}
#endif

	lock();
	tp = (MprTask*) tasks.getFirst();
	while (tp) {
		next = (MprTask*) tasks.getNext(tp);
		queueRunningTask(tp);
		mprLog(6, log, "runTasks: task %x\n", tp);
		tp->flags |= MPR_TASK_RUNNING;
		tp->inUse++;
		unlock();

		(*tp->proc)(tp->data, tp);

		lock();
		tp->flags &= ~MPR_TASK_RUNNING;
#if BLD_FEATURE_MULTITHREAD
		if (tp->stoppingCond) {
			tp->stoppingCond->signalCond();
		}
#endif
		if (--tp->inUse == 0 && tp->flags & MPR_TASK_DISPOSED) {
			delete tp;
		}
		tp = next;
	}
	unlock();
	return (tp != 0) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD
//
//	Wrapper function for the prunter
//

static void pruner(void *arg, MprTimer *timer)
{
	MprPoolService		*pp;

	pp = (MprPoolService*) arg;
	pp->prune();
	timer->reschedule();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Trim idle threads from a task
//

void MprPoolService::prune()
{
	MprPoolThread	*pt, *nextPt;
	Mpr				*mpr;
	int				toTrim;

	mpr = mprGetMpr();

	//	FUTURE -- re-enable
	if (1 || mpr->isExiting()) {
		return;
	}
	mprLog(6, log, "prune: running\n");

	lock();

	//
	//	Prune half of what we could prune. This gives exponentional decay.
	//	We use the high water mark seen in the last period.
	//
	toTrim = (pruneHighWater - minThreads) / 2;

	pt = (MprPoolThread*) idleThreads.getFirst();
	while (toTrim-- > 0 && pt) {
		nextPt = (MprPoolThread*) idleThreads.getNext(pt);
		//
		//	Leave floating -- in no queue. The thread will kill itself.
		//
		mprAssert(pt->head == &idleThreads);
		mprAssert(pt->getCurrentTask() == 0);
		idleThreads.remove(pt); 
		mprLog(6, log, "prune: prune thread %x\n", pt);
		pt->wakeupThread();
		pt = nextPt;
	}
	pruneHighWater = minThreads;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Called from threadMainWrapper when the thread is exiting
//

void MprPoolService::removeThread(MprPoolThread *pt) 
{
	mprLog(6, log, "removeThread: %x\n", pt);

	lock();
	if (pt->head == &busyThreads) {
		busyThreads.remove(pt);
	}
	if (pt->head == &idleThreads) {
		idleThreads.remove(pt);
	}
	numThreads--;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprPoolService::lock() 
{
	mutex->lock();
}

////////////////////////////////////////////////////////////////////////////////

void MprPoolService::unlock()
{
	mutex->unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprPoolService::getNextThreadNum()
{
	int		rc;

	lock();
	rc = nextThreadNum++;
	unlock();
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_DEBUG

void MprPoolService::getStats(MprPoolStats *ps)
{
	mprAssert(ps);

	ps->maxThreads = maxThreads;
	ps->minThreads = minThreads;
	ps->numThreads = numThreads;
	ps->maxUse = maxUseThreads;
	ps->pruneHighWater = pruneHighWater;
	ps->idleThreads = idleThreads.getNumItems();
	ps->busyThreads = busyThreads.getNumItems();
}

#endif // BLD_DEBUG
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprPoolThread ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a new thread for the task
//

MprPoolThread::MprPoolThread(MprPoolService *pool, int stackSize)
{
	char	name[16];

	idleCond = new MprCond();
	this->pool = pool;
	currentTask = 0;
	flags = 0;

	mprSprintf(name, sizeof(name) - 1, "pool.%u", pool->getNextThreadNum());
	name[sizeof(name) - 1] = '\0';
	mprLog(6, pool->log, "MprPoolThread: New thread %s\n", name);

	thread = new MprThread(threadMainWrapper, MPR_POOL_PRIORITY, 
		(void*)this, name);
}

////////////////////////////////////////////////////////////////////////////////

MprPoolThread::~MprPoolThread()
{
    delete idleCond;
}

////////////////////////////////////////////////////////////////////////////////

void MprPoolThread::start()
{
	mprLog(6, pool->log, "MprPoolThread::start: %x\n", this);
	thread->start();
}

////////////////////////////////////////////////////////////////////////////////

void MprPoolThread::setTask(MprTask *tp)
{
	mprAssert(currentTask == 0);
	mprLog(6, pool->log, "setTask: poolThread %x, task %x\n", this, tp);

	currentTask = tp;
	tp->pt = this;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Wrapper function for the task thread main routine
//

static void threadMainWrapper(void *arg, MprThread *tp)
{
	MprPoolThread	*thread;
	MprPoolService	*pool;

	thread = (MprPoolThread*) arg;
	pool = thread->getMprPoolService();
	thread->threadMain();

	pool->lock();
	pool->removeThread(thread);
	delete thread;
	pool->unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Task thread main service routine
//

void MprPoolThread::threadMain()
{
	MprTask	*tp;
	Mpr		*mpr;

	mprLog(6, pool->log, "threadMain: begin thread %x\n", this);

	mpr = mprGetMpr();

	thread->setPriority(MPR_POOL_PRIORITY);

	pool->lock();

	while (!mpr->isExiting() && getList() != 0) {	// Not in list == pruned

		if (currentTask != 0) {
			mprAssert(head == &pool->busyThreads);
			mprLog(6, pool->log, "threadMain: %x, run task %x\n", 
				this, currentTask);

			mprAssert(currentTask->head == &pool->runningTasks);
			tp = currentTask;
			currentTask = 0;

			tp->flags |= MPR_TASK_RUNNING;
			tp->inUse++;
			pool->unlock();

			//	MOB -- tp can be deleted here

			thread->setPriority(tp->priority);
			(*tp->proc)(tp->data, tp);
			thread->setPriority(MPR_POOL_PRIORITY);

			pool->lock();
			tp->flags &= ~MPR_TASK_RUNNING;
			if (tp->stoppingCond) {
				tp->stoppingCond->signalCond();
			}
			mprAssert(tp->inUse >= 1);
			if (--tp->inUse == 0 && tp->flags & MPR_TASK_DISPOSED) {
				delete tp;
			}

			//
			//	Put this thread back on the idle list. But proc may have called
			//	makeIdle() to expediate going idle. So currentTask may 
			//	already be set by dispatchTasks while proc was running.
			//
			if (currentTask == 0 && head == &pool->busyThreads) {
				makeIdle();
			}

		} else if (pool->assignNextTask(this) != 0) {
			mprAssert(head == &pool->idleThreads);
			mprAssert(currentTask != 0);
			mprLog(6, "threadMain Assign a new currentTask %x\n", currentTask);
			pool->idleThreads.remove(this);
			pool->busyThreads.insert(this);

		} else {
			mprAssert(head == &pool->idleThreads);
			mprAssert(currentTask == 0);
			mprLog(6, pool->log, "threadMain: %x, sleeping\n", this);

			flags |= MPR_POOL_THREAD_SLEEPING;
			pool->unlock();
			idleCond->waitForCond(-1);
			pool->lock();
		}
	}
	pool->unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Called by some task procs to put the hosting thread back on the idle queue
//	more quickly.
//

void MprPoolThread::makeIdle()
{
	mprLog(6, pool->log, "makeIdle: %x\n", this);

	pool->lock();
	mprAssert(this->head == &pool->busyThreads);
	pool->busyThreads.remove(this);
	pool->idleThreads.insert(this);
	pool->unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Wakeup a thread when there is more work to do
//

void MprPoolThread::wakeupThread()
{
	pool->lock();
	if (flags & MPR_POOL_THREAD_SLEEPING) {
		mprLog(6, pool->log, "wakeupThread: %x\n", this);
		flags &= ~MPR_POOL_THREAD_SLEEPING;
		idleCond->signalCond(); 
	}
	pool->unlock();
}

#endif // BLD_FEATURE_MULTITHREAD
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MprTask ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a task on the default Mpr task service
//

MprTask::MprTask(MprTaskProc fn, void *arg, int pri)
{
	Mpr		*mpr;

	mpr = mprGetMpr();

	if (pri == 0) {
		pri = MPR_NORMAL_PRIORITY;
	}
	data = arg;
	flags = 0;
	inUse = 1;
	pool = mpr->poolService;
	priority = pri;
	proc = fn;
#if BLD_FEATURE_MULTITHREAD
	stoppingCond = 0;
#endif

	mprLog(6, pool->log, "new task: %x\n", this);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Create a task on a specific task service
//

MprTask::MprTask(MprPoolService *ps, MprTaskProc fn, void *arg, int pri)
{
	if (pri == 0) {
		pri = MPR_NORMAL_PRIORITY;
	}
	data = arg;
	flags = 0;
	inUse = 1;
	pool = ps;
	priority = pri;
	proc = fn;
#if BLD_FEATURE_MULTITHREAD
	stoppingCond = 0;
#endif

	mprLog(6, pool->log, "MprTask::MprTask: %x, pool %x\n", this, ps);
}

////////////////////////////////////////////////////////////////////////////////

bool MprTask::dispose()
{
	MprPoolService		*ps;

	//
	//	We may delete ourselves below. Must remember our pool service pointer
	//
	ps = pool;
	ps->lock();

	mprAssert(inUse > 0);
	if (flags & MPR_TASK_DISPOSED) {
		mprAssert(0);
		ps->unlock();
		return 0;
	}
	flags |= MPR_TASK_DISPOSED;

	if (--inUse == 0) {
		delete this;
		ps->unlock();
		return 1;
	}
	ps->unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MprTask::~MprTask()
{
	mprAssert(inUse == 0);
#if BLD_FEATURE_MULTITHREAD
	mprAssert(stoppingCond == 0);
#endif
	pool->dequeueTask(this);
}

////////////////////////////////////////////////////////////////////////////////

void MprTask::start()
{
	mprLog(6, pool->log, "MprTask::start %x\n", this);
	pool->queueTask(this);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return 1 if the task was stopped, 0 if it has completed, -1 for timeout
//

int MprTask::stop(int timeout)
{
	MprPoolService		*ps;
	int				rc;

	ps = pool;
	ps->lock();

	//
	//	If the task has not yet run, move it to the running queue and 
	//	pretend it has alread run.
	//
	if (head == &ps->tasks) {
		ps->queueRunningTask(this);
		ps->unlock();
		return 1;
	}

	inUse++;

#if BLD_FEATURE_MULTITHREAD
	//
	//	The task is running -- just wait for it to complete. Increment inUse
	//	so it doen't get deleted from underneath us.
	//
	while (timeout > 0 && (flags & MPR_TASK_RUNNING)) {
		int		start;
		if (stoppingCond == 0) {
			stoppingCond = new MprCond();
		}
		start = mprGetTime(0);

		ps->unlock();
		stoppingCond->waitForCond(timeout);
		ps->lock();

		timeout -= mprGetTime(0) - start;
	}

	if (stoppingCond) {
		delete stoppingCond;
		stoppingCond = 0;
	}
#endif

	if (--inUse == 0 && flags & MPR_TASK_DISPOSED) {
		delete this;
	}
	rc = (flags & MPR_TASK_RUNNING) ? -1 : 0;
	ps->unlock();
	return rc;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
