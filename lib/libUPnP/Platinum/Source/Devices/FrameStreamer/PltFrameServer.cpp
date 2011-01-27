/*****************************************************************
|
|   Platinum - Frame Server
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
#include "PltFrameStream.h"
#include "PltFrameServer.h"
#include "PltUPnPHelper.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server.frame")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BOUNDARY "BOUNDARYGOAWAY"

/*----------------------------------------------------------------------
|   PLT_SocketPolicyServer
+---------------------------------------------------------------------*/
class PLT_SocketPolicyServer : public NPT_Thread
{
public:
    PLT_SocketPolicyServer(const char* policy, 
                           NPT_UInt32  port = 0,
                           const char* authorized_ports = "5900") :
        m_Policy(policy),
        m_Port(port),
        m_AuthorizedPorts(authorized_ports),
        m_Aborted(false) {}
        
    ~PLT_SocketPolicyServer() {
        Stop();
    }
        
    NPT_Result Start() {
        NPT_Result result = NPT_FAILURE;
        
        // bind
        // randomly try a port for our http server
        int retries = 100;
        do {    
            int random = NPT_System::GetRandomInteger();
            int port = (unsigned short)(50000 + (random % 15000));
                        
            result = m_Socket.Bind(
                NPT_SocketAddress(NPT_IpAddress::Any, m_Port?m_Port:port), 
                false);
                
            if (NPT_SUCCEEDED(result) || m_Port)
                break;
        } while (--retries > 0);

        if (NPT_FAILED(result) || retries == 0) return NPT_FAILURE;

        // remember that we're bound
        NPT_SocketInfo info;
        m_Socket.GetInfo(info);
        m_Port = info.local_address.GetPort();
        
        return NPT_Thread::Start();
    }
    
    NPT_Result Stop() {
        m_Aborted = true;
        m_Socket.Disconnect();
        
        return Wait();
    }
    
    void Run() {
        do {
            // wait for a connection
            NPT_Socket* client = NULL;
            NPT_LOG_FINE_1("waiting for connection on port %d...", m_Port);
            NPT_Result result = m_Socket.WaitForNewClient(client, NPT_TIMEOUT_INFINITE);
            if (NPT_FAILED(result) || client == NULL) return;
                    
            NPT_SocketInfo client_info;
            client->GetInfo(client_info);
            NPT_LOG_FINE_2("client connected (%s)",
                client_info.local_address.ToString().GetChars(),
                client_info.remote_address.ToString().GetChars());

            // get the output stream
            NPT_OutputStreamReference output;
            client->GetOutputStream(output);

            // generate policy based on our current IP
            NPT_String policy = "<cross-domain-policy>";
            policy += "<allow-access-from domain=\""+client_info.local_address.GetIpAddress().ToString()+"\" to-ports=\""+m_AuthorizedPorts+"\"/>";
            policy += "<allow-access-from domain=\""+client_info.remote_address.GetIpAddress().ToString()+"\" to-ports=\""+m_AuthorizedPorts+"\"/>";
            policy += "</cross-domain-policy>";

            NPT_MemoryStream* mem_input = new NPT_MemoryStream();
            mem_input->Write(policy.GetChars(), policy.GetLength());
            NPT_InputStreamReference input(mem_input);
            
            NPT_StreamToStreamCopy(*input, *output);
            
            
            delete client;
        } while (!m_Aborted);
    }
    
    NPT_TcpServerSocket m_Socket;
    NPT_String          m_Policy;
    NPT_UInt32          m_Port;
    NPT_String          m_AuthorizedPorts;
    bool                m_Aborted;
};

