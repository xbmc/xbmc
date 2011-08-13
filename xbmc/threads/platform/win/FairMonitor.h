/**********************************************************************
*
COPYRIGHT (C) 2000 Thomas Becker

PERMISSION NOTICE:

PERMISSION TO USE, COPY, MODIFY, AND DISTRIBUTE THIS SOFTWARE FOR ANY
PURPOSE AND WITHOUT FEE IS HEREBY GRANTED, PROVIDED THAT ALL COPIES
ARE ACCOMPANIED BY THE COMPLETE MACHINE-READABLE SOURCE CODE, ALL
MODIFIED FILES CARRY PROMINENT NOTICES AS TO WHEN AND BY WHOM THEY
WERE MODIFIED, THE ABOVE COPYRIGHT NOTICE, THIS PERMISSION NOTICE AND
THE NO-WARRANTY NOTICE BELOW APPEAR IN ALL SOURCE FILES AND IN ALL
SUPPORTING DOCUMENTATION.

NO-WARRANTY NOTICE:

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF
MERCHANTABILITY ARE HEREBY DISCLAIMED.
*
**********************************************************************/

 
//
// File: FairMonitor.h
//
// Description: Declaration of class FairMonitor
//
// Revision History:
// 
// Thomas Becker Dec 2000: Created
// Thomas Becker Oct 2006: More error checking in Wait() and EndSynchronized()
//
// Jim Carroll Jul 2011: Use include <windows>

// Multiple inclusion protection
//
#ifndef __FAIR_MONITOR_INCLUDED__
#define __FAIR_MONITOR_INCLUDED__

#include<Windows.h>
#include<deque>

///////////////////////////////////////////////////////////////////////
//
// The class FairMonitor implements a Java-style monitor in Win32. The
// monitor guarantees FIFO treatment of waiting threads. The price for
// this "fairness guarantee" is the use of a significant amount of
// user-mode code in the Wait(), Notify(), and NotifyAll() functions.
// See thbecker.net/free_software_utilities/fair_monitor_for_win32/start_page.html
// for a detailed discussion of monitors and fairness under Win32.
//
// Anybody who has used monitors in Java or under pthreads should need
// no more documentation than this header file.
//
class FairMonitor
{
public:

  // Public Construction and Destruction
  // ===================================
  
  // Default constructor.
  FairMonitor();

  // NOTE: If you must derive from this class (which you should not, but I
  // cannot currently enforce it), then you must make the destructor virtual.
  ~FairMonitor();

  // Operations
  // ==========
  
  // Acquires the lock once. There is no error checking as the lock is
  // implemented as a Win32 critical section.
  void BeginSynchronized();

  // Releases the lock once. As the lock is implemented as a Win32 
  // critical section, no error checking is possible on the system
  // call that releases the lock. However, the function does check
  // whether the calling thread currently holds the lock. If not,
  // FALSE is returned and the last error is set to 
  // ERROR_INVALID_FUNCTION (for the lack of better).
  BOOL EndSynchronized();

  // Waits for a notification. The first argument is the timeout
  // in milliseconds. The alertable flag must be true if the wait
  // should complete when a user APC is queued.
  //
  // In case of success, the return value is WAIT_OBJECT_0. See
  // the documentation of the Win32 function WaitForSingleObjectEx()
  // for an explanation of the other possible return values.
  //
  // The function checks whether or not the lock has been acquired by
  // the calling thread prior to calling this function. If that is not
  // the case, WAIT_FAILED is returned and the last error is set to 
  // ERROR_INVALID_FUNCTION (for the lack of better).
  //
  // If WAIT_FAILED is returned for a reason other than the one
  // discussed above, then that indicates an irrecoverably bad
  // state of the process, usually caused by a bad API call such as 
  // entering or leaving a destroyed critical section.
  //
  // Since the lock is implemented as a Win32 critical section,
  // no error checking can be performed on the system calls that
  // aquire or release the lock. 
  DWORD Wait(
    DWORD dwMillisecondsTimeout = INFINITE,
    BOOL bAlertable = FALSE
    );

  // Notifies one of the waiting threads, if any. In case of success, 
  // the return value is TRUE. In case of failure, call GetLastError()
  // to obtain error information.
  //
  // NOTE: In most applications, this function will be called while
  // the lock is held (look at any textbook example code that uses
  // a monitor). However, it is not technically necessary for the 
  // calling thread to hold the lock when calling this function.
  //
  // The return value FALSE indicates an irrecoverrably bad state
  // of the process, usually caused by a bad API call such entering or
  // leaving a destroyed critical section.
  BOOL Notify();

  // Notifies the waiting threads, if any. In case of success, the 
  // return value is TRUE. In case of failure, call GetLastError() to 
  // obtain error information.
  //
  // NOTE: In most applications, this function will be called while
  // the lock is held (look at any textbook example code that uses
  // a monitor). However, it is not technically necessary for the 
  // calling thread to hold the lock when calling this function.
  //
  // The return value FALSE indicates an irrecoverrably bad state
  // of the process, usually caused by a bad API call such entering or
  // leaving a destroyed critical section.
  BOOL NotifyAll();

private:

  // Private helper functions
  // ========================
  
  // Creates an initially non-signalled auto-reset event and
  // pushes the handle to the event onto the wait set. The
  // return value is the event handle. In case of failure,
  // NULL is returned.
  HANDLE Push();

  // Pops the first handle off the wait set. Returns NULL if the
  // wait set was empty.
  HANDLE Pop();

  // Checks whether the calling thread is holding the lock.
  BOOL LockHeldByCallingThread();

  // The intended use of the monitor never requires any value copies.
  // Therefore, the class is non-copyable.
  FairMonitor(FairMonitor const&);
  FairMonitor& operator=(FairMonitor const&);

  // Data members
  // ============

  // STL deque that implements the wait set.
  std::deque<HANDLE> m_deqWaitSet;

  // Critical section to protect access to wait set.
  CRITICAL_SECTION m_critsecWaitSetProtection;

  // Critical section for external synchronization.
  CRITICAL_SECTION m_critsecSynchronized;

  // The monitor must keep track of how many times the lock
  // has been acquired, because Win32 does not divulge this
  // information to the client programmer.
  int m_nLockCount;

};

#endif
//
// End multiple inclusion protection
