/*****************************************************************
|
|   Platinum - Control Point Tasks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "PltDeviceHost.h"
#include "PltCtrlPointTask.h"
#include "PltCtrlPoint.h"
#include "PltDatagramStream.h"

/*----------------------------------------------------------------------
|    PLT_CtrlPointGetDescriptionTask::PLT_CtrlPointGetDescriptionTask
+---------------------------------------------------------------------*/
PLT_CtrlPointGetDescriptionTask::PLT_CtrlPointGetDescriptionTask(const NPT_HttpUrl&       url,
                                                                 PLT_CtrlPoint*           ctrl_point, 
                                                                 PLT_DeviceDataReference& device) :
    PLT_HttpClientSocketTask(new NPT_HttpRequest(url, "GET")), 
    m_CtrlPoint(ctrl_point), 
    m_Device(device) 
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointGetDescriptionTask::~PLT_CtrlPointGetDescriptionTask
+---------------------------------------------------------------------*/
PLT_CtrlPointGetDescriptionTask::~PLT_CtrlPointGetDescriptionTask() 
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointGetDescriptionTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_CtrlPointGetDescriptionTask::ProcessResponse(NPT_Result        res, 
                                                 NPT_HttpRequest*  request, 
                                                 NPT_SocketInfo&   info, 
                                                 NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);
    return m_CtrlPoint->ProcessGetDescriptionResponse(res, response, m_Device);
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointGetSCPDTask::PLT_CtrlPointGetSCPDTask
+---------------------------------------------------------------------*/
PLT_CtrlPointGetSCPDTask::PLT_CtrlPointGetSCPDTask(const NPT_HttpUrl&       url,
                                                   PLT_CtrlPoint*           ctrl_point, 
                                                   PLT_DeviceDataReference& device) :  
    PLT_HttpClientSocketTask(new NPT_HttpRequest(url, "GET")), 
    m_CtrlPoint(ctrl_point), 
    m_Device(device) 
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointGetSCPDTask::~PLT_CtrlPointGetSCPDTask
+---------------------------------------------------------------------*/
PLT_CtrlPointGetSCPDTask::~PLT_CtrlPointGetSCPDTask() 
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointGetSCPDTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_CtrlPointGetSCPDTask::ProcessResponse(NPT_Result        res, 
                                          NPT_HttpRequest*  request, 
                                          NPT_SocketInfo&   info, 
                                          NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(info);
    return m_CtrlPoint->ProcessGetSCPDResponse(res, request, response, m_Device);
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointInvokeActionTask::PLT_CtrlPointInvokeActionTask
+---------------------------------------------------------------------*/
PLT_CtrlPointInvokeActionTask::PLT_CtrlPointInvokeActionTask(NPT_HttpRequest*     request,
                                                             PLT_CtrlPoint*       ctrl_point, 
                                                             PLT_ActionReference& action,
                                                             void*                userdata) : 
    PLT_HttpClientSocketTask(request), 
    m_CtrlPoint(ctrl_point), 
    m_Action(action), 
    m_Userdata(userdata)
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointInvokeActionTask::~PLT_CtrlPointInvokeActionTask
+---------------------------------------------------------------------*/
PLT_CtrlPointInvokeActionTask::~PLT_CtrlPointInvokeActionTask() 
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointInvokeActionTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_CtrlPointInvokeActionTask::ProcessResponse(NPT_Result        res, 
                                               NPT_HttpRequest*  request, 
                                               NPT_SocketInfo&   info, 
                                               NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);

    return m_CtrlPoint->ProcessActionResponse(res, response, m_Action, m_Userdata);
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointHouseKeepingTask::PLT_CtrlPointHouseKeepingTask
+---------------------------------------------------------------------*/
PLT_CtrlPointHouseKeepingTask::PLT_CtrlPointHouseKeepingTask(PLT_CtrlPoint*   ctrl_point, 
                                                             NPT_TimeInterval timer) : 
    m_CtrlPoint(ctrl_point), 
    m_Timer(timer)
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointHouseKeepingTask::DoRun
+---------------------------------------------------------------------*/
void  
PLT_CtrlPointHouseKeepingTask::DoRun() 
{
    while (1) {
        if (m_CtrlPoint) {
            m_CtrlPoint->DoHouseKeeping();
        }

        if (IsAborting(m_Timer.m_Seconds*1000)) break;
    }
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointSubscribeEventTask::PLT_CtrlPointSubscribeEventTask
+---------------------------------------------------------------------*/
PLT_CtrlPointSubscribeEventTask::PLT_CtrlPointSubscribeEventTask(NPT_HttpRequest* request,
                                                                 PLT_CtrlPoint*   ctrl_point, 
                                                                 PLT_Service*     service,
                                                                 void*            userdata) : 
    PLT_HttpClientSocketTask(request), 
    m_CtrlPoint(ctrl_point), 
    m_Service(service), 
    m_Userdata(userdata) 
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointSubscribeEventTask::~PLT_CtrlPointSubscribeEventTask
+---------------------------------------------------------------------*/
PLT_CtrlPointSubscribeEventTask::~PLT_CtrlPointSubscribeEventTask() 
{
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointSubscribeEventTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_CtrlPointSubscribeEventTask::ProcessResponse(NPT_Result        res, 
                                                 NPT_HttpRequest*  request, 
                                                 NPT_SocketInfo&   info, 
                                                 NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);

    return m_CtrlPoint->ProcessSubscribeResponse(res, response, m_Service, m_Userdata);
}
