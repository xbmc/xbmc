/*****************************************************************
|
|      Neptune - Threads :: PSP Implementation
|
|      (c) 2001-2002 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptTypes.h"
#include "NptThreads.h"
#include "NptSystem.h"
#include "NptDebug.h"

#include <stdio.h>

#include <kernel.h>
#include <psptypes.h>
#include <psperror.h>

/*----------------------------------------------------------------------
|       NPT_Mutex
+---------------------------------------------------------------------*/
class NPT_PSPMutex : public NPT_MutexInterface
{
public:
    // methods
    NPT_PSPMutex();
    ~NPT_PSPMutex();
    NPT_Result Lock();
    NPT_Result Unlock();

private:
    SceUID m_semaphore;
};

/*----------------------------------------------------------------------
|       NPT_PSPMutex::NPT_PSPMutex
+---------------------------------------------------------------------*/
NPT_PSPMutex::NPT_PSPMutex()
{
    char sema_name[256];
    sprintf(sema_name, "sema_%d", (int)NPT_System::GetSystem()->GetRandomInteger());
    m_semaphore = sceKernelCreateSema(sema_name, SCE_KERNEL_SA_THFIFO, 1, 1, NULL);
}

/*----------------------------------------------------------------------
|       NPT_PSPMutex::~NPT_PSPMutex
+---------------------------------------------------------------------*/
NPT_PSPMutex::~NPT_PSPMutex()
{
    sceKernelDeleteSema(m_semaphore);
}

/*----------------------------------------------------------------------
|       NPT_PSPMutex::Lock
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPMutex::Lock()
{
    sceKernelWaitSema(m_semaphore, 1, NULL);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPMutex::Unlock
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPMutex::Unlock()
{
    sceKernelSignalSema(m_semaphore, 1);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_Mutex::NPT_Mutex
+---------------------------------------------------------------------*/
NPT_Mutex::NPT_Mutex()
{
    m_Delegate = new NPT_PSPMutex();
}

/*----------------------------------------------------------------------
|       NPT_PSPSharedVariable
+---------------------------------------------------------------------*/
class NPT_PSPSharedVariable : public NPT_SharedVariableInterface
{
public:
    // methods
    NPT_PSPSharedVariable(NPT_Integer value);
    ~NPT_PSPSharedVariable();
    NPT_Result SetValue(NPT_Integer value);
    NPT_Result GetValue(NPT_Integer& value);
    NPT_Result WaitUntilEquals(NPT_Integer value, NPT_Timeout timeout = NPT_TIMEOUT_INFINITE);
    NPT_Result WaitWhileEquals(NPT_Integer value, NPT_Timeout timeout = NPT_TIMEOUT_INFINITE);

private:
    // members
    volatile NPT_Integer m_Value;
    NPT_Mutex            m_Lock;
};

/*----------------------------------------------------------------------
|       NPT_PSPSharedVariable::NPT_PSPSharedVariable
+---------------------------------------------------------------------*/
NPT_PSPSharedVariable::NPT_PSPSharedVariable(NPT_Integer value) : 
m_Value(value)
{
}

/*----------------------------------------------------------------------
|       NPT_PSPSharedVariable::~NPT_PSPSharedVariable
+---------------------------------------------------------------------*/
NPT_PSPSharedVariable::~NPT_PSPSharedVariable()
{
}

