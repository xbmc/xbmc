/*****************************************************************
|
|   Platinum - Thread Tasks
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
