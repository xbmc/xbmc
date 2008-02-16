/*****************************************************************
|
|   Platinum - HTTP Server
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltHttp.h"
#include "PltVersion.h"

NPT_SET_LOCAL_LOGGER("platinum.core.http.server")

/*----------------------------------------------------------------------
|   PLT_HttpServer::PLT_HttpServer
+---------------------------------------------------------------------*/
PLT_HttpServer::PLT_HttpServer(unsigned int port,
                               NPT_Cardinal max_clients,
                               bool         reuse_address /* = false */) :
    m_TaskManager(new PLT_TaskManager(max_clients)),
    m_Port(port),
    m_ReuseAddress(reuse_address),
    m_HttpListenTask(NULL)
{
}

/*----------------------------------------------------------------------
|   PLT_HttpServer::~PLT_HttpServer
+---------------------------------------------------------------------*/
PLT_HttpServer::~PLT_HttpServer()
{ 
    delete m_TaskManager;
}

/*----------------------------------------------------------------------
|   PLT_HttpServer::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServer::Start()
{
    // if we're given a port for our http server, try it
    if (m_Port) {
        NPT_CHECK_SEVERE(SetListenPort(m_Port, m_ReuseAddress));
    } else {
        // randomly try a port for our http server
        int retries = 100;
        do {    
            int random = NPT_System::GetRandomInteger();
            int port = (unsigned short)(50000 + (random % 15000));
            if (NPT_SUCCEEDED(SetListenPort(port, m_ReuseAddress))) {
                break;
            }
        } while (--retries > 0);

        if (retries == 0) return NPT_FAILURE;
    }

    m_Port = m_Config.m_ListenPort;
    
    // start a task to listen
    m_HttpListenTask = new PLT_HttpServerListenTask(this, &m_Socket, false);
    m_TaskManager->StartTask(m_HttpListenTask, NULL, false);

    NPT_SocketInfo info;
    m_Socket.GetInfo(info);
    NPT_LOG_INFO_2("HttpServer listening on %s:%d", 
        (const char*)info.local_address.GetIpAddress().ToString(), 
        m_Port);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpServer::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServer::Stop()
{
    if (m_HttpListenTask) {
        //m_TaskManager->StopTask(m_HttpListenTask);
        m_HttpListenTask->Kill();
        m_HttpListenTask = NULL;
    }

    // stop all other pending tasks 
    m_TaskManager->StopAllTasks();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpServer::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_HttpServer::ProcessHttpRequest(NPT_HttpRequest&   request, 
                                   NPT_SocketInfo     info, 
                                   NPT_HttpResponse*& response,
                                   bool&              headers_only) 
{
    NPT_LOG_FINE("PLT_HttpServer Received Request:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);

    NPT_HttpRequestHandler* handler = FindRequestHandler(request);
    if (handler == NULL) {
        response = new NPT_HttpResponse(404, "Not Found", NPT_HTTP_PROTOCOL_1_0);
    } else {
        // create a repsones object
        response = new NPT_HttpResponse(200, "OK", NPT_HTTP_PROTOCOL_1_0);
        response->GetHeaders().SetHeader("Server", "Platinum/" PLT_PLATINUM_VERSION_STRING);

        // prepare the response
        response->SetEntity(new NPT_HttpEntity());

        // ask the handler to setup the response
        handler->SetupResponse(request, *response, info);

        // set headers_only flag
        headers_only = (request.GetMethod()==NPT_HTTP_METHOD_HEAD);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileServer::ServeFile
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileServer::ServeFile(NPT_String        filename, 
                          NPT_HttpResponse* response,
                          NPT_Integer       start,
                          NPT_Integer       end,
                          bool              request_is_head) 
{
    NPT_Size total_len;
    NPT_InputStreamReference stream;
    NPT_File file(filename);
    NPT_Result result;

    if (NPT_FAILED(result = file.Open(NPT_FILE_OPEN_MODE_READ)) || 
        NPT_FAILED(result = file.GetInputStream(stream))        ||
        NPT_FAILED(result = stream->GetSize(total_len))) {
        // file didn't open
        response->SetStatus(404, "File Not Found");
        return NPT_SUCCESS;
    } else {
        // set the content type if we can
        if (filename.EndsWith(".htm", true) ||filename.EndsWith(".html", true) ) {
            PLT_HttpHelper::SetContentType(response, "text/html");
        } else if (filename.EndsWith(".xml", true)) {
            PLT_HttpHelper::SetContentType(response, "text/xml; charset=\"utf-8\"");
        } else if (filename.EndsWith(".mp3", true)) {
            PLT_HttpHelper::SetContentType(response, "audio/mpeg");
        } else if (filename.EndsWith(".mpg", true)) {
            PLT_HttpHelper::SetContentType(response, "video/mpeg");
        } else if (filename.EndsWith(".avi", true) || filename.EndsWith(".divx", true)) {
            PLT_HttpHelper::SetContentType(response, "video/avi");
        } else if (filename.EndsWith(".wma", true)) {
            PLT_HttpHelper::SetContentType(response, "audio/x-ms-wma"); 
        } else if (filename.EndsWith(".avi", true) || filename.EndsWith(".divx", true)) {
            PLT_HttpHelper::SetContentType(response, "video/avi"); 
        } else if (filename.EndsWith(".jpg", true)) {
            PLT_HttpHelper::SetContentType(response, "image/jpeg");
        } else {
            PLT_HttpHelper::SetContentType(response, "application/octet-stream");
        }

        if (request_is_head) {            
            NPT_HttpEntity* entity = new NPT_HttpEntity();
            entity->SetContentLength(total_len);
            response->SetEntity(entity);
            return NPT_SUCCESS;
        }

        // see if it was a byte range request
        if (start != -1 || end != -1) {
            // we can only support a range from an offset to the end of the resource for now
            // due to the fact we can't limit how much to read from a stream yet
            NPT_Integer start_offset = -1, end_offset = total_len - 1, len;
            if (start == -1 && end != -1) {
                // we are asked for the last N=end bytes
                // adjust according to total length
                if (end >= (NPT_Integer)total_len) {
                    start_offset = 0;
                } else {
                    start_offset = total_len-end;
                }
            } else if (start != -1) {
                start_offset = start;
                // if the end is specified but incorrect
                // set the end_offset in order to generate a bad response
                if (end != -1 && end < start) {
                    end_offset = -1;
                }
            }

            // in case the range request was invalid or we can't seek then respond appropriately
            if (start_offset == -1 || end_offset == -1 || start_offset > end_offset || 
                NPT_FAILED(stream->Seek(start_offset))) {
                    response->SetStatus(416, "Requested range not satisfiable");
                } else {
                    len = end_offset - start_offset + 1;
                    response->SetStatus(206, "Partial Content");
                    PLT_HttpHelper::SetContentRange(response, start_offset, end_offset, total_len);

                    NPT_InputStreamReference body(stream);
                    NPT_HttpEntity* entity = new NPT_HttpEntity();
                    entity->SetInputStream(body);
                    entity->SetContentLength(len);
                    response->SetEntity(entity);
                }
        } else {
            NPT_InputStreamReference body(stream);
            NPT_HttpEntity* entity = new NPT_HttpEntity();
            entity->SetInputStream(body);
            entity->SetContentLength(total_len);
            response->SetEntity(entity);
        }
        return NPT_SUCCESS;
    }
}
