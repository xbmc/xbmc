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
#include "NptHttp.h"
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltHttp.h"
#include "PltLog.h"
#include "PltVersion.h"

/*----------------------------------------------------------------------
|   PLT_HttpServer::PLT_HttpServer
+---------------------------------------------------------------------*/
PLT_HttpServer::PLT_HttpServer(PLT_HttpServerListener* listener, 
                               unsigned int            port,
                               NPT_Cardinal            max_clients) :
    m_TaskManager(new PLT_TaskManager(max_clients)),
    m_Listener(listener),
    m_Port(port),
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
    NPT_Result res;
    NPT_TcpServerSocket* socket = NULL;

    // if we're given a port for our http server, try it
    if (m_Port) {
        NPT_SocketAddress addr(NPT_IpAddress::Any, m_Port);
        socket = new NPT_TcpServerSocket();
        res = socket->Bind(addr, false);
        if (NPT_FAILED(res)) {
            delete socket;
            socket = NULL;
        }
    } else {
        // randomly try a port for our http server
        int retries = 100;
        do {    
            int random = NPT_System::GetRandomInteger();
            int port = (unsigned short)(50000 + (random % 15000));
            NPT_SocketAddress addr(NPT_IpAddress::Any, port);
            socket = new NPT_TcpServerSocket();
            res = socket->Bind(addr, false);
            if (NPT_SUCCEEDED(res)) {
                m_Port = port;
                break;
            }
            delete socket;
            socket = NULL;
        } while (--retries >= 0);
    }
    
    if (!socket) return NPT_FAILURE;

    // start a task to listen
    m_HttpListenTask = new PLT_HttpServerListenTask(m_Listener, socket);
    m_TaskManager->StartTask(m_HttpListenTask, NULL, false);

    NPT_SocketInfo info;
    socket->GetInfo(info);
    PLT_Log(PLT_LOG_LEVEL_1, "HttpServer listening on %s:%d\n", (const char*)info.local_address.GetIpAddress().ToString(), m_Port);
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
|   PLT_FileServer::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileServer::ProcessHttpRequest(NPT_HttpRequest*    request, 
                                    NPT_SocketInfo     info, 
                                    NPT_HttpResponse*& response) 
{
    NPT_COMPILER_UNUSED(info);

    NPT_String  strMehod = request->GetMethod();
    NPT_HttpUrl url = request->GetUrl();
    NPT_String  strProtocol = request->GetProtocol();

    PLT_Log(PLT_LOG_LEVEL_3, "PLT_FileHttpServer Received Request:\r\n");
    PLT_HttpHelper::ToLog(request, PLT_LOG_LEVEL_3);

    response = new NPT_HttpResponse(200, "OK");
    response->GetHeaders().SetHeader("Server", "Platinum/" PLT_PLATINUM_VERSION_STRING);

    if (strMehod.Compare("GET")) {
        response->SetStatus(500, "Internal Server Error");
        return NPT_SUCCESS;
    }

    NPT_Integer start, end;
    PLT_HttpHelper::GetRange(request, start, end);

    return PLT_FileServer::ServeFile(m_LocalPath + url.GetPath(), response, start, end);
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
        if (filename.EndsWith(".htm") ||filename.EndsWith(".html") ) {
            PLT_HttpHelper::SetContentType(response, "text/html");
        } else if (filename.EndsWith(".xml")) {
            PLT_HttpHelper::SetContentType(response, "text/xml; charset=\"utf-8\"");
        } else if (filename.EndsWith(".mp3")) {
            PLT_HttpHelper::SetContentType(response, "audio/mpeg");
        } else if (filename.EndsWith(".mpg")) {
            PLT_HttpHelper::SetContentType(response, "video/mpeg");
        } else if (filename.EndsWith(".wma")) {
            PLT_HttpHelper::SetContentType(response, "audio/x-ms-wma"); 
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
