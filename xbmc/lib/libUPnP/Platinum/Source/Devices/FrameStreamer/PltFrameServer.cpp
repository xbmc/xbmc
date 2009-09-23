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
#include "PltUPnP.h"
#include "PltMediaItem.h"
#include "PltService.h"
#include "PltDidl.h"
#include "PltFrameStream.h"
#include "PltFrameServer.h"

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
    PLT_SocketPolicyServer(const char* policy, NPT_UInt32 port = 0) :
        m_Policy(policy),
        m_Port(port),
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
            NPT_String policy = "<cross-domain-policy><allow-access-from domain=\""+client_info.local_address.GetIpAddress().ToString()+"\" to-ports=\"5900\"/></cross-domain-policy>";

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
    bool                m_Aborted;
};

/*----------------------------------------------------------------------
|   PLT_FrameServer::PLT_FrameServer
+---------------------------------------------------------------------*/
PLT_FrameServer::PLT_FrameServer(PLT_FrameBuffer&     frame_buffer, 
                                 const char*          www_root,
                                 const char*          friendly_name, 
                                 bool                 show_ip, 
                                 const char*          uuid, 
                                 NPT_UInt16           port,
                                 PLT_StreamValidator* stream_validator) :	
    PLT_FileMediaServer(www_root,
                        friendly_name, 
                        show_ip,
                        uuid, 
                        port),
    m_FrameBuffer(frame_buffer),
    m_PolicyServer(NULL),
    m_StreamValidator(stream_validator)
{
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::~PLT_FrameServer
+---------------------------------------------------------------------*/
PLT_FrameServer::~PLT_FrameServer()
{
    delete m_PolicyServer;
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::SetupDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_FrameServer::SetupDevice()
{
    // FIXME: hack for now: find the first valid non local ip address
    // to use in item resources. TODO: we should advertise all ips as
    // multiple resources instead.
    NPT_List<NPT_IpAddress> ips;
    PLT_UPnPMessageHelper::GetIPAddresses(ips);
    if (ips.GetItemCount() == 0) return NPT_ERROR_INTERNAL;

    // set the base paths for content
    m_StreamBaseUri = NPT_HttpUrl(ips.GetFirstItem()->ToString(), GetPort(), "/screensplitr");
    
    // start the xml socket policy server for flash
    m_PolicyServer = new PLT_SocketPolicyServer("", 8989);
    NPT_CHECK_SEVERE(m_PolicyServer->Start());

    return PLT_FileMediaServer::SetupDevice();
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FrameServer::ProcessHttpRequest(NPT_HttpRequest&              request, 
                                    const NPT_HttpRequestContext& context,
                                    NPT_HttpResponse&             response)
{
    NPT_LOG_INFO_1("ProcessHttpRequest: %s", request.GetUrl().GetPath().GetChars());

    if (request.GetUrl().GetPath().StartsWith(m_StreamBaseUri.GetPath())) {
        return ProcessStreamRequest(request, context, response);
    }
    
    if (request.GetUrl().GetPath().StartsWith(m_FileBaseUri.GetPath()+"/viewer/FLViewer_t.html") ||
        request.GetUrl().GetPath().StartsWith(m_FileBaseUri.GetPath()+"/home.html")) {
        return ProcessTemplate(request, context, response);
    }

    return PLT_FileMediaServer::ProcessHttpRequest(request, context, response);
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::ProcessTemplate
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FrameServer::ProcessTemplate(NPT_HttpRequest&              request, 
                                 const NPT_HttpRequestContext& context,
                                 NPT_HttpResponse&             response)
{
    NPT_COMPILER_UNUSED(context);
            
    NPT_String input;
    NPT_MemoryStreamReference output;
    NPT_InputStreamReference body;
    NPT_HttpEntity* entity;

    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_INFO, &request);

    if (request.GetMethod().Compare("GET")) {
        response.SetStatus(500, "Internal Server Error");
        return NPT_SUCCESS;
    }

    response.SetProtocol(NPT_HTTP_PROTOCOL_1_0);
    response.GetHeaders().SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");
    
    // Extract file path from url
    NPT_String file_path;
    NPT_CHECK_LABEL_WARNING(ExtractResourcePath(request.GetUrl(), file_path),  
                            failure);
                            
    NPT_LOG_INFO_1("Resource = %s", file_path.GetChars());
                  
    // prevent hackers from accessing files outside of our root
    if ((file_path.Find("/..") >= 0) || (file_path.Find("\\..") >= 0)) {
        goto failure;
    }
    
    if (NPT_FAILED(NPT_File::Load(NPT_FilePath::Create(m_Path, file_path), 
                                  input, 
                                  NPT_FILE_OPEN_MODE_READ))) {
        // file didn't open
        goto failure;
    }
    
    // replace placeholders
    output = new NPT_MemoryStream(input.GetLength()+200);
    NPT_CHECK_LABEL_WARNING(Replace(input, *output), failure);
    
    // set entity
    body = output;
    entity = new NPT_HttpEntity();
    entity->SetInputStream(body, true);
    response.SetEntity(entity);
    return NPT_SUCCESS;

failure:
    response.SetStatus(404, "File Not Found");
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

/*----------------------------------------------------------------------
|   PLT_FrameServer::ProcessStreamRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FrameServer::ProcessStreamRequest(NPT_HttpRequest&              request, 
                                      const NPT_HttpRequestContext& context,
                                      NPT_HttpResponse&             response)
{
    NPT_COMPILER_UNUSED(context);
    
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

    NPT_HttpEntity* entity = new NPT_HttpEntity();
    entity->SetContentType("multipart/x-mixed-replace;boundary=" BOUNDARY);

    NPT_InputStreamReference body(new PLT_InputFrameStream(m_FrameBuffer, BOUNDARY));
    entity->SetInputStream(body, false);

    return response.SetEntity(entity);
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
NPT_Result
PLT_FrameServer::OnBrowseMetadata(PLT_ActionReference&          action, 
                                  const char*                   object_id, 
                                  const char*                   filter,
                                  NPT_UInt32                    starting_index,
                                  NPT_UInt32                    requested_count,
                                  const NPT_List<NPT_String>&   sort_criteria,
                                  const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(starting_index);
    NPT_COMPILER_UNUSED(requested_count);
    NPT_COMPILER_UNUSED(sort_criteria);
    
    NPT_String didl;
    PLT_MediaObjectReference item;

    item = BuildFromID(object_id, &context.GetLocalAddress());
    if (item.IsNull())  {
        /* error */
        NPT_LOG_WARNING("PLT_FrameServer::OnBrowse - ObjectID not found.");
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    NPT_String tmp;    
    NPT_CHECK_SEVERE(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));

    /* add didl header and footer */
    didl = didl_header + tmp + didl_footer;

    NPT_CHECK_SEVERE(action->SetArgumentValue("Result", didl));
    NPT_CHECK_SEVERE(action->SetArgumentValue("NumberReturned", "1"));
    NPT_CHECK_SEVERE(action->SetArgumentValue("TotalMatches", "1"));

    // update ID may be wrong here, it should be the one of the container?
    // TODO: We need to keep track of the overall updateID of the CDS
    NPT_CHECK_SEVERE(action->SetArgumentValue("UpdateId", "1"));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
NPT_Result
PLT_FrameServer::OnBrowseDirectChildren(PLT_ActionReference&          action, 
                                        const char*                   object_id, 
                                        const char*                   filter,
                                        NPT_UInt32                    starting_index,
                                        NPT_UInt32                    requested_count,
                                        const NPT_List<NPT_String>&   sort_criteria,
                                        const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(requested_count);
    NPT_COMPILER_UNUSED(sort_criteria);
    
    /* Only root allowed */
    if (!NPT_StringsEqual(object_id, "0")) {
        /* error */
        NPT_LOG_WARNING("PLT_FrameServer::OnBrowse - ObjectID not found.");
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    NPT_String didl = didl_header;
    PLT_MediaObjectReference item;

    unsigned long num_returned = 0;
    unsigned long total_matches = 0;
    if (starting_index == 0) {
        item = BuildFromID("1", &context.GetLocalAddress());
        if (!item.IsNull()) {
            NPT_String tmp;
            NPT_CHECK_SEVERE(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));
            didl += tmp;

            num_returned++;
            total_matches++;        
        }
    };

    didl += didl_footer;

    NPT_CHECK_SEVERE(action->SetArgumentValue("Result", didl));
    NPT_CHECK_SEVERE(action->SetArgumentValue("NumberReturned", NPT_String::FromInteger(num_returned)));
    NPT_CHECK_SEVERE(action->SetArgumentValue("TotalMatches", NPT_String::FromInteger(total_matches)));
    NPT_CHECK_SEVERE(action->SetArgumentValue("UpdateId", "1"));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::BuildResourceUri
+---------------------------------------------------------------------*/
NPT_String
PLT_FrameServer::BuildResourceUri(const NPT_HttpUrl& base_uri, 
                                  const char*        host, 
                                  const char*        file_path)
{
    NPT_HttpUrl uri = base_uri;

    if (host) uri.SetHost(host);

    if (file_path) {
        uri.SetPath(uri.GetPath() + file_path);
    }
    
    return uri.ToStringWithDefaultPort(0);
}

/*----------------------------------------------------------------------
|   PLT_FrameServer::BuildFromID
+---------------------------------------------------------------------*/
PLT_MediaObject*
PLT_FrameServer::BuildFromID(const char* id,
                             const NPT_SocketAddress* req_local_address /* = NULL */)
{
    PLT_MediaItemResource resource;
    PLT_MediaObject*      object = NULL;

    if (NPT_StringsEqual(id, "1")) {
        object = new PLT_MediaItem();
        object->m_ObjectID = "1";
        object->m_ParentID = "0";

        /* Set the title using the filename for now */
        object->m_Title = "ScreenSplitr";

        /* Set the protocol Info from the extension */
        resource.m_ProtocolInfo = "http-get:*:video/x-motion-jpg:DLNA.ORG_OP=01";

        /* Set the resource file size */
        //resource.m_Size = info.m_Size;

        // get list of ip addresses
        NPT_List<NPT_IpAddress> ips;
        NPT_CHECK_LABEL_SEVERE(PLT_UPnPMessageHelper::GetIPAddresses(ips), failure);

        // if we're passed an interface where we received the request from
        // move the ip to the top
        if (req_local_address && req_local_address->GetIpAddress().ToString() != "0.0.0.0") {
            ips.Remove(req_local_address->GetIpAddress());
            ips.Insert(ips.GetFirstItem(), req_local_address->GetIpAddress());
        }

        // iterate through list and build list of resources
        NPT_List<NPT_IpAddress>::Iterator ip = ips.GetFirstItem();
        while (ip) {
            /* prepend the base URI and url encode it */ 
            //resource.m_Uri = NPT_Uri::Encode(uri.ToString(), NPT_Uri::UnsafeCharsToEncode);
            resource.m_Uri = BuildResourceUri(m_StreamBaseUri, ip->ToString(), NULL);

            object->m_ObjectClass.type = "object.item.videoItem.movie";
            object->m_Resources.Add(resource);

            ++ip;
        }
    } else if (NPT_StringsEqual(id, "0")) {
        object = new PLT_MediaContainer;

        object->m_ObjectID = "0";
        object->m_ParentID = "-1";
        object->m_Title    = "Root";
        ((PLT_MediaContainer*)object)->m_ChildrenCount = 1;

        object->m_ObjectClass.type = "object.container";
    }

    return object;

failure:
    delete object;
    return NULL;
}
