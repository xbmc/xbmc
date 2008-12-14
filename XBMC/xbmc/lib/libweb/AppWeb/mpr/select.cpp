///
///	@file 	select.cpp
/// @brief 	Management of socket select
/// @overview This modules provides select management for sockets 
///		and allows users to create IO handlers which will be called 
///		when IO events are detected. Windows uses a different message 
///		based mechanism. Unfortunately while this module can (and has 
///		been run on Windows) -- performance is less than stellar on 
///		windows and higher performance but not cross-platform alternatives 
///		are used for windows.
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
///
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr.h"

////////////////////////////// Forward Declarations ////////////////////////////

#if BLD_FEATURE_MULTITHREAD
static void selectProcWrapper(void *data, MprTask *tp);
#endif

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MprSelectService ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Initialize the select service
//

MprSelectService::MprSelectService()
{
	breakSock = -1;
	breakRetries = 10;
	breakPort = MPR_DEFAULT_BREAK_PORT;				// Select breakout port
	flags	= 0;
	rebuildMasks = 0;
	listGeneration = 0;
	maskGeneration = 0;
	maxDelayedFd = 0;

#if BLD_FEATURE_LOG
	log = new MprLogModule("select");
#endif

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
	cond = new MprCond();
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy the select service (users should have called stop() first)
//
 
MprSelectService::~MprSelectService()
{
	int		i;

	lock();
	for (i = 0; i < maxDelayedFd; i++) {
		if (delayedFds[i] >= 0) {
			closesocket(delayedFds[i]);
			delayedFds[i] = -1;
		}
	}

	//
	//	Moved here from close so awakening threads can still use it after 
	//	stop is called.
	//
	if (breakSock >= 0) {
		closesocket(breakSock);
		breakSock = -1;
	}

#if BLD_FEATURE_MULTITHREAD
	delete cond;
	delete mutex;
#endif

#if BLD_DEBUG
	if (list.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d select handlers unfreed",
			list.getNumItems());
	}
#endif
#if BLD_FEATURE_LOG
	delete log;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the select service. Open a select breakout socket 
//

int MprSelectService::start()
{
	memset(&breakAddress, 0, sizeof(breakAddress));

	breakAddress.sin_family = AF_INET;

	//
	//	Try first to use the loopback for the select breakout port. Otherwise
	//	use any working network addapter.
	//
	breakAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (openBreakoutPort() < 0) {

		mprError(MPR_L, MPR_LOG, 
			"Can't open a select UDP port between : %d and %d on 127.0.0.1)",
			breakPort - breakRetries, breakPort);

		breakAddress.sin_addr.s_addr = INADDR_ANY;
		if (openBreakoutPort() < 0) {

			mprError(MPR_L, MPR_LOG, 
				"Can't open a select UDP port between : %d and %d "
				"on any network address", breakPort - breakRetries, breakPort);
			mprError(MPR_L, MPR_LOG, 
				"Probable system network configuration error\n");
			return MPR_ERR_CANT_OPEN;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Try to open a socket
//
int MprSelectService::openBreakoutPort()
{
	int		rc, retries;

	lock();
	rc = 0;

	//
	//	Try to find a good port to use to break out of the select wait
	// 
	breakPort = MPR_DEFAULT_BREAK_PORT;
	for (retries = 0; retries < breakRetries; retries++) {
		breakSock = socket(AF_INET, SOCK_DGRAM, 0);
		if (breakSock < 0) {
			mprLog(MPR_WARN, 
				"Can't open port %d to use for select. Retrying.\n");
		}
#if !WIN && !VXWORKS
		fcntl(breakSock, F_SETFD, FD_CLOEXEC);
#endif
		breakAddress.sin_port = htons((short) breakPort);
		rc = bind(breakSock, (struct sockaddr *) &breakAddress, 
			sizeof(breakAddress));
		if (breakSock >= 0 && rc == 0) {
#if VXWORKS
			//
			//	VxWorks 6.0 bug workaround
			//
			breakAddress.sin_port = htons((short) breakPort);
#endif
			break;
		}
		if (breakSock >= 0) {
			closesocket(breakSock);
		}
		breakPort++;
	}

	if (breakSock < 0 || rc < 0) {
		mprLog(MPR_WARN, 
			"Can't bind any port to use for select. Tried %d-%d\n", 
			breakPort, breakPort - breakRetries);
		unlock();
		return MPR_ERR_CANT_OPEN;
	}
	unlock();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
//
//	Stop the select service. Must be idempotent.
//

int MprSelectService::stop()
{
	int		i;

#if BLD_FEATURE_LOG
	mprLog(8, log, "stop()\n");
#endif

	//
	//	Don't close sock here as other threads will need the sock when
	//	awakened. Moved to the destructor.
	//
	awaken(0);

	//
	//	Clear out delayed close fds
	//
	lock();
	for (i = 0; i < maxDelayedFd; i++) {
		if (delayedFds[i] >= 0) {
			closesocket(delayedFds[i]);
			delayedFds[i] = -1;
		}
	}
	maxDelayedFd = 0;
	unlock();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a new handler
//

int MprSelectService::insertHandler(MprSelectHandler *sp)
{
	if (list.getNumItems() == FD_SETSIZE) {
		mprLog(MPR_INFO, log, "Too many select handlers: %d\n", FD_SETSIZE);
		return MPR_ERR_TOO_MANY;
	}

	lock();

#if BLD_DEBUG
	MprSelectHandler	*np;
	np = (MprSelectHandler*) list.getFirst();
	while (np) {
		if (sp->fd == np->fd) {
			mprAssert(sp->fd != np->fd);
			break;
		}
		np = (MprSelectHandler*) list.getNext(np);
	}
#endif

	mprLog(8, log, "%d: insertHandler\n", sp->fd);
	list.insert(sp);
	listGeneration++;
	maskGeneration++;

	unlock();
	awaken();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Remove a handler
//

void MprSelectService::removeHandler(MprSelectHandler *sp)
{
	lock();
	mprLog(8, log, "%d: removeHandler\n", sp->fd);
	list.remove(sp);
	listGeneration++;
	maskGeneration++;
	rebuildMasks++;
	unlock();
	awaken();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Modify a handler
//

int MprSelectService::modifyHandler(MprSelectHandler *sp, bool wakeUp)
{
	lock();
	mprLog(8, log, "%d: modifyHandler\n", sp->fd);
	maskGeneration++;
	unlock();
	if (wakeUp) {
		awaken();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get the file handles in use by all select handlers and define the
//	FD set masks. Called by the users main event loop in the users app.
//	Returns TRUE if the masks have changed.
//

int MprSelectService::getFds(fd_set *readInterest, fd_set *writeInterest, 
	fd_set *exceptInterest, int *maxFd, int *lastGet)
{
	MprSelectHandler	*sp;
	int					mask;

#if WIN
	if (flags & MPR_ASYNC_SELECT) {
		return MPR_ERR_BAD_STATE;
	}
#endif

	if (*lastGet == maskGeneration) {
		return 0;
	}

	if (rebuildMasks) {
		FD_ZERO(readInterest);
		FD_ZERO(writeInterest);
		FD_ZERO(exceptInterest);
	}

	*lastGet = maskGeneration;
	*maxFd = 0;

	mask = 0;
	lock();
	sp = (MprSelectHandler*) list.getFirst();
	while (sp) {
		mprAssert(sp->fd >= 0);
		if (sp->proc && !(sp->flags & MPR_SELECT_DISPOSED)) {
			if (sp->flags & MPR_SELECT_FAILED) {
				sp = (MprSelectHandler*) list.getNext(sp);
				continue;
			}
			if (sp->desiredMask != 0) {
				mprLog(7, log, 
				"%d: getFds: flags %x present %x, desired %x, disabled %d\n",
				sp->fd, sp->flags, sp->presentMask, sp->desiredMask, 
				sp->disableMask);

				//
				//	Disable mask will be zero when we are already servicing an
				//	event. 
				//
				mask = sp->desiredMask & sp->disableMask;
				if (mask & MPR_READABLE) {
					mprAssert(sp->presentMask == 0);
					FD_SET((unsigned) sp->fd, readInterest);
				} else {
					FD_CLR((unsigned) sp->fd, readInterest);
				}
				if (mask & MPR_WRITEABLE) {
					FD_SET((unsigned) sp->fd, writeInterest);
				} else {
					FD_CLR((unsigned) sp->fd, writeInterest);
				}
				if (mask & MPR_EXCEPTION) {
					FD_SET((unsigned) sp->fd, exceptInterest);
				} else {
					FD_CLR((unsigned) sp->fd, exceptInterest);
				}

			} else {
				FD_CLR((unsigned) sp->fd, readInterest);
				FD_CLR((unsigned) sp->fd, writeInterest);
				FD_CLR((unsigned) sp->fd, exceptInterest);
			}

			if (mask != 0 && sp->fd >= *maxFd) {
				*maxFd = sp->fd + 1;
			}
		}
		sp = (MprSelectHandler*) list.getNext(sp);
	}

	FD_SET(((unsigned) breakSock), readInterest);
	if (breakSock >= *maxFd) {
		*maxFd = breakSock + 1;
	}

	mprLog(8, log, "getFds: maxFd %d\n", *maxFd);
	unlock();
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Repair the masks by disabling any closed / bad file descriptors.
//

void MprSelectService::repairFds()
{
	MprSelectHandler	*sp;
	fd_set				readFds, writeFds, exceptFds; 
	int					mask, maxFd, count, err;

#if WIN
	if (flags & MPR_ASYNC_SELECT) {
		return;
	}
#endif

	maxFd = 0;
	mask = 0;
	lock();

	/*
	 *	Find the bad file descriptor
	 */
	sp = (MprSelectHandler*) list.getFirst();
	while (sp) {
		mprAssert(sp->fd >= 0);

		FD_ZERO(&readFds);
		FD_ZERO(&writeFds);
		FD_ZERO(&exceptFds);

		if (sp->proc && 
				!(sp->flags & (MPR_SELECT_DISPOSED | MPR_SELECT_FAILED)) &&
				(sp->desiredMask != 0)) {
			mask = sp->desiredMask & sp->disableMask;
			if (mask & MPR_READABLE) {
				mprAssert(sp->presentMask == 0);
				FD_SET((unsigned) sp->fd, &readFds);
			} else {
				FD_CLR((unsigned) sp->fd, &readFds);
			}
			if (mask & MPR_WRITEABLE) {
				FD_SET((unsigned) sp->fd, &writeFds);
			} else {
				FD_CLR((unsigned) sp->fd, &writeFds);
			}
			if (mask & MPR_EXCEPTION) {
				FD_SET((unsigned) sp->fd, &exceptFds);
			} else {
				FD_CLR((unsigned) sp->fd, &exceptFds);
			}

			if (mask != 0) {
				if (sp->fd >= maxFd) {
					maxFd = sp->fd + 1;
				}
				count = doSelect(maxFd, &readFds, &writeFds, &exceptFds, 0);
				err = mprGetOsError();
				if (count < 0) {
					mprLog(4, 
						"repairFds: remove handle bad fd %d\n", sp->fd);
					sp->flags |= MPR_SELECT_FAILED;
					FD_ZERO(&readFds);
					FD_ZERO(&writeFds);
					FD_ZERO(&exceptFds);
					serviceIO(1, &readFds, &writeFds, &exceptFds);
				}
			}

		}
		sp = (MprSelectHandler*) list.getNext(sp);
	}

	/*
	 *	Test the break socket
	 */
	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptFds);
	FD_SET(((unsigned) breakSock), &readFds);

	if (doSelect(breakSock + 1, &readFds, &writeFds, &exceptFds, 0) < 0) {
		if (mprGetOsError() == EBADF) {
			mprLog(4, "Select break sock has bad handle, fd %d\n", breakSock);
			closesocket(breakSock);
			if (openBreakoutPort() < 0) {
				mprError(MPR_L, MPR_LOG, "Can't re-open select breakout port");
				mprError(MPR_L, MPR_LOG, "Exiting ...");
				mprGetMpr()->terminate(0);
			}
		}
	}

	/*
	 *	Force masks to be rebuilt by getFds()
	 */
	maskGeneration++;
	rebuildMasks++;

	unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprSelectService::doSelect(int maxFd, fd_set *readFds, 
		fd_set *writeFds, fd_set *exceptFds, int timeout)
{
	struct timeval		tval;
	int					fdCount;

	tval.tv_sec = timeout / 1000;
	tval.tv_usec = (timeout % 1000) * 1000;

#if WIN
	//
	//	Windows does not accept empty descriptor arrays
	//
	readFds = (readFds->fd_count == 0) ? 0 : readFds;
	writeFds = (writeFds->fd_count == 0) ? 0 : writeFds;
	exceptFds = (exceptFds->fd_count == 0) ? 0 : exceptFds;
#endif

	fdCount = select(maxFd, readFds, writeFds, exceptFds, &tval);

	//	Must not do any statements here to allow caller to get error code

	return fdCount;
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED

void MprSelectService::checkList(char *file, int line)
{
	MprSelectHandler	*sp;
	int				count;

	lock();
	mprLog(1, "Checklist at %s, %d\n", file, line);
	sp = (MprSelectHandler*) list.getFirst();
	count = 0;
	while (sp) {
		mprAssert(sp->head == &list);
		mprAssert(sp->prev->next == sp);
		if (sp->next) {
			mprAssert(sp->next->prev == sp);
		}
		sp = (MprSelectHandler*) list.getNext(sp);
		count++;
	}
	mprAssert(count == list.getNumItems());
	unlock();
}
#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Service any I/O events. Cross platform select version. Called by the 
//	users app.
//

void MprSelectService::serviceIO(int readyFds, fd_set *readFds, 
	fd_set *writeFds, fd_set *exceptFds)
{
	MprSelectHandler	*sp, *next;
	Mpr					*mpr;
	char				buf[16];
	MprSocklen			len;
	int					i, rc, mask, lastChange;

	mprLog(8, log, "serviceIO START\n");

	mpr = mprGetMpr();

#if WIN
	if (flags & MPR_ASYNC_SELECT) {
		mprAssert((flags & MPR_ASYNC_SELECT) == 0);
		return;
	}
#endif

	lock();
	//
	//	Clear out delayed close fds
	//
	for (i = 0; i < maxDelayedFd; i++) {
		closesocket(delayedFds[i]);
		FD_CLR((uint) delayedFds[i], readFds);
		mprLog(8, log, "serviceIO: delayed close for %d\n", delayedFds[i]);
	}
	maxDelayedFd = 0;

	//
	//	Service the select breakout socket first
	//
	if (FD_ISSET(breakSock, readFds)) {
		FD_CLR((uint) breakSock, readFds);
		len = sizeof(breakAddress);
		rc = recvfrom(breakSock, buf, sizeof(buf), 0, (struct sockaddr*)
			&breakAddress, (SocketLenPtr) &len);
		if (rc < 0) {
			closesocket(breakSock);
			if (openBreakoutPort() < 0) {
				mprError(MPR_L, MPR_LOG, "Can't re-open select breakout port");
			}
		}
		flags &= ~MPR_BREAK_REQUESTED;
		if (readyFds == 1) {
			mprLog(8, log, "serviceIO: solo breakout event\n");
			unlock();
			return;
		}
	}

	lastChange = listGeneration;

	//
	//	Now service all select handlers
	//
startAgain:
	sp = (MprSelectHandler*) list.getFirst();
	while (sp) {
		next = (MprSelectHandler*) list.getNext(sp);
		mask = 0;
		//
		//	Present mask is only cleared after the select handler callback has
		//	completed
		//
		mprLog(8, log, 
			"%d: ServiceIO: pres %x, desire %x, disable %d, set %d\n",
			sp->fd, sp->presentMask, sp->desiredMask, sp->disableMask, 
			FD_ISSET(sp->fd, readFds));

		mprAssert(sp->fd >= 0);
		if (sp->flags & MPR_SELECT_FAILED) {
			/*
			 *	Generate a read event so the file descriptor will be read and
			 *	then be closed
			 */
			mask |= MPR_READABLE;
			sp->desiredMask |= MPR_READABLE;
			FD_CLR((uint) sp->fd, readFds);
			mprAssert(FD_ISSET(sp->fd, readFds) == 0);
		}
		if ((sp->desiredMask & MPR_READABLE) && FD_ISSET(sp->fd, readFds)) {
			mask |= MPR_READABLE;
			FD_CLR((uint) sp->fd, readFds);
			mprAssert(FD_ISSET(sp->fd, readFds) == 0);
		}
		if ((sp->desiredMask & MPR_WRITEABLE) && FD_ISSET(sp->fd, writeFds)) {
			mask |= MPR_WRITEABLE;
			FD_CLR((uint) sp->fd, writeFds);
		}
		if ((sp->desiredMask & MPR_EXCEPTION) && FD_ISSET(sp->fd, exceptFds)) {
			mask |= MPR_EXCEPTION;
			FD_CLR((uint) sp->fd, exceptFds);
		}
		if (mask == 0) {
			sp = next;
			continue;
		}

		mprAssert(!(sp->flags & MPR_SELECT_RUNNING));
		mprAssert(sp->presentMask == 0);
		mprAssert(sp->disableMask == -1);

		if (mask & sp->desiredMask) {
			sp->presentMask = mask;
			mprAssert(sp->inUse == 1);
			sp->flags |= MPR_SELECT_RUNNING;
#if BLD_FEATURE_MULTITHREAD
			if (mpr->poolService->getMaxPoolThreads() > 0) {
				//
				//	Will be re-enabled in selectProc() after the handler has run
				//
				sp->disableEvents(0);
				mprAssert(sp->presentMask != 0);

				mprLog(8, log, 
					"%d: serviceIO: creatingTask present %x, desired %x\n", 
					sp->fd, sp->presentMask, sp->desiredMask);
				sp->serviceTask = new MprTask(selectProcWrapper, (void*)sp, 
					MPR_REQUEST_PRIORITY);
				sp->serviceTask->start();

			} else
#endif
			{
				mprLog(8, log, "%d: serviceIO: direct call\n", sp->fd);
				sp->presentMask = 0;
				sp->inUse++;

				unlock();
				(*sp->proc)(sp->handlerData, mask, 0);
				lock();

				sp->flags &= ~MPR_SELECT_RUNNING;
#if BLD_FEATURE_MULTITHREAD
				if (sp->stoppingCond) {
					sp->stoppingCond->signalCond();
				}
#endif
				if (--sp->inUse == 0 && sp->flags & MPR_SELECT_DISPOSED) {
					delete sp;
				}
			}
			if (lastChange != listGeneration) {
				//
				//	Note: sp or next may have been deleted while unlocked
				//	We have cleared the mask bits (FD_CLR) above so we 
				//	won't reprocess the event.
				//
				mprLog(9, log, "ServiceIO: rescan %d %d\n", lastChange, 
					listGeneration);
				goto startAgain;
			}
		}
		sp = next;
	}

#if BLD_FEATURE_MULTITHREAD
	if (flags & MPR_WAITING_FOR_SELECT) {
		flags &= ~MPR_WAITING_FOR_SELECT;
		cond->signalCond();
	}
#endif
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
#if WIN
//
//	Service any I/O events. Windows AsyncSelect version. Called by users app.
//

void MprSelectService::serviceIO(int sockFd, int winMask)
{
	MprSelectHandler	*sp;
	Mpr					*mpr;
	char 				buf[256];
	int					mask, j;

	mpr = mprGetMpr();

	if (!(flags & MPR_ASYNC_SELECT)) {
		mprAssert(flags & MPR_ASYNC_SELECT);
		return;
	}

	lock();
	mprLog(8, log, "serviceIO\n");

	if (winMask & FD_CLOSE) {
		// 
		//	Handle server initiated closes. See if the fd is in the delayed 
		//	close list. 
		//
		for (int i = 0; i < maxDelayedFd; i++) {
			if (delayedFds[i] == sockFd) {
				mprLog(8, log, "serviceIO: delayed close for %d\n", sockFd);
				while (recv(sockFd, buf, sizeof(buf), 0) > 0) {
					;
				}
				delayedFds[i] = -1;
				closesocket(sockFd);

				for (j = maxDelayedFd - 1; j >= 0; j--) {
					if (delayedFds[j] >= 0) {
						break;
					}
				}
				maxDelayedFd = j + 1;
				unlock();
				return;
			}
		}
		mprLog(7, log, "serviceIO: Client initiated close for %d\n", sockFd);
	}

	sp = (MprSelectHandler*) list.getFirst();
	//
	//	FUTURE -- this is slow
	//
	while (sp) {
		if (sp->fd == sockFd) {
			break;
		}
		sp = (MprSelectHandler*) list.getNext(sp);
	}
	if (sp == 0) {
		//
		//	If the server forcibly closed the socket, we may still get a read
		//	event. Just ignore it.
		//
		mprLog(9, log, "%d: serviceIO: NO HANDLER, winEvent %x\n", 
			sockFd, winMask);
		unlock();
		return;
	}

	//
	//	disableMask will be zero if we are already servicing an event
	//
	mask = sp->desiredMask & sp->disableMask;

	if (mask == 0) {
		//
		//	Already have an event scheduled so we must not schedule another yet
		//	We should have disabled events, but a message may already be in the
		//	message queue.
		//
		unlock();
		return;
	}

	mprLog(8, log, 
		"%d: serviceIO MASK winMask %x, desired %x, disable %x, flags %x\n", 
		sockFd, winMask, sp->desiredMask, sp->disableMask, sp->flags);

	//
	//	Mask values: READ==1, WRITE=2, ACCEPT=8, CONNECT=10, CLOSE=20
	//	Handle client initiated FD_CLOSE here.
	//

	if (winMask & FD_CLOSE) {
		sp->flags |= MPR_SELECT_CLIENT_CLOSED;
	}
	if (winMask & (FD_READ | FD_ACCEPT | FD_CLOSE)) {
		sp->presentMask |= MPR_READABLE;
	}
	if (winMask & (FD_WRITE | FD_CONNECT)) {
		sp->presentMask |= MPR_WRITEABLE;
	}

	if (sp->presentMask) {
		mprAssert(sp->inUse == 1);
		sp->flags |= MPR_SELECT_RUNNING;
#if BLD_FEATURE_MULTITHREAD
		if (mpr->poolService->getMaxPoolThreads() > 0) {
			sp->disableEvents(0);
			mprAssert(sp->presentMask != 0);

			mprLog(8, log, 
				"%d: serviceIO: creatingTask present %x, desired %x\n", 
				sp->fd, sp->presentMask, sp->desiredMask);
			sp->serviceTask = new MprTask(selectProcWrapper, (void*)sp, 
				MPR_REQUEST_PRIORITY);
			sp->serviceTask->start();

		} else
#endif
		{
			mprLog(8, log, "%d: serviceIO: direct call\n", sp->fd);
			sp->presentMask = 0;
			sp->inUse++;

#if BLD_FEATURE_MULTITHREAD
			if (mpr->isRunningEventsThread()) {
				sp->disableEvents(0);
			}
#endif
			mprLog(8, log, "serviceIO -- calling handler directly\n");
			unlock();
			(*sp->proc)(sp->handlerData, mask, 0);
			lock();

			sp->flags &= ~MPR_SELECT_RUNNING;
#if BLD_FEATURE_MULTITHREAD
			if (sp->stoppingCond) {
				sp->stoppingCond->signalCond();
			} else if (mpr->isRunningEventsThread()) {
				sp->enableEvents(0);
			}
#endif
			if (--sp->inUse == 0 && sp->flags & MPR_SELECT_DISPOSED) {
				delete sp;
			}
		}
	} else {
		mprLog(4, "serviceIO: Warning got event but no action %x\n", winMask);
	}
	unlock();
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Break out of the select()/message dispatch(WIN) wait
//

void MprSelectService::awaken(int wait)
{
#if BLD_FEATURE_MULTITHREAD
	Mpr		*mpr;
	int		c, count;

	mpr = mprGetMpr();
	if (mpr->poolService->getMaxPoolThreads() == 0 && 
			!mpr->isRunningEventsThread()) {
		return;
	}

#if WIN
	if (flags & MPR_ASYNC_SELECT) {
		PostMessage((HWND) getHwnd(), WM_NULL, 0, 0L);
		return;
	}
#endif

	lock();
	mprLog(8, log, "awaken: wait %d\n", wait);
	if (breakSock >= 0 && !(flags & MPR_BREAK_REQUESTED)) {
		c = 0;
		count = sendto(breakSock, (char*) &c, 1, 0, (struct sockaddr*)
			&breakAddress, sizeof(breakAddress));
		if (count == 1) {
			flags |= MPR_BREAK_REQUESTED;
		} else {
			mprLog(6, log, "Breakout send failed: %d\n", errno);
		}
	}
	if (wait) {
		//
		//	Potential bug here. If someone does a socketClose using 
		//	the primary thread that is also being used by select. That will
		//	call awaken(1) which will get here. If they are multithreaded,
		//	they will lock the select thread forever
		//
		flags |= MPR_WAITING_FOR_SELECT;
		unlock();
		cond->waitForCond(-1);
	} else {
		unlock();
	}
#else
	//
	//	If this is called by a signal handler when single threaded, the select
	//	loop will by definition be awakened.
	//
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Do a delayed close on a file/socket
//

void MprSelectService::delayedClose(int fd)
{
	lock();
	mprLog(7, log, "%d: requesting delayed close, maxDelayedFd %d\n", 
		fd, maxDelayedFd);

	if (maxDelayedFd < (int) FD_SETSIZE) {
		delayedFds[maxDelayedFd++] = fd;

	} else {
		mprLog(7, log, "%d: but doing immediate close\n", fd);
		closesocket(fd);
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Configure the breakout port
//

void MprSelectService::setPort(int n) 
{ 
	breakPort = n; 
}

////////////////////////////////////////////////////////////////////////////////
#if WIN
//
//	Turn on/off async select mode
//

void MprSelectService::setAsyncSelectMode(bool asyncSelect) 
{ 
	lock();
	flags |= MPR_ASYNC_SELECT;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return async select mode
//

bool MprSelectService::getAsyncSelectMode() 
{ 
	bool	rc;

	lock();
	rc = (flags & MPR_ASYNC_SELECT);
	unlock();
	return rc;
}

#endif
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MprSelectHandler ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a handler. Priority is only observed when MULTITHREAD
//

MprSelectHandler::MprSelectHandler(int fd, int mask, MprSelectProc proc, 
	void *data, int priority)
{
	Mpr		*mpr;

	mprAssert(fd >= 0);

	mpr = mprGetMpr();

#if CYGWIN || LINUX || MACOSX || VXWORKS || SOLARIS || FREEBSD
	if (fd >= (int) FD_SETSIZE) {
		mprError(MPR_L, MPR_LOG, 
			"File descriptor %d exceeds max select of %d", fd, FD_SETSIZE);
	}
#endif

	if (priority == 0) {
		priority = MPR_NORMAL_PRIORITY;
	}
	this->fd		= fd;
	this->priority	= priority;
	this->proc		= proc;

	flags			= 0;
	handlerData		= data;
	inUse			= 1;
	log				= mpr->selectService->getLog();
	presentMask		= 0;
	disableMask		= -1;
	selectService	= mpr->selectService;

#if BLD_FEATURE_MULTITHREAD
	stoppingCond	= 0;
	serviceTask		= 0;
#endif

	mprLog(8, log, "%d: MprSelectHandler: new handler\n", fd);

#if WIN
	if (selectService->getFlags() & MPR_ASYNC_SELECT) {
		desiredMask = 0;
		selectService->insertHandler(this);
		setInterest(mask);
	} else 
#endif
	{
		desiredMask = mask;
		selectService->insertHandler(this);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return TRUE if disposed completely.
//

bool MprSelectHandler::dispose()
{
	MprSelectService	*ss;

	mprLog(8, log, "%d: SelectHandler::dispose: inUse %d\n", fd, inUse);

	ss = selectService;
	ss->lock();

	mprAssert(inUse > 0);
	if (flags & MPR_SELECT_DISPOSED) {
		mprAssert(0);
		ss->unlock();
		return 0;
	}
	flags |= MPR_SELECT_DISPOSED;
	mprLog(8, log, "%d: dispose: inUse %d\n", fd, inUse);

	//
	//	Incase dispose is called from within a handler (ie. won't delete)
	//	we must remove from the select list immediately.
	//
	if (getList()) {
		selectService->removeHandler(this);
	}

	if (--inUse == 0) {
		delete this;
		ss->unlock();
		return 1;
	}
	ss->unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Delete a select handler. We make sure there are no outstanding tasks 
//	scheduled before we complete the deleting.
//

MprSelectHandler::~MprSelectHandler()
{
	mprLog(8, log, "%d: MprSelectHandler Destructor\n", fd);

	if (flags & MPR_SELECT_CLOSEFD) {
		mprLog(8, log, "%d: ~MprSelectHandler close on dispose\n", fd);
		close(fd);
	}

	//
	//	Just in case stop() was not called
	//
	selectService->lock();
	if (getList()) {
		mprLog(3, "MprSelectHandler destructor -- still in list\n");
		selectService->removeHandler(this);
	}
	mprAssert(inUse == 0);

#if BLD_FEATURE_MULTITHREAD
	//
	//	Disable the task callback if it is still pending. NOTE: we never 
	//	stop or dispose the task. We always allow it to run.
	// 
	if (serviceTask) {
		serviceTask->data = 0;
	}
#endif
	selectService->unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprSelectHandler::stop(int timeout)
{
	MprSelectService	*ss;
	int					rc;

	mprLog(8, log, "%d: selectHandler::stop\n", fd);
	ss = selectService;
	ss->lock();

	if (getList()) {
		ss->removeHandler(this);
	}

#if BLD_FEATURE_MULTITHREAD
	//
	//	The timer is running -- just wait for it to complete. Increment inUse
	//	so it doen't get deleted from underneath us.
	//
	inUse++;
	while (timeout > 0 && (flags & MPR_SELECT_RUNNING)) {
		int start;
		if (stoppingCond == 0) {
			stoppingCond = new MprCond();
		}
		start = mprGetTime(0);
		ss->unlock();
		stoppingCond->waitForCond(timeout);
		ss->lock();
		timeout -= mprGetTime(0) - start;
	}

	if (stoppingCond) {
		delete stoppingCond;
		stoppingCond = 0;
	}
	--inUse;
#endif
	rc = (flags & MPR_SELECT_RUNNING) ? -1 : 0;

	if (inUse == 0 && flags & MPR_SELECT_DISPOSED) {
		delete this;
	}
	ss->unlock();
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD
//
//	Call select handler on a task thread from the pool
//

static void selectProcWrapper(void *data, MprTask *tp)
{
	MprSelectHandler	*sp;

	mprGetMpr()->selectService->lock();
	if (data != 0) {
		sp = (MprSelectHandler*) data;
		sp->selectProc(tp);
	}
	tp->dispose();
	mprGetMpr()->selectService->unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprSelectHandler::selectProc(MprTask *tp)
{
	MprSelectService	*ss;

#if BLD_FEATURE_MULTITHREAD
	//
	//	The task is now past the point of no return. Clear the serviceTask
	//	pointer to prevent the handler from attempting to stop the task.
	//
	serviceTask = 0;
#endif

	if (! (flags & MPR_SELECT_DISPOSED)) {

		mprLog(8, log, "%d: selectProc BEGIN, proc %x\n", fd, proc);
		ss = selectService;

		inUse++;
		ss->unlock();

		(proc)(handlerData, presentMask, 1);
		mprLog(8, log, "%d: selectProc -- after proc\n", fd);

		ss->lock();
		mprLog(9, log, "%d: selectProc: inUse %d, flags %x\n", 
			fd, inUse, flags);
		flags &= ~MPR_SELECT_RUNNING;

#if BLD_FEATURE_MULTITHREAD
		if (stoppingCond) {
			stoppingCond->signalCond();
		}
#endif
	}

	if (--inUse == 0 && flags & MPR_SELECT_DISPOSED) {
		mprLog(8, log, "%d: selectProc delete this\n", fd);
		delete this;
	} else {
		//
		//	EnableEvents can cause a new IO event which can invoke another
		//	pool thread. This call to makeIdle allows this thread to be
		//	selected to handle the new task rather than wakening another thread
		//
		tp->getThread()->makeIdle();
		if (! (flags & MPR_SELECT_DISPOSED)) {
			//
			//	FUTURE -- OPT. Only need to enable events if disableMask == 0
			//
			enableEvents(1);
		}
	}
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Modify a select handlers interested events
//

void MprSelectHandler::setInterest(int mask)
{
	selectService->lock();

	mprLog(8, log, "%d: setInterest: new mask %x, old %x, disableMask %d\n", 
		fd, mask, desiredMask, disableMask);

	if ((desiredMask & disableMask) == (mask & disableMask)) {
		desiredMask = mask;
	} else {
		desiredMask = mask;
#if WIN
		if (selectService->getFlags() & MPR_ASYNC_SELECT) {
			setWinInterest();
		}
#endif
		selectService->modifyHandler(this, 1);
	}
	selectService->unlock();
}

////////////////////////////////////////////////////////////////////////////////
#if WIN

void MprSelectHandler::setWinInterest()
{
	int	eligible, winMask;

	winMask = 0;
	eligible = desiredMask & disableMask;
	if (eligible & MPR_READABLE) {
		winMask |= FD_READ | FD_ACCEPT | FD_CLOSE;
	}
	if (eligible & MPR_WRITEABLE) {
		winMask |= FD_WRITE | FD_CONNECT;
	}
	mprLog(8, log, 
	"%d: setWinInterest: winMask %x, desiredMask %x, disableMask %d\n", 
		fd, winMask, desiredMask, disableMask);

	WSAAsyncSelect(fd, selectService->getHwnd(), selectService->getMessage(), 
		winMask);
}

#endif
////////////////////////////////////////////////////////////////////////////////

void MprSelectHandler::disableEvents(bool wakeUp)
{
	selectService->lock();
	disableMask = 0;
#if WIN
	if (selectService->getFlags() & MPR_ASYNC_SELECT) {
		setWinInterest();
	}
#endif
	selectService->modifyHandler(this, wakeUp);
	selectService->unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprSelectHandler::enableEvents(bool wakeUp)
{
	selectService->lock();
	disableMask = -1;
	presentMask = 0;
#if WIN
	if (selectService->getFlags() & MPR_ASYNC_SELECT) {
		setWinInterest();
	}
#endif
	selectService->modifyHandler(this, 1);
	selectService->unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprSelectHandler::setProc(MprSelectProc newProc, int mask)
{
	selectService->lock();
	proc = newProc;
	setInterest(mask);
	selectService->unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprSelectHandler::setCloseOnDispose()
{
	flags |= MPR_SELECT_CLOSEFD;
}

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
