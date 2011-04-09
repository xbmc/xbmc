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
// Copyright (c) 1996-2010, Live Networks, Inc.  All rights reserved
//	Help by Carlo Bonamico to get working for Windows
// Delay queue
// Implementation

#include "DelayQueue.hh"
#include "GroupsockHelper.hh"

static const int MILLION = 1000000;

///// Timeval /////

int Timeval::operator>=(const Timeval& arg2) const {
  return seconds() > arg2.seconds()
    || (seconds() == arg2.seconds()
	&& useconds() >= arg2.useconds());
}

void Timeval::operator+=(const DelayInterval& arg2) {
  secs() += arg2.seconds(); usecs() += arg2.useconds();
  if (useconds() >= MILLION) {
    usecs() -= MILLION;
    ++secs();
  }
}

void Timeval::operator-=(const DelayInterval& arg2) {
  secs() -= arg2.seconds(); usecs() -= arg2.useconds();
  if ((int)useconds() < 0) {
    usecs() += MILLION;
    --secs();
  }
  if ((int)seconds() < 0)
    secs() = usecs() = 0;

}

DelayInterval operator-(const Timeval& arg1, const Timeval& arg2) {
  time_base_seconds secs = arg1.seconds() - arg2.seconds();
  time_base_seconds usecs = arg1.useconds() - arg2.useconds();

  if ((int)usecs < 0) {
    usecs += MILLION;
    --secs;
  }
  if ((int)secs < 0)
    return DELAY_ZERO;
  else
    return DelayInterval(secs, usecs);
}


///// DelayInterval /////

DelayInterval operator*(short arg1, const DelayInterval& arg2) {
  time_base_seconds result_seconds = arg1*arg2.seconds();
  time_base_seconds result_useconds = arg1*arg2.useconds();

  time_base_seconds carry = result_useconds/MILLION;
  result_useconds -= carry*MILLION;
  result_seconds += carry;

  return DelayInterval(result_seconds, result_useconds);
}

#ifndef INT_MAX
#define INT_MAX	0x7FFFFFFF
#endif
const DelayInterval DELAY_ZERO(0, 0);
const DelayInterval DELAY_SECOND(1, 0);
const DelayInterval ETERNITY(INT_MAX, MILLION-1);
// used internally to make the implementation work


///// DelayQueueEntry /////

long DelayQueueEntry::tokenCounter = 0;

DelayQueueEntry::DelayQueueEntry(DelayInterval delay)
  : fDeltaTimeRemaining(delay) {
  fNext = fPrev = this;
  fToken = ++tokenCounter;
}

DelayQueueEntry::~DelayQueueEntry() {
}

void DelayQueueEntry::handleTimeout() {
  delete this;
}


///// DelayQueue /////

DelayQueue::DelayQueue()
  : DelayQueueEntry(ETERNITY) {
  fLastSyncTime = TimeNow();
}

DelayQueue::~DelayQueue() {
  while (fNext != this) removeEntry(fNext);
}

void DelayQueue::addEntry(DelayQueueEntry* newEntry) {
  synchronize();

  DelayQueueEntry* cur = head();
  while (newEntry->fDeltaTimeRemaining >= cur->fDeltaTimeRemaining) {
    newEntry->fDeltaTimeRemaining -= cur->fDeltaTimeRemaining;
    cur = cur->fNext;
  }

  cur->fDeltaTimeRemaining -= newEntry->fDeltaTimeRemaining;

  // Add "newEntry" to the queue, just before "cur":
  newEntry->fNext = cur;
  newEntry->fPrev = cur->fPrev;
  cur->fPrev = newEntry->fPrev->fNext = newEntry;
}

void DelayQueue::updateEntry(DelayQueueEntry* entry, DelayInterval newDelay) {
  if (entry == NULL) return;

  removeEntry(entry);
  entry->fDeltaTimeRemaining = newDelay;
  addEntry(entry);
}

void DelayQueue::updateEntry(long tokenToFind, DelayInterval newDelay) {
  DelayQueueEntry* entry = findEntryByToken(tokenToFind);
  updateEntry(entry, newDelay);
}

void DelayQueue::removeEntry(DelayQueueEntry* entry) {
  if (entry == NULL || entry->fNext == NULL) return;

  entry->fNext->fDeltaTimeRemaining += entry->fDeltaTimeRemaining;
  entry->fPrev->fNext = entry->fNext;
  entry->fNext->fPrev = entry->fPrev;
  entry->fNext = entry->fPrev = NULL;
  // in case we should try to remove it again
}

DelayQueueEntry* DelayQueue::removeEntry(long tokenToFind) {
  DelayQueueEntry* entry = findEntryByToken(tokenToFind);
  removeEntry(entry);
  return entry;
}

DelayInterval const& DelayQueue::timeToNextAlarm() {
  if (head()->fDeltaTimeRemaining == DELAY_ZERO) return DELAY_ZERO; // a common case

  synchronize();
  return head()->fDeltaTimeRemaining;
}

void DelayQueue::handleAlarm() {
  if (head()->fDeltaTimeRemaining != DELAY_ZERO) synchronize();

  if (head()->fDeltaTimeRemaining == DELAY_ZERO) {
    // This event is due to be handled:
    DelayQueueEntry* toRemove = head();
    removeEntry(toRemove); // do this first, in case handler accesses queue

    toRemove->handleTimeout();
  }
}

DelayQueueEntry* DelayQueue::findEntryByToken(long tokenToFind) {
  DelayQueueEntry* cur = head();
  while (cur != this) {
    if (cur->token() == tokenToFind) return cur;
    cur = cur->fNext;
  }

  return NULL;
}

void DelayQueue::synchronize() {
  // First, figure out how much time has elapsed since the last sync:
  EventTime timeNow = TimeNow();
  if (timeNow < fLastSyncTime) {
    // The system clock has apparently gone back in time; reset our sync time and return:
    fLastSyncTime  = timeNow;
    return;
  }
  DelayInterval timeSinceLastSync = timeNow - fLastSyncTime;
  fLastSyncTime = timeNow;

  // Then, adjust the delay queue for any entries whose time is up:
  DelayQueueEntry* curEntry = head();
  while (timeSinceLastSync >= curEntry->fDeltaTimeRemaining) {
    timeSinceLastSync -= curEntry->fDeltaTimeRemaining;
    curEntry->fDeltaTimeRemaining = DELAY_ZERO;
    curEntry = curEntry->fNext;
  }
  curEntry->fDeltaTimeRemaining -= timeSinceLastSync;
}


///// EventTime /////

EventTime TimeNow() {
  struct timeval tvNow;

  gettimeofday(&tvNow, NULL);

  return EventTime(tvNow.tv_sec, tvNow.tv_usec);
}

const EventTime THE_END_OF_TIME(INT_MAX);
