/*****************************************************************
|
|   Platinum - Task Manager
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltTaskManager.h"
#include "PltThreadTask.h"

NPT_SET_LOCAL_LOGGER("platinum.core.taskmanager")

/*----------------------------------------------------------------------
|   PLT_TaskManager::PLT_TaskManager
+---------------------------------------------------------------------*/
PLT_TaskManager::PLT_TaskManager(NPT_Cardinal max_items /* = 0 */) :
    m_Queue(max_items?new NPT_Queue<int>(max_items):NULL)
{
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::~PLT_TaskManager
+---------------------------------------------------------------------*/
PLT_TaskManager::~PLT_TaskManager()
{    
    StopAllTasks();
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::StartTask
+---------------------------------------------------------------------*/
NPT_Result 
PLT_TaskManager::StartTask(PLT_ThreadTask*   task, 
                           NPT_TimeInterval* delay /* = NULL*/,
                           bool              auto_destroy /* = true */)
{
    return task->Start(this, delay, auto_destroy);
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::StopAllTasks
+---------------------------------------------------------------------*/
NPT_Result
PLT_TaskManager::StopAllTasks()
{
    // unblock the queue if any
    if (m_Queue) {
        NPT_Queue<int>* queue = m_Queue;
        m_Queue = NULL;
        delete queue;
    }  

    // stop all tasks first but don't block
    // otherwise when RemoveTask is called by PLT_ThreadTask::Run
    // it will deadlock with m_TasksLock
    {      
        NPT_AutoLock lock(m_TasksLock);
        NPT_List<PLT_ThreadTask*>::Iterator task = m_Tasks.GetFirstItem();
        while (task) {
            (*task)->Stop(false);
            ++task;
        }
    }

    // then wait for list to become empty
    // as tasks remove themselves from the list
    while (1) {
        {
            NPT_AutoLock lock(m_TasksLock);
            if (m_Tasks.GetItemCount() == 0)
                return NPT_SUCCESS;
        }

        NPT_System::Sleep(NPT_TimeInterval(0, 10000));
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::AddTask
+---------------------------------------------------------------------*/
NPT_Result 
PLT_TaskManager::AddTask(PLT_ThreadTask* task) 
{
    if (m_Queue) {
        NPT_CHECK_SEVERE(m_Queue->Push(new int));
    }

    {
        NPT_AutoLock lock(m_TasksLock);
        return m_Tasks.Add(task);
    }
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::RemoveTask
+---------------------------------------------------------------------*/
// called by a PLT_ThreadTask::Run when done
NPT_Result
PLT_TaskManager::RemoveTask(PLT_ThreadTask* task)
{
    if (m_Queue) {
        int* val = NULL;
        if (NPT_SUCCEEDED(m_Queue->Pop(val)))
            delete val;
    }

    {
        NPT_AutoLock lock(m_TasksLock);
        m_Tasks.Remove(task);
    }
    
    // cleanup task only if auto-destroy flag was set
    if (task->m_AutoDestroy) delete task;

    return NPT_SUCCESS;
}
