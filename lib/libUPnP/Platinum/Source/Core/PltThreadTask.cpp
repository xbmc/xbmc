/*****************************************************************
|
|   Platinum - Tasks
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

    // if auto-destroy, the thread may be dead, so we can't wait
    // regardless on the m_Thread...only TaskManager knows
    if (m_AutoDestroy == true && blocking) return NPT_FAILURE;

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