/*----------------------------------------------------------------------
|       NPT_PSPSharedVariable::SetValue
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPSharedVariable::SetValue(NPT_Integer value)
{
    m_Lock.Lock();
    m_Value = value;
    m_Lock.Unlock();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPSharedVariable::GetValue
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPSharedVariable::GetValue(NPT_Integer& value)
{
    // reading an integer should be atomic on most platforms
    value = m_Value;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPSharedVariable::WaitUntilEquals
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPSharedVariable::WaitUntilEquals(NPT_Integer value, NPT_Timeout timeout)
{
    NPT_Timeout sleep = 0;
    do {
        m_Lock.Lock();
        if (m_Value == value) {
            break;
        }
        m_Lock.Unlock();
        NPT_System::GetSystem()->Sleep(50000);
        sleep += 50000;
        if (timeout != NPT_TIMEOUT_INFINITE && sleep > timeout) {
            return NPT_FAILURE;
        }
    } while (1);

    m_Lock.Unlock();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPSharedVariable::WaitWhileEquals
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPSharedVariable::WaitWhileEquals(NPT_Integer value, NPT_Timeout timeout)
{
    NPT_Timeout sleep = 0;
    do {
        m_Lock.Lock();
        if (m_Value != value) {
            break;
        }
        m_Lock.Unlock();
        NPT_System::GetSystem()->Sleep(50000);
        sleep += 50000;
        if (timeout != NPT_TIMEOUT_INFINITE && sleep > timeout) {
            return NPT_FAILURE;
        }
    } while (1);

    m_Lock.Unlock();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_SharedVariable::NPT_SharedVariable
+---------------------------------------------------------------------*/
NPT_SharedVariable::NPT_SharedVariable(NPT_Integer value)
{
    m_Delegate = new NPT_PSPSharedVariable(value);
}

/*----------------------------------------------------------------------
|       NPT_PSPAtomicVariable
+---------------------------------------------------------------------*/
class NPT_PSPAtomicVariable : public NPT_AtomicVariableInterface
{
public:
    // methods
    NPT_PSPAtomicVariable(NPT_Integer value);
    ~NPT_PSPAtomicVariable();
    NPT_Integer Increment(); 
    NPT_Integer Decrement();
    NPT_Integer GetValue();
    void        SetValue(NPT_Integer value);

private:
    // members
    volatile NPT_Integer m_Value;
    NPT_Mutex            m_Mutex;
};

/*----------------------------------------------------------------------
|       NPT_PSPAtomicVariable::NPT_PSPAtomicVariable
+---------------------------------------------------------------------*/
NPT_PSPAtomicVariable::NPT_PSPAtomicVariable(NPT_Integer value) : 
m_Value(value)
{
}

/*----------------------------------------------------------------------
|       NPT_PSPAtomicVariable::~NPT_PSPAtomicVariable
+---------------------------------------------------------------------*/
NPT_PSPAtomicVariable::~NPT_PSPAtomicVariable()
{
}

/*----------------------------------------------------------------------
|       NPT_PSPAtomicVariable::Increment
+---------------------------------------------------------------------*/
NPT_Integer
NPT_PSPAtomicVariable::Increment()
{
    NPT_Integer value;

    m_Mutex.Lock();
    value = ++m_Value;
    m_Mutex.Unlock();

    return value;
}

/*----------------------------------------------------------------------
|       NPT_PSPAtomicVariable::Decrement
+---------------------------------------------------------------------*/
NPT_Integer
NPT_PSPAtomicVariable::Decrement()
{
    NPT_Integer value;

    m_Mutex.Lock();
    value = --m_Value;
    m_Mutex.Unlock();

    return value;
}

/*----------------------------------------------------------------------
|       NPT_PSPAtomicVariable::GetValue
+---------------------------------------------------------------------*/
NPT_Integer
NPT_PSPAtomicVariable::GetValue()
{
    return m_Value;
}

/*----------------------------------------------------------------------
|       NPT_PSPAtomicVariable::SetValue
+---------------------------------------------------------------------*/
void
NPT_PSPAtomicVariable::SetValue(NPT_Integer value)
{
    m_Mutex.Lock();
    m_Value = value;
    m_Mutex.Unlock();
}

/*----------------------------------------------------------------------
|       NPT_AtomicVariable::NPT_AtomicVariable
+---------------------------------------------------------------------*/
NPT_AtomicVariable::NPT_AtomicVariable(NPT_Integer value)
{
    m_Delegate = new NPT_PSPAtomicVariable(value);
}

/*----------------------------------------------------------------------
|       NPT_PSPThread
+---------------------------------------------------------------------*/
class NPT_PSPThread : public NPT_ThreadInterface
{
public:
    // methods
                NPT_PSPThread(NPT_Thread*   delegator,
                              NPT_Runnable& target,
                              bool   detached);
               ~NPT_PSPThread();
    NPT_Result  Start(); 
    NPT_Result  Wait();
    NPT_Result  Terminate();

private:
    // methods
    static int EntryPoint(SceSize argument_size, void* argument);

