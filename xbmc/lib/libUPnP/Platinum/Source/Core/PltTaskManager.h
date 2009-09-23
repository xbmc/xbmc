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
    NPT_Queue<int>*            m_Queue;
};

#endif /* _PLT_TASKMANAGER_H_ */
