/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __THREAD_H
#define __THREAD_H

#include <stdio.h>
#include <sys/types.h>
#include "tools.h"
#include "utils/StdString.h"
#include "libPlatform/os-dependent.h"

class cCondWait {
private:
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  bool signaled;
public:
  cCondWait(void);
  ~cCondWait();
  static void SleepMs(int TimeoutMs);
       ///< Creates a cCondWait object and uses it to sleep for TimeoutMs
       ///< milliseconds, immediately giving up the calling thread's time
       ///< slice and thus avoiding a "busy wait".
       ///< In order to avoid a possible busy wait, TimeoutMs will be automatically
       ///< limited to values >2.
  bool Wait(int TimeoutMs = 0);
       ///< Waits at most TimeoutMs milliseconds for a call to Signal(), or
       ///< forever if TimeoutMs is 0.
       ///< \return Returns true if Signal() has been called, false it the given
       ///< timeout has expired.
  void Signal(void);
       ///< Signals a caller of Wait() that the condition it is waiting for is met.
  };

class cMutex;

class cCondVar {
private:
  pthread_cond_t cond;
public:
  cCondVar(void);
  ~cCondVar();
  void Wait(cMutex &Mutex);
  bool TimedWait(cMutex &Mutex, int TimeoutMs);
  void Broadcast(void);
  };

class cMutex {
  friend class cCondVar;
private:
  pthread_mutex_t mutex;
  int locked;
public:
  cMutex(void);
  ~cMutex();
  void Lock(void);
  void Unlock(void);
  };

typedef pid_t tThreadId;

class cThread {
  friend class cThreadLock;
private:
  bool active;
  bool running;
  pthread_t childTid;
  tThreadId childThreadId;
  cMutex mutex;
  char *description;
  static tThreadId mainThreadId;
  static void *StartThread(cThread *Thread);
protected:
  void SetPriority(int Priority);
  void SetIOPriority(int Priority);
  void Lock(void) { mutex.Lock(); }
  void Unlock(void) { mutex.Unlock(); }
  virtual void Action(void) = 0;
       ///< A derived cThread class must implement the code it wants to
       ///< execute as a separate thread in this function. If this is
       ///< a loop, it must check Running() repeatedly to see whether
       ///< it's time to stop.
  bool Running(void) { return running; }
       ///< Returns false if a derived cThread object shall leave its Action()
       ///< function.
  void Cancel(int WaitSeconds = 0);
       ///< Cancels the thread by first setting 'running' to false, so that
       ///< the Action() loop can finish in an orderly fashion and then waiting
       ///< up to WaitSeconds seconds for the thread to actually end. If the
       ///< thread doesn't end by itself, it is killed.
       ///< If WaitSeconds is -1, only 'running' is set to false and Cancel()
       ///< returns immediately, without killing the thread.
public:
  cThread(const char *Description = NULL);
       ///< Creates a new thread.
       ///< If Description is present, a log file entry will be made when
       ///< the thread starts and stops. The Start() function must be called
       ///< to actually start the thread.
  virtual ~cThread();
#ifdef __WINDOWS__
  void SetDescription(const char *Description, ...);
#else
  void SetDescription(const char *Description, ...) __attribute__ ((format (printf, 2, 3)));
#endif
  bool Start(void);
       ///< Actually starts the thread.
       ///< If the thread is already running, nothing happens.
  bool Active(void);
       ///< Checks whether the thread is still alive.
  static tThreadId ThreadId(void);
  static tThreadId IsMainThread(void) { return ThreadId() == mainThreadId; }
  static void SetMainThreadId(void);
  };

// cMutexLock can be used to easily set a lock on mutex and make absolutely
// sure that it will be unlocked when the block will be left. Several locks can
// be stacked, so a function that makes many calls to another function which uses
// cMutexLock may itself use a cMutexLock to make one longer lock instead of many
// short ones.

class cMutexLock {
private:
  cMutex *mutex;
  bool locked;
public:
  cMutexLock(cMutex *Mutex = NULL);
  ~cMutexLock();
  bool Lock(cMutex *Mutex);
  };

// cThreadLock can be used to easily set a lock in a thread and make absolutely
// sure that it will be unlocked when the block will be left. Several locks can
// be stacked, so a function that makes many calls to another function which uses
// cThreadLock may itself use a cThreadLock to make one longer lock instead of many
// short ones.

class cThreadLock {
private:
  cThread *thread;
  bool locked;
public:
  cThreadLock(cThread *Thread = NULL);
  ~cThreadLock();
  bool Lock(cThread *Thread);
  };

#define LOCK_THREAD cThreadLock ThreadLock(this)

#endif //__THREAD_H
