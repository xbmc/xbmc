// Win32++   Version 7.2
// Released: 5th AUgust 2011
//
//      David Nash
//      email: dnash@bigpond.net.au
//      url: https://sourceforge.net/projects/win32-framework
//
//
// Copyright (c) 2005-2011  David Nash
//
// Permission is hereby granted, free of charge, to
// any person obtaining a copy of this software and
// associated documentation files (the "Software"),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////


// The CThread class simplifies the use of threads with Win32++.
// To use threads in your Win32++ application, inherit a class from
// CThread, and override InitInstance. When your class is instanciated,
// a new thread is started, and the InitInstance function is called to
// run in the new thread.

// If your thread is used to run one or more windows, InitInstance should 
// return TRUE, causing the MessageLoop function to be called. If your
// thread doesn't require a MessageLoop, it should return FALSE. Threads
// which don't run a message loop as sometimes referred to as "worker" threads.

// Note: It is your job to end the thread before CThread ends!
//       To end a thread with a message loop, use PostQuitMessage on the thread.
//       To end a thread without a message loop, set an event, and end the thread 
//       when the event is received.

// Hint: It is never a good idea to use things like TerminateThread or ExitThread to 
//       end your thread. These represent poor programming techniques, and are likely 
//       to leak memory and resources.

// More Hints for thread programming:
// 1) Avoid using SendMessage between threads, as this will cause one thread to wait for
//    the other to respond. Use PostMessage between threads to avoid this problem.
// 2) Access to variables and resources shared between threads need to be made thread safe.
//    Having one thread modify a resouce or variable while another thread is accessing it is
//    a recipe for disaster.
// 3) Thread Local Storage (TLS) can be used to replace global variables to make them thread
//    safe. With TLS, each thread gets its own copy of the variable.
// 4) Critical Sections can be used to make shared resources thread safe.
// 5) Window messages (including user defined messages) can be posted between GUI threads to
//    communicate information between them.
// 6) Events (created by CreateEvent) can be used to comunicate information between threads 
//    (both GUI and worker threads).
// 7) Avoid using sleep to synchronise threads. Generally speaking, the various wait 
//    functions (e.g. WaitForSingleObject) will be better for this.

// About Threads:
// Each program that executes has a "process" allocated to it. A process has one or more
// threads. Threads run independantly of each other. It is the job of the operating system
// to manage the running of the threads, and do the task switching between threads as required.
// Systems with multiple CPUs will be able to run as many threads simultaneously as there are
// CPUs. 

// Threads behave like a program within a program. When the main thread starts, the application 
// runs the WinMain function and ends when WinMain ends. When another thread starts, it too
// will run the function provided to it, and end when that function ends.


#ifndef _WIN32XX_WINTHREAD_H_
#define _WIN32XX_WINTHREAD_H_


#include <process.h>


namespace Win32xx
{

	//////////////////////////////////////
	// Declaration of the CThread class
	//
	class CThread
	{
	public:
		CThread();
		CThread(LPSECURITY_ATTRIBUTES pSecurityAttributes, unsigned stack_size, unsigned initflag);
		virtual ~CThread();
		
		// Overridables
		virtual BOOL InitInstance();
		virtual int MessageLoop();

		// Operations
		HANDLE	GetThread()	const;
		int		GetThreadID() const;
		int		GetThreadPriority() const;
		DWORD	ResumeThread() const;
		BOOL	SetThreadPriority(int nPriority) const;
		DWORD	SuspendThread() const;

	private:
		CThread(const CThread&);				// Disable copy construction
		CThread& operator = (const CThread&);	// Disable assignment operator
		void CreateThread(LPSECURITY_ATTRIBUTES pSecurityAttributes, unsigned stack_size, unsigned initflag);
		static	UINT WINAPI StaticThreadCallback(LPVOID pCThread);

		HANDLE m_hThread;			// Handle of this thread
		UINT m_nThreadID;			// ID of this thread
	};
	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Win32xx
{

	///////////////////////////////////////
	// Definitions for the CThread class
	//
	inline CThread::CThread() : m_hThread(0), m_nThreadID(0)
	{
		CreateThread(0, 0, CREATE_SUSPENDED);
	}

	inline CThread::CThread(LPSECURITY_ATTRIBUTES pSecurityAttributes, unsigned stack_size, unsigned initflag)
		: m_hThread(0), m_nThreadID(0)
										
	{
		// Valid argument values:
		// pSecurityAttributes		Either a pointer to SECURITY_ATTRIBUTES or 0
		// stack_size				Either the stack size or 0
		// initflag					Either CREATE_SUSPENDED or 0
		
		CreateThread(pSecurityAttributes, stack_size, initflag);
	}

	inline CThread::~CThread()
	{	
		// A thread's state is set to signalled when the thread terminates.
		// If your thread is still running at this point, you have a bug.
		if (0 != WaitForSingleObject(m_hThread, 0))
			TRACE(_T("*** Error *** Ending CThread before ending its thread\n"));

		// Close the thread's handle
		::CloseHandle(m_hThread);
	}

	inline void CThread::CreateThread(LPSECURITY_ATTRIBUTES pSecurityAttributes, unsigned stack_size, unsigned initflag)
	{
		// NOTE:  By default, the thread is created in the default state.
		m_hThread = (HANDLE)_beginthreadex(pSecurityAttributes, stack_size, CThread::StaticThreadCallback, (LPVOID) this, initflag, &m_nThreadID);

		if (0 == m_hThread)
			throw CWinException(_T("Failed to create thread"));
	}

	inline HANDLE CThread::GetThread() const
	{
		assert(m_hThread);
		return m_hThread;
	}

	inline int CThread::GetThreadID() const 
	{
		assert(m_hThread);
		return m_nThreadID;
	}

	inline int CThread::GetThreadPriority() const
	{
		assert(m_hThread);
		return ::GetThreadPriority(m_hThread);
	}

	inline BOOL CThread::InitInstance()
	{
		// Override this function to perform tasks when the thread starts.

		// return TRUE to run a message loop, otherwise return FALSE.
		// A thread with a window must run a message loop.
		return FALSE;
	}

	inline int CThread::MessageLoop()
	{
		// Override this function if your thread needs a different message loop
		return GetApp()->MessageLoop();
	}

	inline DWORD CThread::ResumeThread() const
	{
		assert(m_hThread);
		return ::ResumeThread(m_hThread);
	}

	inline DWORD CThread::SuspendThread() const
	{
		assert(m_hThread);
		return ::SuspendThread(m_hThread); 
	}
	
	inline BOOL CThread::SetThreadPriority(int nPriority) const
	{
		assert(m_hThread);
		return ::SetThreadPriority(m_hThread, nPriority);
	}

	inline UINT WINAPI CThread::StaticThreadCallback(LPVOID pCThread)
	// When the thread starts, it runs this function.
	{
		// Get the pointer for this CMyThread object
		CThread* pThread = (CThread*)pCThread;

		if (pThread->InitInstance())
			return pThread->MessageLoop();

		return 0;
	}
	
}

#endif // #define _WIN32XX_WINTHREAD_H_

