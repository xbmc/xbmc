/*****************************************************************
|
|   Platinum - Tasks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltThreadTask.h"
#include "PltTaskManager.h" 

NPT_SET_LOCAL_LOGGER("platinum.core.threadtask")

/*----------------------------------------------------------------------
|   PLT_ThreadTask::PLT_ThreadTask
+---------------------------------------------------------------------*/
PLT_ThreadTask::PLT_ThreadTask() :
    m_TaskManager(NULL),
    m_Thread(NULL),
    m_AutoDestroy(false)
{
}

/*----------------------------------------------------------------------
|   PLT_ThreadTask::~PLT_ThreadTask
+---------------------------------------------------------------------*/
PLT_ThreadTask::~PLT_ThreadTask()
{
    if (!m_AutoDestroy) delete m_Thread;
}

/*----------------------------------------------------------------------
|   PLT_ThreadTask::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_ThreadTask::Start(PLT_TaskManager*  task_manager /* = NULL */, 
                      NPT_TimeInterval* delay /* = NULL */,
                      bool              auto_destroy /* = true */)
{
    m_Abort.SetValue(0);
    m_AutoDestroy = auto_destroy;

    if (delay) m_Delay = *delay;

    if (task_manager) {
        m_TaskManager = task_manager;
        NPT_CHECK_SEVERE(m_TaskManager->AddTask(this));
    }

    m_Thread = new NPT_Thread((NPT_Runnable&)*this, m_AutoDestroy);
    return m_Thread->Start();
}

/*----------------------------------------------------------------------
|   PLT_ThreadTask::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_ThreadTask::Stop(bool blocking /* = true */)
{
    m_Abort.SetValue(1);

    // tell thread we want to die
    DoAbort();

    // wait for thread to be dead
    return (blocking && m_Thread)?m_Thread->Wait():NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_ThreadTask::Kill
+---------------------------------------------------------------------*/
NPT_Result
PLT_ThreadTask::Kill()
{
    // A task can only be destroyed manually
    // when the m_AutoDestroy is false
    // otherwise the task manager takes
    // care of deleting it

    NPT_ASSERT(m_AutoDestroy == false);

    Stop();

    // cleanup
    delete this;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_ThreadTask::Run
+---------------------------------------------------------------------*/
void
PLT_ThreadTask::Run() 
{
//    NPT_TimeStamp now;
//    NPT_System::GetCurrentTimeStamp(now);
//    NPT_System::SetRandomSeed(now.m_NanoSeconds);


    if (m_Delay) {
        NPT_TimeStamp start, now;
        NPT_System::GetCurrentTimeStamp(start);
        do {
            NPT_System::GetCurrentTimeStamp(now);
            if (now >= start + m_Delay) break;
        } while(!IsAborting(100));
    }

    if (!IsAborting(0))  {
        DoInit();
        DoRun();
    }

    if (m_TaskManager) {
        m_TaskManager->RemoveTask(this);
    }
}

/*----------------------------------------------------------------------
|   PLT_ThreadTaskCallback::Callback
+---------------------------------------------------------------------*/
NPT_Result
PLT_ThreadTaskCallback::Callback()
{
    // acquire global lock
    NPT_AutoLock lock(m_Lock);

    // invoke callback
    return DoCallback();
}
