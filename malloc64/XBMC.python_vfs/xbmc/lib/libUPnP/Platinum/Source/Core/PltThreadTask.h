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
#include "Neptune.h"
#include "PltTaskManager.h"

/*----------------------------------------------------------------------
|   PLT_ThreadTask class
+---------------------------------------------------------------------*/
class PLT_ThreadTask : public NPT_Runnable
{
public:
    friend class PLT_TaskManager;

    PLT_ThreadTask();

    NPT_Result Kill();

    virtual bool IsAborting(NPT_Timeout timeout = NPT_TIMEOUT_INFINITE) {
        return NPT_SUCCEEDED(m_Abort.WaitUntilEquals(1, timeout));
    }

protected:
    NPT_Result Start(PLT_TaskManager*  task_manager = NULL, 
                     NPT_TimeInterval* delay = NULL,
                     bool              auto_destroy = true);
    NPT_Result Stop(bool blocking = true);

    // overridable
    virtual void DoAbort()   {}
    virtual void DoRun()     {}
    virtual void DoInit()    {}

    // the task manager will destroy the task when finished
    // if m_AutoDestroy is set otherwise use Kill 
    virtual ~PLT_ThreadTask();
    
private:
    // NPT_Thread methods
    void Run();

protected:
    // members
    PLT_TaskManager*    m_TaskManager;

private:
    // members
    NPT_SharedVariable  m_Abort;
    NPT_Thread*         m_Thread;
    bool                m_AutoDestroy;
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