/*----------------------------------------------------------------------
|   PLT_HttpStreamRequestHandler::SetupResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpStreamRequestHandler::SetupResponse(NPT_HttpRequest&              request, 
                                            const NPT_HttpRequestContext& context,
                                            NPT_HttpResponse&             response)
{
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity == NULL) return NPT_ERROR_INVALID_STATE;
    
    NPT_LOG_FINE("Received Request:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);

    if (request.GetMethod().Compare("GET")) {
        response.SetStatus(500, "Internal Server Error");
        return NPT_SUCCESS;
    }
    
    if (m_StreamValidator && !m_StreamValidator->OnNewRequestAccept(context)) {
        response.SetStatus(404, "Not Found");
        return NPT_SUCCESS;
    }

    response.SetProtocol(NPT_HTTP_PROTOCOL_1_0);
    response.GetHeaders().SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");
    response.GetHeaders().SetHeader("Cache-Control", "no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0");
    response.GetHeaders().SetHeader("Pragma", "no-cache");
    response.GetHeaders().SetHeader("Expires", "Tue, 4 Jan 2000 02:43:05 GMT");

    entity->SetContentType("multipart/x-mixed-replace;boundary=" BOUNDARY);

    NPT_InputStreamReference body(new PLT_InputFrameStream(m_FrameBuffer, BOUNDARY));
    entity->SetInputStream(body, false);

    return response.SetEntity(entity);
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::PLT_FrameServer
+---------------------------------------------------------------------*/
PLT_FrameServer::PLT_FrameServer(PLT_FrameBuffer&     frame_buffer,
                                 const char*          friendly_name, 
                                 const char*          www_root,
                                 const char*          resource_name,
                                 NPT_UInt16           port,
                                 PLT_StreamValidator* stream_validator) :	
    PLT_HttpServer(port, true),
    m_FriendlyName(friendly_name),
    m_RootFilePath(www_root),
    m_PolicyServer(NULL),
    m_StreamValidator(stream_validator)
{
    /* set up the server root path */
    m_RootFilePath.TrimRight("/\\");

    NPT_String resource(resource_name);
    resource.Trim("/\\");
    AddRequestHandler(
        new PLT_HttpStreamRequestHandler(frame_buffer, stream_validator), 
        "/" + resource, 
        false);
        
    AddRequestHandler(
        new NPT_HttpFileRequestHandler("/content", m_RootFilePath, true), 
        "/content", 
        true);
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::~PLT_FrameServer
+---------------------------------------------------------------------*/
PLT_FrameServer::~PLT_FrameServer()
{
    delete m_PolicyServer;
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_FrameServer::Start()
{
    // start main server so we can get the listening port
    NPT_CHECK_SEVERE(PLT_HttpServer::Start());
    
    // start the xml socket policy server for flash
    m_PolicyServer = new PLT_SocketPolicyServer(
        "", 
        8989, 
        "5900,"+NPT_String::FromInteger(GetPort()));
    NPT_CHECK_SEVERE(m_PolicyServer->Start());

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FrameServer::ProcessHttpRequest(NPT_HttpRequest&              request, 
                                    const NPT_HttpRequestContext& context,
                                    NPT_HttpResponse*&            response,
                                    bool&                         headers_only)
{
    NPT_LOG_INFO_1("ProcessHttpRequest: %s", request.GetUrl().GetPath().GetChars());
    
    if (request.GetUrl().GetPath().StartsWith("/content/viewer/FLViewer_t.html") ||
        request.GetUrl().GetPath().StartsWith("/content/home.html")) {
        return ProcessTemplate(request, context, response, headers_only);
    }

    return PLT_HttpServer::ProcessHttpRequest(request, context, response, headers_only);
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::ProcessTemplate
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FrameServer::ProcessTemplate(NPT_HttpRequest&              request, 
                                 const NPT_HttpRequestContext& context,
                                 NPT_HttpResponse*&            response,
                                 bool&                         headers_only)
{
    NPT_COMPILER_UNUSED(context);
            
    NPT_String input;
    NPT_MemoryStreamReference output;
    NPT_InputStreamReference body;
    NPT_HttpEntity* entity;
    NPT_String file_path;

    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_INFO, &request);

    response = new NPT_HttpResponse(200, "OK", NPT_HTTP_PROTOCOL_1_0);
    response->GetHeaders().SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");

    if (request.GetMethod() != NPT_HTTP_METHOD_GET && 
        request.GetMethod() != NPT_HTTP_METHOD_HEAD) {
        response->SetStatus(500, "Internal Server Error");
        return NPT_SUCCESS;
    }
    
    // make sure url starts with our root
    if (!request.GetUrl().GetPath().StartsWith("/content", true)) {
        goto failure;
    }
    
    // Extract file path from url
    file_path = NPT_Uri::PercentDecode(
            request.GetUrl().GetPath().SubString(NPT_StringLength("/content")+1));
                            
    NPT_LOG_INFO_1("Resource = %s", file_path.GetChars());
                  
    // prevent hackers from accessing files outside of our root
    if ((file_path.Find("/..") >= 0) || (file_path.Find("\\..") >= 0)) {
        goto failure;
    }
    
    if (NPT_FAILED(NPT_File::Load(NPT_FilePath::Create(m_RootFilePath, file_path), 
                                  input, 
                                  NPT_FILE_OPEN_MODE_READ))) {
        // file didn't open
        goto failure;
    }
    
    // replace placeholders
    output = new NPT_MemoryStream(input.GetLength()+200); // pre-allocate enough space
    NPT_CHECK_LABEL_WARNING(Replace(input, *output), failure);
    
    // set entity
    body = output;
    entity = new NPT_HttpEntity();
    entity->SetInputStream(body, true);
    response->SetEntity(entity);
    
    // set headers_only flag
    headers_only = (request.GetMethod()==NPT_HTTP_METHOD_HEAD);
        
    return NPT_SUCCESS;

failure:
    response->SetStatus(404, "File Not Found");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::Replace
+---------------------------------------------------------------------*/
NPT_Result
PLT_FrameServer::Replace(const char* input, NPT_MemoryStream& output)
{
    NPT_String value;
    NPT_List<NPT_IpAddress> ips;
    PLT_UPnPMessageHelper::GetIPAddresses(ips);
    if (ips.GetItemCount() == 0) return NPT_FAILURE;
    
    const char* p = input;
    while (*p != '\0') {
        if (NPT_StringsEqualN(p, "$name", 5)) {
            value = m_FriendlyName;
            output.Write((const void*)value.GetChars(), value.GetLength());
            p += 5;
        } else if (NPT_StringsEqualN(p, "$host", 5)) {
            value = ips.GetFirstItem()->ToString();
            output.Write((const void*)value.GetChars(), value.GetLength());
            p += 5;
        } else if (NPT_StringsEqualN(p, "$port", 5)) {
            value = NPT_String::FromInteger(GetPort());
            output.Write((const void*)value.GetChars(), value.GetLength());
            p += 5;
        } else if (NPT_StringsEqualN(p, "$securityPort", 13)) {
            value = NPT_String::FromInteger(m_PolicyServer->m_Port);
            output.Write((const void*)value.GetChars(), value.GetLength());
            p += 13;
        } else {
            output.Write((const void*)p++, 1);
        }
    }
    return NPT_SUCCESS;
}