    // NPT_Runnable methods
    void Run();

    // members
    NPT_Thread*   m_Delegator;
    NPT_Runnable& m_Target;
    bool   m_Detached;
    SceUID        m_ThreadId;
};

/*----------------------------------------------------------------------
|       NPT_PSPThread::NPT_PSPThread
+---------------------------------------------------------------------*/
NPT_PSPThread::NPT_PSPThread(NPT_Thread*   delegator,
                                 NPT_Runnable& target,
                                 bool   detached) : 
    m_Delegator(delegator),
    m_Target(target),
    m_Detached(detached),
    m_ThreadId(0)
{
}

/*----------------------------------------------------------------------
|       NPT_PSPThread::~NPT_PSPThread
+---------------------------------------------------------------------*/
NPT_PSPThread::~NPT_PSPThread()
{
    if (!m_Detached) {
        // we're not detached, and not in the Run() method, so we need to 
        // wait until the thread is done
        Wait();
    }

    m_ThreadId = 0;
}

/*----------------------------------------------------------------------
|       NPT_PSPThread::Terminate
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPThread::Terminate()
{
    // end the thread
    sceKernelExitDeleteThread(0);

    // if we're detached, we need to delete ourselves
    if (m_Detached) {
        delete m_Delegator;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPThread::EntryPoint
+---------------------------------------------------------------------*/
int
NPT_PSPThread::EntryPoint(SceSize /* argument_size */, void* argument)
{
    NPT_PSPThread** pthread = (NPT_PSPThread**)argument;
    NPT_PSPThread*  thread  = *pthread;

    //NPT_PSPThread* thread = reinterpret_cast<NPT_PSPThread*>(*argument);

    NPT_Debug(NPT_LOG_LEVEL_1, ":: NPT_PSPThread::EntryPoint - in =======================\n");

    // run the thread 
    thread->Run();

    NPT_Debug(NPT_LOG_LEVEL_1, ":: NPT_PSPThread::EntryPoint - out ======================\n");

    // we're done with the thread object
    thread->Terminate();

    // done
    return 0;
}

/*----------------------------------------------------------------------
|       NPT_PSPThread::Start
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPThread::Start()
{
    if (m_ThreadId > 0) {
        NPT_Debug(NPT_LOG_LEVEL_1, ":: NPT_PSPThread::Start already started !\n");
        return NPT_FAILURE;
    }

    NPT_Debug(NPT_LOG_LEVEL_1, ":: NPT_PSPThread::Start - creating thread\n");
    char thread_name[32];
    sprintf(thread_name, "thread_%d", (int)NPT_System::GetSystem()->GetRandomInteger());

    // create the native thread
    m_ThreadId = (SceUID)
        sceKernelCreateThread(
        thread_name,
        EntryPoint,
        SCE_KERNEL_USER_LOWEST_PRIORITY,
        1024 * 16,
        0,
        NULL);
    if (m_ThreadId <= 0) {
        // failed
        return NPT_FAILURE;
    }

    NPT_PSPThread* thread = this;
    int ret = sceKernelStartThread(
        m_ThreadId,
        sizeof(thread),
        &thread);
    if (ret != SCE_KERNEL_ERROR_OK) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPThread::Run
+---------------------------------------------------------------------*/
void
NPT_PSPThread::Run()
{
    m_Target.Run();
}

/*----------------------------------------------------------------------
|       NPT_PSPThread::Wait
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPThread::Wait()
{
    // check that we're not detached
    if (m_ThreadId <= 0 || m_Detached) {
        return NPT_FAILURE;
    }

    // wait for the thread to finish
    NPT_Debug(NPT_LOG_LEVEL_1, ":: NPT_PSPThread::Wait - joining thread id %d\n", m_ThreadId);
    int result = sceKernelWaitThreadEnd(m_ThreadId, NULL);
    if (result < 0) {
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
    m_Delegate = new NPT_PSPThread(this, *this, detached);
}

/*----------------------------------------------------------------------
|       NPT_Thread::NPT_Thread
+---------------------------------------------------------------------*/
NPT_Thread::NPT_Thread(NPT_Runnable& target, bool detached)
{
    m_Delegate = new NPT_PSPThread(this, target, detached);
}






