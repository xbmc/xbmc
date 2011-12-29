/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Basic Usage Environment: for a simple, non-scripted, console application
// Implementation

#include "BasicUsageEnvironment0.hh"
#include "HandlerSet.hh"

////////// A subclass of DelayQueueEntry,
//////////     used to implement BasicTaskScheduler0::scheduleDelayedTask()

class AlarmHandler: public DelayQueueEntry {
public:
  AlarmHandler(TaskFunc* proc, void* clientData, DelayInterval timeToDelay)
    : DelayQueueEntry(timeToDelay), fProc(proc), fClientData(clientData) {
  }

private: // redefined virtual functions
  virtual void handleTimeout() {
    (*fProc)(fClientData);
    DelayQueueEntry::handleTimeout();
  }

private:
  TaskFunc* fProc;
  void* fClientData;
};


////////// BasicTaskScheduler0 //////////

BasicTaskScheduler0::BasicTaskScheduler0()
  : fLastHandledSocketNum(-1) {
  fReadHandlers = new HandlerSet;
}

BasicTaskScheduler0::~BasicTaskScheduler0() {
  delete fReadHandlers;
}

TaskToken BasicTaskScheduler0::scheduleDelayedTask(int64_t microseconds,
						 TaskFunc* proc,
						 void* clientData) {
  if (microseconds < 0) microseconds = 0;
  DelayInterval timeToDelay((long)(microseconds/1000000), (long)(microseconds%1000000));
  AlarmHandler* alarmHandler = new AlarmHandler(proc, clientData, timeToDelay);
  fDelayQueue.addEntry(alarmHandler);

  return (void*)(alarmHandler->token());
}

void BasicTaskScheduler0::unscheduleDelayedTask(TaskToken& prevTask) {
  DelayQueueEntry* alarmHandler = fDelayQueue.removeEntry((long)prevTask);
  prevTask = NULL;
  delete alarmHandler;
}

void BasicTaskScheduler0::doEventLoop(char* watchVariable) {
  // Repeatedly loop, handling readble sockets and timed events:
  for (int i=0; i < 100;++i)
  {
    //if (watchVariable != NULL && *watchVariable != 0) break;
    SingleStep();
  }
}


////////// HandlerSet (etc.) implementation //////////

HandlerDescriptor::HandlerDescriptor(HandlerDescriptor* nextHandler) {
  // Link this descriptor into a doubly-linked list:
  if (nextHandler == this) { // initialization
    fNextHandler = fPrevHandler = this;
  } else {
    fNextHandler = nextHandler;
    fPrevHandler = nextHandler->fPrevHandler;
    nextHandler->fPrevHandler = this;
    fPrevHandler->fNextHandler = this;
  }
}

HandlerDescriptor::~HandlerDescriptor() {
  // Unlink this descriptor from a doubly-linked list:
  fNextHandler->fPrevHandler = fPrevHandler;
  fPrevHandler->fNextHandler = fNextHandler;
}

HandlerSet::HandlerSet()
  : fHandlers(&fHandlers) {
  fHandlers.socketNum = -1; // shouldn't ever get looked at, but in case...
}

HandlerSet::~HandlerSet() {
  // Delete each handler descriptor:
  while (fHandlers.fNextHandler != &fHandlers) {
    delete fHandlers.fNextHandler; // changes fHandlers->fNextHandler
  }
}

void HandlerSet
::assignHandler(int socketNum,
		TaskScheduler::BackgroundHandlerProc* handlerProc,
		void* clientData) {
  // First, see if there's already a handler for this socket:
  HandlerDescriptor* handler = lookupHandler(socketNum);
  if (handler == NULL) { // No existing handler, so create a new descr:
    handler = new HandlerDescriptor(fHandlers.fNextHandler);
    handler->socketNum = socketNum;
  }

  handler->handlerProc = handlerProc;
  handler->clientData = clientData;
}

void HandlerSet::removeHandler(int socketNum) {
  HandlerDescriptor* handler = lookupHandler(socketNum);
  delete handler;
}

void HandlerSet::moveHandler(int oldSocketNum, int newSocketNum) {
  HandlerDescriptor* handler = lookupHandler(oldSocketNum);
  if (handler != NULL) {
    handler->socketNum = newSocketNum;
  }
}

HandlerDescriptor* HandlerSet::lookupHandler(int socketNum) {
  HandlerDescriptor* handler;
  HandlerIterator iter(*this);
  while ((handler = iter.next()) != NULL) {
    if (handler->socketNum == socketNum) break;
  }
  return handler;
}

HandlerIterator::HandlerIterator(HandlerSet& handlerSet)
  : fOurSet(handlerSet) {
  reset();
}

HandlerIterator::~HandlerIterator() {
}

void HandlerIterator::reset() {
  fNextPtr = fOurSet.fHandlers.fNextHandler;
}

HandlerDescriptor* HandlerIterator::next() {
  HandlerDescriptor* result = fNextPtr;
  if (result == &fOurSet.fHandlers) { // no more
    result = NULL;
  } else {
    fNextPtr = fNextPtr->fNextHandler;
  }

  return result;
}
