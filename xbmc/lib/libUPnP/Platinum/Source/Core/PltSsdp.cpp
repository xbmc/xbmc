/*****************************************************************
|
|   Platinum - SSDP
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
#include "PltSsdp.h"
#include "PltDatagramStream.h"
#include "PltDeviceHost.h"
#include "PltUPnP.h"
#include "PltHttp.h"
#include "PltVersion.h"

NPT_SET_LOCAL_LOGGER("platinum.core.ssdp")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const int NPT_SSDP_MAX_LINE_SIZE  = 2048;
const int NPT_SSDP_MAX_DGRAM_SIZE = 4096;

/*----------------------------------------------------------------------
|   PLT_SsdpSender::SendSsdp
+---------------------------------------------------------------------*/
NPT_Result
PLT_SsdpSender::SendSsdp(NPT_HttpRequest&   request,
                         const char*        usn,
                         const char*        target,
                         NPT_UdpSocket&     socket,
                         bool               notify,
                         const NPT_SocketAddress* addr /* = NULL */)
{
    NPT_CHECK_SEVERE(FormatPacket(request, usn, target, socket, notify));

    // logging
    NPT_LOG_FINE_2("Sending SSDP %s for %s",
        (const char*)request.GetMethod(), 
        usn);
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);

    // use a memory stream to write all the data
    NPT_MemoryStream stream;
    NPT_Result res = request.Emit(stream);
    if (NPT_FAILED(res)) return res;

    // copy stream into a data packet and send it
    NPT_LargeSize size;
    stream.GetSize(size);
    if (size != (NPT_Size)size) return NPT_ERROR_OUT_OF_RANGE;

    NPT_DataBuffer packet(stream.GetData(), (NPT_Size)size);
    return socket.Send(packet, addr);
}

