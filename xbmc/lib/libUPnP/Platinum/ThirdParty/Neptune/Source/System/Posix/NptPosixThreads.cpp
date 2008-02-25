/*****************************************************************
|
|      Neptune - Threads :: Posix Implementation
|
|      (c) 2001-2002 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#if defined(__SYMBIAN32__)
#include <errno.h>
#else
#include <cerrno>
#endif

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptThreads.h"
#include "NptLogging.h"
#include "NptTime.h"
#include "NptSystem.h"

/*----------------------------------------------------------------------
|       logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.threads.posix")

/*----------------------------------------------------------------------
|       NPT_PosixMutex
+---------------------------------------------------------------------*/
class NPT_PosixMutex : public NPT_MutexInterface
{
public:
    // methods
             NPT_PosixMutex();
    virtual ~NPT_PosixMutex();

    // NPT_Mutex methods
    virtual NPT_Result Lock();
    virtual NPT_Result Unlock();

private:
    // members
    pthread_mutex_t m_Mutex;
};

/*----------------------------------------------------------------------
|       NPT_PosixMutex::NPT_PosixMutex
+---------------------------------------------------------------------*/
NPT_PosixMutex::NPT_PosixMutex()
{
    pthread_mutex_init(&m_Mutex, NULL);
}

/*----------------------------------------------------------------------
|       NPT_PosixMutex::~NPT_PosixMutex
+---------------------------------------------------------------------*/
NPT_PosixMutex::~NPT_PosixMutex()
{
    pthread_mutex_destroy(&m_Mutex);
}

/*----------------------------------------------------------------------
|       NPT_PosixMutex::Lock
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixMutex::Lock()
{
    pthread_mutex_lock(&m_Mutex);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PosixMutex::Unlock
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixMutex::Unlock()
{
    pthread_mutex_unlock(&m_Mutex);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_Mutex::NPT_Mutex
+---------------------------------------------------------------------*/
NPT_Mutex::NPT_Mutex()
{
    m_Delegate = new NPT_PosixMutex();
}

/*----------------------------------------------------------------------
|       NPT_PosixSharedVariable
+---------------------------------------------------------------------*/
class NPT_PosixSharedVariable : public NPT_SharedVariableInterface
{
public:
    // methods
               NPT_PosixSharedVariable(NPT_Integer value);
              ~NPT_PosixSharedVariable();
    NPT_Result SetValue(NPT_Integer value);
    NPT_Result GetValue(NPT_Integer& value);
    NPT_Result WaitUntilEquals(NPT_Integer value, NPT_Timeout timeout);
    NPT_Result WaitWhileEquals(NPT_Integer value, NPT_Timeout timeout);

 private:
    // members
    volatile NPT_Integer m_Value;
    pthread_mutex_t      m_Mutex;
    pthread_cond_t       m_Condition;
};

/*----------------------------------------------------------------------
|       NPT_PosixSharedVariable::NPT_PosixSharedVariable
+---------------------------------------------------------------------*/
NPT_PosixSharedVariable::NPT_PosixSharedVariable(NPT_Integer value) : 
    m_Value(value)
{
    pthread_mutex_init(&m_Mutex, NULL);
    pthread_cond_init(&m_Condition, NULL);
}

/*----------------------------------------------------------------------
|       NPT_PosixSharedVariable::~NPT_PosixSharedVariable
+---------------------------------------------------------------------*/
NPT_PosixSharedVariable::~NPT_PosixSharedVariable()
{
    pthread_cond_destroy(&m_Condition);
    pthread_mutex_destroy(&m_Mutex);
}

