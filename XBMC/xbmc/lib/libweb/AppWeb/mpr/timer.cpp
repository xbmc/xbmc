///
///	@file 	timer.cpp
/// @brief 	Management of callback timer module
/// @overview The timer module allows callbacks to be scheduled for 
///		execution at arbitrary periods. The MprTimerService class 
///		manages the overall service and each timer is controlled 
///		by an instance of the Timer class.  MprTimerService keeps 
///		a list of timers in unsorted order. This is generally faster 
///		than keeping a sorted list if the list is mostly empty.
///		\n\n
///		Users should delete their timers. They can delete them either 
///		in their callbacks or in another thread. 
///	@remarks This module is thread-safe and uses a single mutex in 
///		the MprTimerService. We use a per-timer inUse counters for 
///		safe Timer deletion. Users call dispose() in their callbacks 
///		rather than calling delete.
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
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr.h"

////////////////////////////// Forward Declarations ////////////////////////////

static void callTimerWrapper(void *data, MprTask *task);

//////////////////////////////////// Code //////////////////////////////////////
//
//	Initialize the timer service.
//

MprTimerService::MprTimerService()
{
	MprTime	now;

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif

	mprGetTime(&now);
	lastRanTimers = now.sec * 1000 + now.usec / 1000;
	lastIdleTime = 0;

#if BLD_FEATURE_LOG
	log = new MprLogModule("timer");
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Close the timer service. Expect stop() to have been called to stop running
//	timers.
//

MprTimerService::~MprTimerService()
{
	lock();
#if BLD_FEATURE_LOG
	delete log;
#endif
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the timer service. Nothing to do as the main event loop will pump
//	timer messages
//

int MprTimerService::start()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop servicing timers
//

int MprTimerService::stop()
{
	mprAssert(timerList.getNumItems() == 0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the time in milliseconds till the next scheduled timer.
//

int MprTimerService::getIdleTime()
{
	MprTimer	*tp;
	MprTime		now;
	Mpr			*mpr;
	int			nowMsec, till;

	mpr = mprGetMpr();

	if (mpr->isExiting()) {
		return 0;
	}

	mprGetTime(&now);

	//
	//	Optimization to avoid rescanning the timer list if the calculation 
	//	made in runTimers is still current enough.
	//
	nowMsec = now.sec * 1000 + now.usec / 1000;
	if (nowMsec <= (lastRanTimers + MPR_TIMER_TOLERANCE)) {	// Within 2 msec
		return lastIdleTime;
	}

	//
	//	Lock list only to prevent insert/removal of timers. 
	//
	lastIdleTime = MAXINT;
	lock();
	for (tp = (MprTimer*) timerList.getFirst(); tp; ) {
		if ((tp->time.sec > now.sec) || 
				((tp->time.sec == now.sec) && (tp->time.usec > now.usec))) {
			till = (tp->time.sec - now.sec) * 1000 + 
				(tp->time.usec - now.usec) / 1000;
			lastIdleTime = min(till, lastIdleTime);
		} else {
			lastIdleTime = 0;
			break;
		}
		tp = (MprTimer*) timerList.getNext(tp);
	}
	unlock();
	mprLog(8, log, "getIdleTime: lastIdleTime %d\n", lastIdleTime);
	return lastIdleTime;
}

////////////////////////////////////////////////////////////////////////////////
//
//	This procedure is called by the users event loop to service any due timers.
//	Return 1 if we actually ran a timer
//

int MprTimerService::runTimers()
{
	MprTask		*task;
	MprTimer	*tp;
	MprTimer	*next;
	MprTime		now;
	Mpr			*mpr;
	int			till, minNap, ranTimer;

	mpr = mprGetMpr();
	mprGetTime(&now);
	minNap = MAXINT;

	if (mpr->isExiting()) {
		return 0;
	}

	//
	//	Loop over all timers.
	//
	ranTimer = 0;
	mprLog(8, log, "runTimers: at sec %d, usec %d\n", now.sec, now.usec);
	lock();

startAgain:
	for (tp = (MprTimer*) timerList.getFirst(); tp; tp = next) {
		next = (MprTimer*) timerList.getNext(tp);
		//
		//	If our time has not come yet, continue. Calculate minNap to speed
		//	up getIdleTime.
		//
		mprAssert(tp->inUse > 0);

		if ((tp->flags & MPR_TIMER_RUNNING) || (tp->time.sec > now.sec) || 
				((tp->time.sec == now.sec) && (tp->time.usec > now.usec))) {
			till = (tp->time.sec - now.sec) * 1000 + 
				(tp->time.usec - now.usec) / 1000;
			minNap = min(till, minNap);
			continue;
		}
		//
		//	We remove the timer from the timerList -- it will be reinserted 
		//	rescheduled by the user in the callback.
		//
		timerList.remove(tp);
		tp->flags |= MPR_TIMER_RUNNING;
		ranTimer = 1;

		//	
		//	Note: the user may call dispose() on the Timer in the callback. 
		//
		if (!(tp->flags & MPR_TIMER_TASK) || 
				mpr->poolService->getMaxPoolThreads() == 0) {
			mprLog(7, log, "runTimers: callTimer directly\n");
			
			tp->inUse++;
			unlock();
			callTimer(tp);

			lock();
			if (--tp->inUse == 0 && tp->flags & MPR_TIMER_DISPOSED) {
				mprAssert(tp->getList() == 0);
				delete tp;
			}
			//
			//	Anything may have happened while we were unlocked
			//
			goto startAgain;

		} else {
			mprLog(5, log, "runTimers: creatingTask\n");
			task = new MprTask(callTimerWrapper, (void*) tp); 
			task->start();
		}
	}
	lastRanTimers = now.sec * 1000 + now.usec / 1000;
	lastIdleTime = minNap;
	unlock();
	return ranTimer;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Callback wrapper
//

static void callTimerWrapper(void *data, MprTask *taskp)
{
	MprTimerService		*ts;
	MprTimer			*tp;

	tp = (MprTimer*) data;
	ts = tp->getMprTimerService();
	ts->callTimer(tp);

	taskp->dispose();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Call a timer. Expect the timer to be locked. We run the timer and then
//	unlock the timer.
//

void MprTimerService::callTimer(MprTimer *tp)
{
	mprAssert(tp);

	lock();
	tp->inUse++;
	unlock();

	(tp->proc)(tp->data, tp);

	lock();
	if (tp->flags & MPR_TIMER_AUTO_RESCHED) {
		tp->reschedule();
	}
	tp->flags &= ~MPR_TIMER_RUNNING;

#if BLD_FEATURE_MULTITHREAD
	if (tp->stoppingCond) {
		tp->stoppingCond->signalCond();
	}
#endif

	if (--tp->inUse == 0 && tp->flags & MPR_TIMER_DISPOSED) {
		delete tp;
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Wake select if required
//

void MprTimerService::updateSelect(MprTimer *tp)
{
	int		selectWillAwake, due;

	selectWillAwake = lastIdleTime + lastRanTimers;
	due = tp->time.sec / 1000 + tp->time.usec / 1000;
	if (selectWillAwake < 0 || due < selectWillAwake) {
		mprGetMpr()->selectService->awaken();
	}
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MprTimer ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Run a timer callback at the scheduled time
//

MprTimer::MprTimer(int msec, MprTimerProc routine, void *arg, int userFlags)
{
	time.sec = MAXINT;
	proc = routine;
	data = arg;
	period = msec;
	inUse = 1;
	flags = (userFlags & (MPR_TIMER_TASK | MPR_TIMER_AUTO_RESCHED));
	timerService = mprGetMpr()->timerService;

#if BLD_FEATURE_MULTITHREAD
	stoppingCond = 0;
#endif

	//
	//	Reschedule will add to the timer list
	//
	mprLog(6, timerService->log, "New Timer %x, time msec %d\n", this, msec);
	reschedule(msec);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return TRUE if disposed completely
//

bool MprTimer::dispose()
{
	MprTimerService	*ts;

	ts = timerService;
	ts->lock();

	mprAssert(getList() == 0);
	mprAssert(inUse > 0);

	if (flags & MPR_TIMER_DISPOSED) {
		mprAssert(0);
		ts->unlock();
		return 0;
	}
	flags |= MPR_TIMER_DISPOSED;

	if (getList()) {
		timerService->timerList.remove(this);
	}
	mprLog(5, ts->log, "%x: MprTimer dispose, inUse %d\n", this, inUse);

	if (--inUse == 0) {
		delete this;
		ts->unlock();
		return 1;
	}
	ts->unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a timer. Never called by the user in a callback!
//

MprTimer::~MprTimer()
{
	MprTimerService	*ts;

	ts = timerService;

	ts->lock();
	mprLog(5, ts->log, "%x: MprTimer delete, inUse %d\n", this, inUse);
	mprAssert(inUse == 0);
	if (getList()) {
		timerService->timerList.remove(this);
	}
	ts->unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return 1 if the task was stopped, 0 if it has completed, -1 for timeout
//

int MprTimer::stop(int timeout)
{
	MprTimerService	*ts;
	int				rc;

	ts = timerService;
	ts->lock();

	mprLog(5, ts->log, "%x: MprTimer stop, inUse %d\n", this, inUse);
	if (getList()) {
		ts->timerList.remove(this);
		ts->unlock();
		return 1;
	}


#if BLD_FEATURE_MULTITHREAD
	//
	//	The timer is running -- just wait for it to complete. Increment inUse
	//	so it doen't get deleted from underneath us.
	//
	inUse++;
	while (timeout > 0 && (flags & MPR_TIMER_RUNNING)) {
		int start;
		if (stoppingCond == 0) {
			stoppingCond = new MprCond();
		}
		start = mprGetTime(0);
		ts->unlock();
		stoppingCond->waitForCond(timeout);
		ts->lock();
		timeout -= mprGetTime(0) - start;
	}

	if (stoppingCond) {
		delete stoppingCond;
		stoppingCond = 0;
	}
	--inUse;
#endif

	rc = (flags & MPR_TIMER_RUNNING) ? -1 : 0;
	if (inUse == 0 && flags & MPR_TIMER_DISPOSED) {
		delete this;
	}
	ts->unlock();
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Reschedule the timer to run again
//

void MprTimer::reschedule()
{
	MprTimerService	*ts;

	ts = timerService;

	//
	//	Because the timer service accesses "time" directly, we need to 
	//	lock the list here to ensure we are thread-safe.
	//
	ts->lock();
	mprAssert(inUse > 0 && !(flags & MPR_TIMER_DISPOSED));
	mprGetTime(&(time));

	mprAssert(inUse > 0);

	time.sec += period / 1000;
	time.usec += (period % 1000) * 1000;
	if (time.usec >= 1000000) {
		time.usec -= 1000000;
		time.sec += 1;
	}

	if (getList() == 0) {
		ts->timerList.insert(this);
	}
	ts->updateSelect(this);
	//
	//	WARNING: may be deleted here as the timer may have already run
	//
	ts->unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Adjust the period for scheduled timer.
//

void MprTimer::reschedule(int msec)
{
	if (msec < 10) {				// Sanity check
		msec = 10;
	}
	period = msec;
	reschedule();
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
