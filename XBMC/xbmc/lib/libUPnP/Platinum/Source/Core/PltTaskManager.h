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
#include "Neptune.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_ThreadTask;

/*----------------------------------------------------------------------
|   PLT_TaskManager class
+---------------------------------------------------------------------*/
class PLT_TaskManager
{
    friend class PLT_ThreadTask;
    friend class PLT_ThreadTaskCallback;

public:
	PLT_TaskManager(NPT_Cardinal max_items = 0);
	virtual ~PLT_TaskManager();

    // tasks related methods
    virtual NPT_Result StartTask(PLT_ThreadTask*   task, 
                                 NPT_TimeInterval* delay = NULL,
                                 bool              auto_destroy = true);
    virtual NPT_Result StopTask(PLT_ThreadTask* task);

    // methods
    NPT_Result StopAllTasks();

private:
    // called by PLT_ThreadTaskCallback
    NPT_Mutex& GetCallbackLock() { return m_CallbackLock; }

    // called by PLT_ThreadTask
    NPT_Result AddTask(PLT_ThreadTask* task);
    NPT_Result RemoveTask(PLT_ThreadTask* task);

private:
    NPT_List<PLT_ThreadTask*>  m_Tasks;
    NPT_Mutex                  m_TasksLock;
    NPT_Mutex                  m_CallbackLock;
    NPT_Queue<NPT_Integer>*    m_Queue;
};

#endif /* _PLT_TASKMANAGER_H_ */