/*----------------------------------------------------------------------
|   PLT_SsdpSender::SendSsdp
+---------------------------------------------------------------------*/
NPT_Result
PLT_SsdpSender::SendSsdp(NPT_HttpResponse&  response,
                         const char*        usn,
                         const char*        target,
                         NPT_UdpSocket&     socket,
                         bool               notify, 
                         const NPT_SocketAddress* addr /* = NULL */)
{
    NPT_CHECK_SEVERE(FormatPacket(response, usn, target, socket, notify));

    // logging
    NPT_LOG_FINE("Sending SSDP:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &response);

    // use a memory stream to write all the data
    NPT_MemoryStream stream;
    NPT_Result res = response.Emit(stream);
    if (NPT_FAILED(res)) return res;

    // copy stream into a data packet and send it
    NPT_LargeSize size;
    stream.GetSize(size);
    if (size != (NPT_Size)size) return NPT_ERROR_OUT_OF_RANGE;

    NPT_DataBuffer packet(stream.GetData(), (NPT_Size)size);
    return socket.Send(packet, addr);
}

/*----------------------------------------------------------------------
|   PLT_SsdpSender::FormatPacket
+---------------------------------------------------------------------*/
NPT_Result
PLT_SsdpSender::FormatPacket(NPT_HttpMessage& message, 
                             const char*      usn,
                             const char*      target,
                             NPT_UdpSocket&   socket,
                             bool             notify)
{
    NPT_COMPILER_UNUSED(socket);

    PLT_UPnPMessageHelper::SetUSN(message, usn);
    if (notify) {
        PLT_UPnPMessageHelper::SetNT(message, target);
    } else {
        PLT_UPnPMessageHelper::SetST(message, target);
    }
    //PLT_HttpHelper::SetContentLength(message, 0);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_SsdpDeviceSearchResponseInterfaceIterator class
+---------------------------------------------------------------------*/
NPT_Result
PLT_SsdpDeviceSearchResponseInterfaceIterator::operator()(NPT_NetworkInterface*& net_if) const 
{
    NPT_Result res;
    const NPT_SocketAddress* remote_addr = &m_RemoteAddr;

    NPT_List<NPT_NetworkInterfaceAddress>::Iterator niaddr = 
        net_if->GetAddresses().GetFirstItem();
    if (!niaddr) return NPT_SUCCESS;

    NPT_UdpSocket socket;

    // by connecting, the kernel chooses which interface to use to route to the remote
    // this is the IP we should use in our Location URL header
    if (NPT_FAILED(res = socket.Connect(m_RemoteAddr, 5000))) {
        return res;
    }
    NPT_SocketInfo info;
    socket.GetInfo(info);

    // did we successfully connect and found out which interface is used?
    if (info.local_address.GetIpAddress().AsLong()) {
        // check that the interface the kernel chose matches the interface
        // we wanted to send on
        if ((*niaddr).GetPrimaryAddress().AsLong() != info.local_address.GetIpAddress().AsLong()) {
            return NPT_SUCCESS;
        }

        // socket already connected, so we don't need to specify where to go
        remote_addr = NULL;
    }

    NPT_HttpResponse response(200, "OK", NPT_HTTP_PROTOCOL_1_1);
    PLT_UPnPMessageHelper::SetLocation(response, m_Device->GetDescriptionUrl((*niaddr).GetPrimaryAddress().ToString()));
    PLT_UPnPMessageHelper::SetLeaseTime(response, (NPT_Timeout)((float)m_Device->GetLeaseTime()));
    PLT_UPnPMessageHelper::SetServer(response, NPT_HttpServer::m_ServerHeader, false);
    response.GetHeaders().SetHeader("EXT", "");

    // process search response twice to be NMPR compliant
    NPT_CHECK_SEVERE(m_Device->SendSsdpSearchResponse(response, socket, m_ST, remote_addr));
    NPT_CHECK_SEVERE(m_Device->SendSsdpSearchResponse(response, socket, m_ST, remote_addr));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_SsdpDeviceSearchResponseTask::DoRun()
+---------------------------------------------------------------------*/
void
PLT_SsdpDeviceSearchResponseTask::DoRun() 
{
    NPT_List<NPT_NetworkInterface*> if_list;
    NPT_CHECK_LABEL_WARNING(PLT_UPnPMessageHelper::GetNetworkInterfaces(if_list, true), 
                            done);

    if_list.Apply(PLT_SsdpDeviceSearchResponseInterfaceIterator(
        m_Device, 
        m_RemoteAddr, 
        m_ST));
    if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());

done:
    return;
}

/*----------------------------------------------------------------------
|   PLT_SsdpAnnounceInterfaceIterator class
+---------------------------------------------------------------------*/
NPT_Result
PLT_SsdpAnnounceInterfaceIterator::operator()(NPT_NetworkInterface*& net_if) const 
{
    // don't use this interface address if it's not broadcast capable
    if (m_Broadcast && !(net_if->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_BROADCAST)) {
        return NPT_FAILURE;
    }

    NPT_List<NPT_NetworkInterfaceAddress>::Iterator niaddr = 
        net_if->GetAddresses().GetFirstItem();
    if (!niaddr) return NPT_FAILURE;

    // Remove disconnected interfaces
    NPT_IpAddress addr =  (*niaddr).GetPrimaryAddress();
    if (!addr.ToString().Compare("0.0.0.0")) return NPT_FAILURE;

    NPT_HttpUrl            url;
    NPT_UdpMulticastSocket multicast_socket;
    NPT_UdpSocket          broadcast_socket;
    NPT_UdpSocket*         socket;

    if (m_Broadcast) {
        //url = NPT_HttpUrl("255.255.255.255", 1900, "*");
        url = NPT_HttpUrl((*niaddr).GetBroadcastAddress().ToString(), 1900, "*");
        socket = &broadcast_socket;
    } else {
        url = NPT_HttpUrl("239.255.255.250", 1900, "*");
        socket = &multicast_socket;
        NPT_CHECK_SEVERE(((NPT_UdpMulticastSocket*)socket)->SetInterface(addr));
    }

    NPT_HttpRequest req(url, "NOTIFY", NPT_HTTP_PROTOCOL_1_1);
    PLT_HttpHelper::SetHost(req, "239.255.255.250:1900");
    
    // put a location only if alive message
    if (m_IsByeBye == false) {
        PLT_UPnPMessageHelper::SetLocation(req, m_Device->GetDescriptionUrl(addr.ToString()));
    }

    NPT_CHECK_SEVERE(m_Device->Announce(req, *socket, m_IsByeBye));
    NPT_CHECK_SEVERE(m_Device->Announce(req, *socket, m_IsByeBye));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_SsdpDeviceAnnounceUnicastTask::DoRun
+---------------------------------------------------------------------*/
void
PLT_SsdpDeviceAnnounceTask::DoRun()
{
    NPT_Result res = NPT_SUCCESS;
    NPT_List<NPT_NetworkInterface*> if_list;

    while (1) {
        NPT_CHECK_LABEL_FATAL(PLT_UPnPMessageHelper::GetNetworkInterfaces(if_list, true),
                              cleanup);

        // if we're announcing our arrival, sends a byebye first (NMPR compliance)
        if (m_IsByeByeFirst == true) {
            m_IsByeByeFirst = false;
            res = if_list.Apply(PLT_SsdpAnnounceInterfaceIterator(m_Device, true, m_IsBroadcast));
            if (NPT_FAILED(res)) goto cleanup;

            // schedule to announce alive in 300 ms
            if (NPT_FAILED(res) || IsAborting((NPT_Timeout)300)) break;
        }
            
        res = if_list.Apply(PLT_SsdpAnnounceInterfaceIterator(m_Device, false, m_IsBroadcast));
        
cleanup:
        if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
        if_list.Clear();

        if (NPT_FAILED(res) || IsAborting(m_Repeat.m_Seconds*1000)) break;
    };
}

/*----------------------------------------------------------------------
|    PLT_SsdpListenTask::DoInit
+---------------------------------------------------------------------*/
void
PLT_SsdpListenTask::DoInit() 
{
    if (m_Multicast) {
        if (m_JoinHard) {
            NPT_List<NPT_IpAddress> ips;
            PLT_UPnPMessageHelper::GetIPAddresses(ips);

            /* Join multicast group for every ip we found */
            ips.Apply(PLT_SsdpInitMulticastIterator((NPT_UdpMulticastSocket*)m_Socket));
        } else {
            NPT_IpAddress addr;
            addr.ResolveName("239.255.255.250");
            ((NPT_UdpMulticastSocket*)m_Socket)->JoinGroup(addr, NPT_IpAddress::Any);
        }
    }
}

/*----------------------------------------------------------------------
|    PLT_SsdpListenTask::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpListenTask::GetInputStream(NPT_InputStreamReference& stream) 
{
    if (!m_Datagram.IsNull()) {
        stream = m_Datagram;
        return NPT_SUCCESS;
    } else {
        NPT_InputStreamReference input_stream;
        NPT_Result res = m_Socket->GetInputStream(input_stream);
        if (NPT_FAILED(res)) {
            return res;
        }
        // for datagrams, we can't simply write to the socket directly
        // we need to write into a datagramstream (buffer) that redirects to the real stream when flushed
        m_Datagram = new PLT_InputDatagramStream((NPT_UdpSocket*)m_Socket);
        stream = m_Datagram;
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|    PLT_SsdpListenTask::GetInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpListenTask::GetInfo(NPT_SocketInfo& info) 
{
    if (m_Datagram.IsNull()) return NPT_FAILURE;
    return m_Datagram->GetInfo(info);
}

/*----------------------------------------------------------------------
|    PLT_SsdpListenTask::ProcessRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpListenTask::ProcessRequest(NPT_HttpRequest&              request, 
                                   const NPT_HttpRequestContext& context,
                                   NPT_HttpResponse*&            response,
                                   bool&                         headers_only) 
{
    NPT_COMPILER_UNUSED(headers_only);

    NPT_AutoLock lock(m_Mutex);
    m_Listeners.Apply(PLT_SsdpPacketListenerIterator(request, context));

    // set response to NULL since we don't have anything to respond
    // as we use a separate task to respond with ssdp
    response = NULL;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|    PLT_SsdpSearchTask::PLT_SsdpSearchTask
+---------------------------------------------------------------------*/
PLT_SsdpSearchTask::PLT_SsdpSearchTask(NPT_UdpSocket*                  socket,
                                       PLT_SsdpSearchResponseListener* listener, 
                                       NPT_HttpRequest*                request,
                                       NPT_Timeout                     frequency) : 
    m_Listener(listener), 
    m_Request(request),
    m_Timeout(frequency?frequency:30000),
    m_Repeat(frequency!=0),
    m_Socket(socket)
{
    m_Socket->SetReadTimeout(m_Timeout);
    m_Socket->SetWriteTimeout(10000);
}

/*----------------------------------------------------------------------
|   PLT_SsdpSearchTask::~PLT_SsdpSearchTask
+---------------------------------------------------------------------*/
PLT_SsdpSearchTask::~PLT_SsdpSearchTask()
{
    delete m_Socket;
    delete m_Request;
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::DoAbort
+---------------------------------------------------------------------*/
void
PLT_SsdpSearchTask::DoAbort()
{
     m_Socket->Disconnect();
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::DoRun
+---------------------------------------------------------------------*/
void
PLT_SsdpSearchTask::DoRun()
{
    NPT_HttpResponse*      response = NULL;
    NPT_Timeout            timeout = 30;
    NPT_HttpRequestContext context;

    do {
        // get the address of the server
        NPT_IpAddress server_address;
        NPT_CHECK_LABEL_SEVERE(server_address.ResolveName(
                                   m_Request->GetUrl().GetHost(), 
                                   timeout), 
                               done);
        NPT_SocketAddress address(server_address, 
                                  m_Request->GetUrl().GetPort());

        // send 2 requests in a row
        NPT_OutputStreamReference output_stream(
            new PLT_OutputDatagramStream(m_Socket, 
                                         4096, 
                                         &address));
        NPT_CHECK_LABEL_SEVERE(NPT_HttpClient::WriteRequest(
                                   *output_stream.AsPointer(), 
                                   *m_Request), 
                               done);
        NPT_CHECK_LABEL_SEVERE(NPT_HttpClient::WriteRequest(
                                   *output_stream.AsPointer(), 
                                   *m_Request), 
                               done);
        output_stream = NULL;

        // keep track of when we sent the request
        NPT_TimeStamp last_send;
        NPT_System::GetCurrentTimeStamp(last_send);

        while (!IsAborting(0)) {
            // read response
            PLT_InputDatagramStreamReference input_stream(
                new PLT_InputDatagramStream(m_Socket));

            NPT_InputStreamReference stream = input_stream;
            NPT_Result res = NPT_HttpClient::ReadResponse(
                stream, 
                false,
                response);
            // callback to process response
            if (NPT_SUCCEEDED(res)) {
                // get source info    
                NPT_SocketInfo info;
                input_stream->GetInfo(info);

                context.SetLocalAddress(info.local_address);
                context.SetRemoteAddress(info.remote_address);

                // process response
                ProcessResponse(NPT_SUCCESS, m_Request, context, response);
                delete response;
                response = NULL;
            } else if (res != NPT_ERROR_TIMEOUT) {
                NPT_LOG_WARNING_1("PLT_SsdpSearchTask got an error (%d) waiting for response", res);
            }

            input_stream = NULL;

            // check if it's time to resend request
            NPT_TimeStamp now;
            NPT_System::GetCurrentTimeStamp(now);
            if (now >= last_send + (long)m_Timeout/1000)
                break;
        }
    } while (!IsAborting(0) && m_Repeat);

done:
    return;
}

/*----------------------------------------------------------------------
|    PLT_CtrlPointGetDescriptionTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpSearchTask::ProcessResponse(NPT_Result                    res, 
                                    NPT_HttpRequest*              request,  
                                    const NPT_HttpRequestContext& context,
                                    NPT_HttpResponse*             response)
{
    NPT_COMPILER_UNUSED(request);
    return m_Listener->ProcessSsdpSearchResponse(res, context, response);
}
