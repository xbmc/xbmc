/*
Copyright (c) 2010 Marcus Geelnard

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#include <iostream>
#include <list>
#include <tinythread.h>
#include <fast_mutex.h>

using namespace std;
using namespace tthread;

// HACK: Mac OS X and early MinGW do not support thread-local storage
#if defined(__APPLE__) || (defined(__MINGW32__) && (__GNUC__ < 4))
 #define NO_TLS
#endif


// Thread local storage variable
#ifndef NO_TLS
thread_local int gLocalVar;
#endif

// Mutex + global count variable
mutex gMutex;
fast_mutex gFastMutex;
int gCount;

// Condition variable
condition_variable gCond;

// Thread function: Thread ID
void ThreadIDs(void * aArg)
{
  cout << " My thread id is " << this_thread::get_id() << "." << endl;
}

#ifndef NO_TLS
// Thread function: Thread-local storage
void ThreadTLS(void * aArg)
{
  gLocalVar = 2;
  cout << " My gLocalVar is " << gLocalVar << "." << endl;
}
#endif

// Thread function: Mutex locking
void ThreadLock(void * aArg)
{
  for(int i = 0; i < 10000; ++ i)
  {
    lock_guard<mutex> lock(gMutex);
    ++ gCount;
  }
}

// Thread function: Mutex locking
void ThreadLock2(void * aArg)
{
  for(int i = 0; i < 10000; ++ i)
  {
    lock_guard<fast_mutex> lock(gFastMutex);
    ++ gCount;
  }
}

// Thread function: Condition notifier
void ThreadCondition1(void * aArg)
{
  lock_guard<mutex> lock(gMutex);
  -- gCount;
  gCond.notify_all();
}

// Thread function: Condition waiter
void ThreadCondition2(void * aArg)
{
  cout << " Wating..." << flush;
  lock_guard<mutex> lock(gMutex);
  while(gCount > 0)
  {
    cout << "." << flush;
    gCond.wait(gMutex);
  }
  cout << "." << endl;
}

// Thread function: Yield
void ThreadYield(void * aArg)
{
  // Yield...
  this_thread::yield();
}


// This is the main program (i.e. the main thread)
int main()
{
  // Test 1: Show number of CPU cores in the system
  cout << "PART I: Info" << endl;
  cout << " Number of processor cores: " << thread::hardware_concurrency() << endl;

  // Test 2: thread IDs
  cout << endl << "PART II: Thread IDs" << endl;
  {
    // Show the main thread ID
    cout << " Main thread id is " << this_thread::get_id() << "." << endl;

    // Start a bunch of child threads - only run a single thread at a time
    thread t1(ThreadIDs, 0);
    t1.join();
    thread t2(ThreadIDs, 0);
    t2.join();
    thread t3(ThreadIDs, 0);
    t3.join();
    thread t4(ThreadIDs, 0);
    t4.join();
  }

  // Test 3: thread local storage
  cout << endl << "PART III: Thread local storage" << endl;
#ifndef NO_TLS
  {
    // Clear the TLS variable (it should keep this value after all threads are
    // finished).
    gLocalVar = 1;
    cout << " Main gLocalVar is " << gLocalVar << "." << endl;

    // Start a child thread that modifies gLocalVar
    thread t1(ThreadTLS, 0);
    t1.join();

    // Check if the TLS variable has changed
    if(gLocalVar == 1)
      cout << " Main gLocalID was not changed by the child thread - OK!" << endl;
    else
      cout << " Main gLocalID was changed by the child thread - FAIL!" << endl;
  }
#else
  cout << " TLS is not supported on this platform..." << endl;
#endif

  // Test 4: mutex locking
  cout << endl << "PART IV: Mutex locking (100 threads x 10000 iterations)" << endl;
  {
    // Clear the global counter.
    gCount = 0;

    // Start a bunch of child threads
    list<thread *> threadList;
    for(int i = 0; i < 100; ++ i)
      threadList.push_back(new thread(ThreadLock, 0));

    // Wait for the threads to finish
    list<thread *>::iterator it;
    for(it = threadList.begin(); it != threadList.end(); ++ it)
    {
      thread * t = *it;
      t->join();
      delete t;
    }

    // Check the global count
    cout << " gCount = " << gCount << endl;
  }

  // Test 5: fast_mutex locking
  cout << endl << "PART V: Fast mutex locking (100 threads x 10000 iterations)" << endl;
  {
    // Clear the global counter.
    gCount = 0;

    // Start a bunch of child threads
    list<thread *> threadList;
    for(int i = 0; i < 100; ++ i)
      threadList.push_back(new thread(ThreadLock2, 0));

    // Wait for the threads to finish
    list<thread *>::iterator it;
    for(it = threadList.begin(); it != threadList.end(); ++ it)
    {
      thread * t = *it;
      t->join();
      delete t;
    }

    // Check the global count
    cout << " gCount = " << gCount << endl;
  }

  // Test 6: condition variable
  cout << endl << "PART VI: Condition variable (40 + 1 threads)" << endl;
  {
    // Set the global counter to the number of threads to run.
    gCount = 40;

    // Start the waiting thread (it will wait for gCount to reach zero).
    thread t1(ThreadCondition2, 0);

    // Start a bunch of child threads (these will decrease gCount by 1 when they
    // finish)
    list<thread *> threadList;
    for(int i = 0; i < 40; ++ i)
      threadList.push_back(new thread(ThreadCondition1, 0));

    // Wait for the waiting thread to finish
    t1.join();

    // Wait for the other threads to finish
    list<thread *>::iterator it;
    for(it = threadList.begin(); it != threadList.end(); ++ it)
    {
      thread * t = *it;
      t->join();
      delete t;
    }
  }

  // Test 7: yield
  cout << endl << "PART VII: Yield (40 + 1 threads)" << endl;
  {
    // Start a bunch of child threads
    list<thread *> threadList;
    for(int i = 0; i < 40; ++ i)
      threadList.push_back(new thread(ThreadYield, 0));

    // Yield...
    this_thread::yield();

    // Wait for the threads to finish
    list<thread *>::iterator it;
    for(it = threadList.begin(); it != threadList.end(); ++ it)
    {
      thread * t = *it;
      t->join();
      delete t;
    }
  }

  // Test 8: sleep
  cout << endl << "PART VIII: Sleep (10 x 100 ms)" << endl;
  {
    // Sleep...
    cout << " Sleeping" << flush;
    for(int i = 0; i < 10; ++ i)
    {
      this_thread::sleep_for(chrono::milliseconds(100));
      cout << "." << flush;
    }
    cout << endl;
  }
}