/*----------------------------------------------------------------------
|       NPT_PosixSharedVariable::SetValue
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixSharedVariable::SetValue(NPT_Integer value)
{
    pthread_mutex_lock(&m_Mutex);
    m_Value = value;
    pthread_cond_broadcast(&m_Condition);
    pthread_mutex_unlock(&m_Mutex);
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PosixSharedVariable::GetValue
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixSharedVariable::GetValue(NPT_Integer& value)
{
    // reading an integer should be atomic on most platforms
    value = m_Value;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PosixSharedVariable::WaitUntilEquals
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixSharedVariable::WaitUntilEquals(NPT_Integer value, NPT_Timeout timeout)
{
    NPT_Result result = NPT_SUCCESS;
    struct     timespec timed;
    struct     timeval  now;

    // get current time from system
    if (gettimeofday(&now, NULL)) {
        return NPT_FAILURE;
    }

    now.tv_usec += timeout * 1000;
    if (now.tv_usec >= 1000000) {
        now.tv_sec += now.tv_usec / 1000000;
        now.tv_usec = now.tv_usec % 1000000;
    }

    // setup timeout
    timed.tv_sec  = now.tv_sec;
    timed.tv_nsec = now.tv_usec * 1000;

    pthread_mutex_lock(&m_Mutex);
    while (value != m_Value) {
        if (timeout == NPT_TIMEOUT_INFINITE) {
            pthread_cond_wait(&m_Condition, &m_Mutex);
        } else {
            int wait_res = pthread_cond_timedwait(&m_Condition, &m_Mutex, &timed);
            if (wait_res == ETIMEDOUT) {
                result = NPT_ERROR_TIMEOUT;
                break;
            }
        }
    }
    pthread_mutex_unlock(&m_Mutex);
    
    return result;
}

/*----------------------------------------------------------------------
|       NPT_PosixSharedVariable::WaitWhileEquals
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixSharedVariable::WaitWhileEquals(NPT_Integer value, NPT_Timeout timeout)
{
    NPT_Result result = NPT_SUCCESS;
    struct     timespec timed;
    struct     timeval  now;

    // get current time from system
    if (gettimeofday(&now, NULL)) {
        return NPT_FAILURE;
    }

    now.tv_usec += timeout * 1000;
    if (now.tv_usec >= 1000000) {
        now.tv_sec += now.tv_usec / 1000000;
        now.tv_usec = now.tv_usec % 1000000;
    }

    // setup timeout
    timed.tv_sec  = now.tv_sec;
    timed.tv_nsec = now.tv_usec * 1000;

    pthread_mutex_lock(&m_Mutex);
    while (value == m_Value) {
        if (timeout == NPT_TIMEOUT_INFINITE) {
            pthread_cond_wait(&m_Condition, &m_Mutex);
        } else {
            int wait_res = pthread_cond_timedwait(&m_Condition, &m_Mutex, &timed);
            if (wait_res == ETIMEDOUT) {
                result = NPT_ERROR_TIMEOUT;
                break;
            }
        }
    }
    pthread_mutex_unlock(&m_Mutex);
    
    return result;
}

/*----------------------------------------------------------------------
|       NPT_SharedVariable::NPT_SharedVariable
+---------------------------------------------------------------------*/
NPT_SharedVariable::NPT_SharedVariable(NPT_Integer value)
{
    m_Delegate = new NPT_PosixSharedVariable(value);
}

/*----------------------------------------------------------------------
|       NPT_PosixAtomicVariable
+---------------------------------------------------------------------*/
class NPT_PosixAtomicVariable : public NPT_AtomicVariableInterface
{
 public:
    // methods
                NPT_PosixAtomicVariable(NPT_Integer value);
               ~NPT_PosixAtomicVariable();
    NPT_Integer Increment(); 
    NPT_Integer Decrement();
    NPT_Integer GetValue();
    void        SetValue(NPT_Integer value);

 private:
    // members
    volatile NPT_Integer m_Value;
    pthread_mutex_t      m_Mutex;
};

/*----------------------------------------------------------------------
|       NPT_PosixAtomicVariable::NPT_PosixAtomicVariable
+---------------------------------------------------------------------*/
NPT_PosixAtomicVariable::NPT_PosixAtomicVariable(NPT_Integer value) : 
    m_Value(value)
{
    pthread_mutex_init(&m_Mutex, NULL);
}

