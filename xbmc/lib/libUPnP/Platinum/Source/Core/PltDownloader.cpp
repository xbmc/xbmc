/*****************************************************************
|
|   Platinum - Downloader
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltDownloader.h"
#include "PltTaskManager.h"

/*----------------------------------------------------------------------
|   PLT_Downloader::PLT_Downloader
+---------------------------------------------------------------------*/
PLT_Downloader::PLT_Downloader(PLT_TaskManager*           task_manager,
                               const char*                url, 
                               NPT_OutputStreamReference& output) :
    m_URL(url),
    m_Output(output),
    m_TaskManager(task_manager),
    m_Task(NULL),
    m_State(PLT_DOWNLOADER_IDLE)
{
}
    
/*----------------------------------------------------------------------
|   PLT_Downloader::~PLT_Downloader
+---------------------------------------------------------------------*/
PLT_Downloader::~PLT_Downloader()
{
    Stop();
}

/*----------------------------------------------------------------------
|   PLT_Downloader::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_Downloader::Start()
{
    Stop();

    m_Task = new PLT_HttpDownloadTask(NPT_HttpUrl(m_URL), this);
    NPT_Result res = m_TaskManager->StartTask(m_Task, NULL, false);
    if (NPT_FAILED(res)) {
        m_State = PLT_DOWNLOADER_ERROR;
        return res;
    }

    m_State = PLT_DOWNLOADER_STARTED;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Downloader::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_Downloader::Stop()
{
    if (m_Task) {
        //m_TaskManager->StopTask(m_Task);
        m_Task->Kill();
        m_Task = NULL;
    }

    m_State = PLT_DOWNLOADER_IDLE;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Downloader::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_Downloader::ProcessResponse(NPT_Result        res, 
                                NPT_HttpRequest*  request, 
                                NPT_SocketInfo&   info, 
                                NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(res)) {
        m_State = PLT_DOWNLOADER_ERROR;
        return res;
    }

    m_State = PLT_DOWNLOADER_DOWNLOADING;

    NPT_HttpEntity* entity;
    NPT_InputStreamReference body;
    if (!response || !(entity = response->GetEntity()) || NPT_FAILED(entity->GetInputStream(body))) {
        m_State = PLT_DOWNLOADER_ERROR;
        return NPT_FAILURE;
    }

    // Read body (no content length means until socket is closed)
    res = NPT_StreamToStreamCopy(*body.AsPointer(), 
        *m_Output.AsPointer(), 
        0, 
        entity->GetContentLength());

    if (NPT_FAILED(res)) {
        m_State = PLT_DOWNLOADER_ERROR;
        return res;
    }

    m_State = PLT_DOWNLOADER_SUCCESS;
    return NPT_SUCCESS;
}
