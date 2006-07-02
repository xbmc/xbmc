/*****************************************************************
|
|   Platinum - Task Manager
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_TASKMANAGER_H_
#define _PLT_TASKMANAGER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptThreads.h"
#include "NptList.h"
#include "NptTime.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_ThreadTask;

/*----------------------------------------------------------------------
|   PLT_TaskManager class
+---------------------------------------------------------------------*/
class PLT_TaskManager : public NPT_Runnable
{
public:
	PLT_TaskManager();
	virtual ~PLT_TaskManager();

    // tasks related methods
    virtual NPT_Result Start();
    virtual NPT_Result Stop();
    virtual NPT_Result StartTask(PLT_ThreadTask* task, NPT_TimeInterval* delay = NULL);
    virtual NPT_Result StopTask(PLT_ThreadTask* task);
    virtual NPT_Result StopAllTasks();
    virtual NPT_Result ScheduleStart() { return NPT_SUCCESS; }

private:
    friend class PLT_ThreadTask;
    friend class PLT_ThreadTaskCallback;

    // called by PLT_ThreadTaskCallback
    NPT_Mutex& GetCallbackMutex() { return m_CallbackLock; }

    // called by PLT_ThreadTask from dtor
    NPT_Result RemoveTask(PLT_ThreadTask* task);

    // NPT_Runnable methods
    void Run();

private:
    NPT_List<PLT_ThreadTask*> m_Tasks;
    NPT_Mutex                 m_TasksLock;
    NPT_Mutex                 m_CallbackLock;
    NPT_SharedVariable        m_Abort;
    NPT_Thread*               m_HouseKeepingThread;
};

#endif /* _PLT_TASKMANAGER_H_ */