/*----------------------------------------------------------------------
|       NPT_PosixAtomicVariable::~NPT_PosixAtomicVariable
+---------------------------------------------------------------------*/
NPT_PosixAtomicVariable::~NPT_PosixAtomicVariable()
{
    pthread_mutex_destroy(&m_Mutex);
}

/*----------------------------------------------------------------------
|       NPT_PosixAtomicVariable::Increment
+---------------------------------------------------------------------*/
NPT_Integer
NPT_PosixAtomicVariable::Increment()
{
    NPT_Integer value;

    pthread_mutex_lock(&m_Mutex);
    value = ++m_Value;
    pthread_mutex_unlock(&m_Mutex);
    
    return value;
}

/*----------------------------------------------------------------------
|       NPT_PosixAtomicVariable::Decrement
+---------------------------------------------------------------------*/
NPT_Integer
NPT_PosixAtomicVariable::Decrement()
{
    NPT_Integer value;

    pthread_mutex_lock(&m_Mutex);
    value = --m_Value;
    pthread_mutex_unlock(&m_Mutex);
    
    return value;
}

/*----------------------------------------------------------------------
|       NPT_PosixAtomicVariable::GetValue
+---------------------------------------------------------------------*/
NPT_Integer
NPT_PosixAtomicVariable::GetValue()
{
    return m_Value;
}

/*----------------------------------------------------------------------
|       NPT_PosixAtomicVariable::SetValue
+---------------------------------------------------------------------*/
void
NPT_PosixAtomicVariable::SetValue(NPT_Integer value)
{
    pthread_mutex_lock(&m_Mutex);
    m_Value = value;
    pthread_mutex_unlock(&m_Mutex);
}

/*----------------------------------------------------------------------
|       NPT_AtomicVariable::NPT_AtomicVariable
+---------------------------------------------------------------------*/
NPT_AtomicVariable::NPT_AtomicVariable(NPT_Integer value)
{
    m_Delegate = new NPT_PosixAtomicVariable(value);
}

/*----------------------------------------------------------------------
|       NPT_PosixThread
+---------------------------------------------------------------------*/
class NPT_PosixThread : public NPT_ThreadInterface
{
 public:
    // methods
                NPT_PosixThread(NPT_Thread*   delegator,
                                NPT_Runnable& target,
                                bool          detached);
               ~NPT_PosixThread();
    NPT_Result  Start(); 
    NPT_Result  Wait(NPT_Timeout timeout = NPT_TIMEOUT_INFINITE);

 private:
    // methods
    static void* EntryPoint(void* argument);

    // NPT_Runnable methods
    void Run();

    // NPT_Interruptible methods
    NPT_Result Interrupt() { return NPT_ERROR_NOT_IMPLEMENTED; }

    // members
    NPT_Thread*        m_Delegator;
    NPT_Runnable&      m_Target;
    bool               m_Detached;
    pthread_t          m_ThreadId;
    bool               m_Joined;
    NPT_PosixMutex     m_JoinLock;
    NPT_SharedVariable m_Done;
};

