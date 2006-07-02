/*****************************************************************
|
|   Platinum - Thread Tasks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_THREADTASK_H_
#define _PLT_THREADTASK_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDefs.h"
#include "NptThreads.h"
#include "NptTime.h"
#include "PltTaskManager.h"

/*----------------------------------------------------------------------
|   PLT_ThreadTask class
+---------------------------------------------------------------------*/
class PLT_ThreadTask : public NPT_Thread
{
public:
    friend class PLT_TaskManager;

    PLT_ThreadTask();

    // NPT_Thread methods 
    // Do not override in subclasses
    void Run();

    // must be implemented
    virtual NPT_Result Abort();
    virtual NPT_Result DoRun() { return NPT_SUCCESS; }

    virtual bool IsAborting(NPT_Timeout timeout = NPT_TIMEOUT_INFINITE) {
        return NPT_SUCCEEDED(m_Abort.WaitUntilEquals(1, timeout));
    }
    virtual bool IsDone() {
        return m_Done;
    }

protected:
    virtual ~PLT_ThreadTask();

    virtual NPT_Result  Init() { return NPT_SUCCESS; }
    
    NPT_Result StartTask(PLT_ThreadTask* task, NPT_TimeInterval* delay = NULL);

private:
    NPT_Result Start(PLT_TaskManager* task_manager, NPT_TimeInterval* delay = NULL);


private:
    NPT_Thread*         m_Thread;
    PLT_TaskManager*    m_TaskManager;
    NPT_SharedVariable  m_Abort;
    bool                m_Done;
    NPT_TimeInterval    m_Delay;
};

/*----------------------------------------------------------------------
|   PLT_ThreadTaskCallback class
+---------------------------------------------------------------------*/
class PLT_ThreadTaskCallback
{
public:
    PLT_ThreadTaskCallback(NPT_Mutex& lock) : m_Lock(lock) {}
    virtual ~PLT_ThreadTaskCallback() {};

    NPT_Result Callback();

protected:
    virtual NPT_Result DoCallback() = 0;

protected:
    NPT_Mutex& m_Lock;
};

#endif /* _PLT_THREADTASK_H_ */
