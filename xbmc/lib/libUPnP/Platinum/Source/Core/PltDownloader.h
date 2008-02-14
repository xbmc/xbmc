/*****************************************************************
|
|   Platinum - Downloader
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_DOWNLOADER_H_
#define _PLT_DOWNLOADER_H_

/*----------------------------------------------------------------------
|   Includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttpClientTask.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_Downloader;

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef PLT_HttpClientTask<PLT_Downloader> PLT_HttpDownloadTask;

typedef enum {
    PLT_DOWNLOADER_IDLE,
    PLT_DOWNLOADER_STARTED,
    PLT_DOWNLOADER_DOWNLOADING,
    PLT_DOWNLOADER_ERROR,
    PLT_DOWNLOADER_SUCCESS
} Plt_DowloaderState;

/*----------------------------------------------------------------------
|   PLT_Downloader class
+---------------------------------------------------------------------*/
class PLT_Downloader
{
public:
    PLT_Downloader(PLT_TaskManager*           task_manager, 
                   const char*                url, 
                   NPT_OutputStreamReference& output);
    virtual ~PLT_Downloader();

    NPT_Result Start();
    NPT_Result Stop();
    Plt_DowloaderState GetState() { return m_State; }

    // PLT_HttpClientTask method
    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response);


private:
    // members
    NPT_String                m_URL;
    NPT_OutputStreamReference m_Output;
    PLT_TaskManager*          m_TaskManager;
    PLT_HttpDownloadTask*     m_Task;
    Plt_DowloaderState        m_State;
};

#endif /* _PLT_DOWNLOADER_H_ */