/*----------------------------------------------------------------------
|       NPT_PosixThread::NPT_PosixThread
+---------------------------------------------------------------------*/
NPT_PosixThread::NPT_PosixThread(NPT_Thread*   delegator,
                                 NPT_Runnable& target,
                                 bool          detached) : 
    m_Delegator(delegator),
    m_Target(target),
    m_Detached(detached),
    m_ThreadId(0),
    m_Joined(false)
{
    NPT_LOG_FINE("NPT_PosixThread::NPT_PosixThread");
    m_Done.SetValue(0);
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::~NPT_PosixThread
+---------------------------------------------------------------------*/
NPT_PosixThread::~NPT_PosixThread()
{
    NPT_LOG_FINE_1("NPT_PosixThread::~NPT_PosixThread %d\n", m_ThreadId);

    if (!m_Detached) {
        // we're not detached, and not in the Run() method, so we need to 
        // wait until the thread is done
        Wait();
    }
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::EntryPoint
+---------------------------------------------------------------------*/
void*
NPT_PosixThread::EntryPoint(void* argument)
{
    NPT_PosixThread* thread = reinterpret_cast<NPT_PosixThread*>(argument);

    NPT_LOG_FINE("NPT_PosixThread::EntryPoint - in =======================");
    
    // set random seed
    NPT_TimeStamp now;
    NPT_System::GetCurrentTimeStamp(now);
    NPT_System::SetRandomSeed((unsigned int)(now.m_NanoSeconds + (long)thread->m_ThreadId));

    // run the thread 
    thread->Run();
    
    NPT_LOG_FINE("NPT_PosixThread::EntryPoint - out ======================");

    // we're done with the thread object
    // if we're detached, we need to delete ourselves
    if (thread->m_Detached) {
        delete thread->m_Delegator;
    } else {
        // notify we're done
        thread->m_Done.SetValue(1);
    }

    // done
    return NULL;
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::Start
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixThread::Start()
{
    NPT_LOG_FINE("NPT_PosixThread::Start - creating thread");

    pthread_attr_t *attributes = NULL;

#if defined(NPT_CONFIG_THREAD_STACK_SIZE)
    pthread_attr_t stack_size_attributes;
    pthread_attr_init(&stack_size_attributes);
    pthread_attr_setstacksize(&stack_size_attributes, NPT_CONFIG_THREAD_STACK_SIZE);
    attributes = &stack_size_attributes;
#endif

    // create the native thread
    int result = pthread_create(&m_ThreadId, attributes, EntryPoint, 
                                reinterpret_cast<void*>(this));
    NPT_LOG_FINE_2("NPT_PosixThread::Start - id = %d, res=%d", 
                   m_ThreadId, result);
    if (result) {
        // failed
        return NPT_FAILURE;
    } else {
        // detach the thread if we're not joinable
        if (m_Detached) {
            pthread_detach(m_ThreadId);
        }
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::Run
+---------------------------------------------------------------------*/
void
NPT_PosixThread::Run()
{
    m_Target.Run();
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::Wait
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixThread::Wait(NPT_Timeout timeout /* = NPT_TIMEOUT_INFINITE */)
{
    void* return_value;
    int   result;

    NPT_LOG_FINE_1("NPT_PosixThread::Wait - waiting for id %d", m_ThreadId);

    // check that we're not detached
    if (m_ThreadId == 0 || m_Detached) {
        return NPT_FAILURE;
    }

    // wait for the thread to finish
    m_JoinLock.Lock();
    if (m_Joined) {
        NPT_LOG_FINE_1("NPT_PosixThread::Wait - %d already joined", m_ThreadId);
        result = 0;
    } else {
        NPT_LOG_FINE_1("NPT_PosixThread::Wait - joining thread id %d", m_ThreadId);
        if (timeout != NPT_TIMEOUT_INFINITE) {
            result = m_Done.WaitUntilEquals(1, timeout);
            if (NPT_FAILED(result)) {
                result = -1;
                goto timedout;
            }
        }

        result = pthread_join(m_ThreadId, &return_value);
        m_Joined = true;
    }

timedout:
    m_JoinLock.Unlock();
    if (result != 0) {
        return NPT_FAILURE;
    } else {
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|       NPT_Thread::NPT_Thread
+---------------------------------------------------------------------*/
NPT_Thread::NPT_Thread(bool detached)
{
    m_Delegate = new NPT_PosixThread(this, *this, detached);
}

/*----------------------------------------------------------------------
|       NPT_Thread::NPT_Thread
+---------------------------------------------------------------------*/
NPT_Thread::NPT_Thread(NPT_Runnable& target, bool detached)
{
    m_Delegate = new NPT_PosixThread(this, target, detached);
}

