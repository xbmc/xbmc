/*****************************************************************
|
|   Platinum - Control Point Tasks
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

#ifndef _PLT_CONTROL_POINT_TASK_H_
#define _PLT_CONTROL_POINT_TASK_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttpClientTask.h"
#include "PltDatagramStream.h"
#include "PltDeviceData.h"
#include "PltCtrlPoint.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_Action;

/*----------------------------------------------------------------------
|   PLT_CtrlPointGetDescriptionTask class
+---------------------------------------------------------------------*/
class PLT_CtrlPointGetDescriptionTask : public PLT_HttpClientSocketTask
{
public:
    PLT_CtrlPointGetDescriptionTask(const NPT_HttpUrl&       url,
                                    PLT_CtrlPoint*           ctrl_point, 
                                    PLT_DeviceDataReference& root_device);
    virtual ~PLT_CtrlPointGetDescriptionTask();

protected:
    // PLT_HttpClientSocketTask methods
    NPT_Result ProcessResponse(NPT_Result                    res, 
                               NPT_HttpRequest*              request, 
                               const NPT_HttpRequestContext& context, 
                               NPT_HttpResponse*             response);

protected:
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_DeviceDataReference m_RootDevice;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointGetSCPDRequest class
+---------------------------------------------------------------------*/
class PLT_CtrlPointGetSCPDRequest : public NPT_HttpRequest
{
public:
    PLT_CtrlPointGetSCPDRequest(const char* url,
                                const char* method,
                                const char* protocol) : 
      NPT_HttpRequest(url, method, protocol) {}
    ~PLT_CtrlPointGetSCPDRequest() {}

    // members
    PLT_Service* m_Service;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointGetSCPDTask class
+---------------------------------------------------------------------*/
class PLT_CtrlPointGetSCPDTask : public PLT_HttpClientSocketTask
{
public:
    PLT_CtrlPointGetSCPDTask(PLT_CtrlPoint*           ctrl_point, 
                             PLT_DeviceDataReference& root_device);
    virtual ~PLT_CtrlPointGetSCPDTask();

protected:
    // PLT_HttpClientSocketTask methods
    NPT_Result ProcessResponse(NPT_Result                    res, 
                               NPT_HttpRequest*              request, 
                               const NPT_HttpRequestContext& context, 
                               NPT_HttpResponse*             response);   

protected:
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_DeviceDataReference m_RootDevice;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointInvokeActionTask class
+---------------------------------------------------------------------*/
class PLT_CtrlPointInvokeActionTask : public PLT_HttpClientSocketTask
{
public:
    PLT_CtrlPointInvokeActionTask(NPT_HttpRequest*     request,
                                  PLT_CtrlPoint*       ctrl_point, 
                                  PLT_ActionReference& action,
                                  void*                userdata);
    virtual ~PLT_CtrlPointInvokeActionTask();

protected:
    // PLT_HttpClientSocketTask methods
    NPT_Result ProcessResponse(NPT_Result                    res, 
                               NPT_HttpRequest*              request, 
                               const NPT_HttpRequestContext& context, 
                               NPT_HttpResponse*             response);   

protected:
    PLT_CtrlPoint*      m_CtrlPoint;
    PLT_ActionReference m_Action;
    void*               m_Userdata;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointHouseKeepingTask class
+---------------------------------------------------------------------*/
class PLT_CtrlPointHouseKeepingTask : public PLT_ThreadTask
{
public:
    PLT_CtrlPointHouseKeepingTask(PLT_CtrlPoint*   ctrl_point, 
                                  NPT_TimeInterval timer = NPT_TimeInterval(5, 0));

protected:
    ~PLT_CtrlPointHouseKeepingTask() {}

    // PLT_ThreadTask methods
    virtual void DoRun();

protected:
    PLT_CtrlPoint*   m_CtrlPoint;
    NPT_TimeInterval m_Timer;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointSubscribeEventTask class
+---------------------------------------------------------------------*/
class PLT_CtrlPointSubscribeEventTask : public PLT_HttpClientSocketTask
{
public:
    PLT_CtrlPointSubscribeEventTask(NPT_HttpRequest* request,
                                    PLT_CtrlPoint*   ctrl_point, 
									PLT_DeviceDataReference &device,
                                    PLT_Service*     service,
                                    void*            userdata = NULL);
    virtual ~PLT_CtrlPointSubscribeEventTask();
    
protected:
    // PLT_HttpClientSocketTask methods
    NPT_Result ProcessResponse(NPT_Result                    res, 
                               NPT_HttpRequest*              request, 
                               const NPT_HttpRequestContext& context, 
                               NPT_HttpResponse*             response);

protected:
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_Service*            m_Service;
	PLT_DeviceDataReference m_Device; // force to keep a reference to device owning m_Service
    void*                   m_Userdata;
};

#endif /* _PLT_CONTROL_POINT_TASK_H_ */
