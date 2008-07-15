/*****************************************************************
|
|   Platinum - Control Point Tasks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
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
                                    PLT_DeviceDataReference& device);
    virtual ~PLT_CtrlPointGetDescriptionTask();

protected:
    // PLT_HttpClientSocketTask methods
    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response);

protected:
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_DeviceDataReference m_Device;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointGetSCPDTask class
+---------------------------------------------------------------------*/
class PLT_CtrlPointGetSCPDTask : public PLT_HttpClientSocketTask
{
public:
    PLT_CtrlPointGetSCPDTask(const NPT_HttpUrl&       url,
                             PLT_CtrlPoint*           ctrl_point, 
                             PLT_DeviceDataReference& m_Device);
    virtual ~PLT_CtrlPointGetSCPDTask();

protected:
    // PLT_HttpClientSocketTask methods
    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response);   

protected:
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_DeviceDataReference m_Device;
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
    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response);   

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
    PLT_CtrlPointHouseKeepingTask(PLT_CtrlPoint* ctrl_point, NPT_TimeInterval timer = NPT_TimeInterval(10, 0));

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
                                    PLT_Service*     service,
                                    void*            userdata);
    virtual ~PLT_CtrlPointSubscribeEventTask();
    
protected:
    // PLT_HttpClientSocketTask methods
    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response);

protected:
    PLT_CtrlPoint*  m_CtrlPoint;
    PLT_Service*    m_Service;
    void*           m_Userdata;
};

#endif /* _PLT_CONTROL_POINT_TASK_H_ */
