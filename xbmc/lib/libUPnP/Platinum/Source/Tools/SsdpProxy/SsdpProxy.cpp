/*****************************************************************
|
|   Platinum - Ssdp Proxy
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "NptUtils.h"
#include "SsdpProxy.h"
#include "PltLog.h"
#include "PltUPnPHelper.h"

#if defined (WIN32)
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#endif

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
static struct {
    long port;
} Options;

/*----------------------------------------------------------------------
|   PLT_SsdpProxy::PLT_SsdpProxy
+---------------------------------------------------------------------*/
PLT_SsdpProxy::PLT_SsdpProxy() : 
    m_UnicastListener(NULL)
{

}

/*----------------------------------------------------------------------
|   PLT_SsdpProxy::~PLT_SsdpProxy
+---------------------------------------------------------------------*/
PLT_SsdpProxy::~PLT_SsdpProxy()
{
    delete m_UnicastListener;
}

/*----------------------------------------------------------------------
|   PLT_SsdpProxy::Start
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpProxy::Start(NPT_UInt32 port)
{
    /* create a SSDP multicast listener task */
    NPT_Socket* multicast_socket = new NPT_UdpMulticastSocket();
    NPT_CHECK(multicast_socket->Bind(NPT_SocketAddress(NPT_IpAddress::Any, 1900)));
    PLT_SsdpListenTask* m_SsdpMulticastListenTask = new PLT_SsdpListenTask(multicast_socket, true);
    NPT_CHECK(m_SsdpMulticastListenTask->AddListener(this));
    NPT_CHECK(StartTask(m_SsdpMulticastListenTask));

    /* create a SSDP unicast listener task */
    /* any broadcast message sent to that, we will receive */
    NPT_Socket* unicast_socket = new NPT_UdpSocket();
    NPT_CHECK(unicast_socket->Bind(NPT_SocketAddress(NPT_IpAddress::Any, port)));
    m_UnicastListener = new PLT_SsdpUnicastListener(this);
    PLT_SsdpListenTask* m_SsdpUnicastListenTask = new PLT_SsdpListenTask(unicast_socket, false);
    NPT_CHECK(m_SsdpUnicastListenTask->AddListener(m_UnicastListener));
    NPT_CHECK(StartTask(m_SsdpUnicastListenTask));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_SsdpProxy::OnSsdpPacket
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpProxy::OnSsdpPacket(NPT_HttpRequest* request, NPT_SocketInfo info) 
{
    NPT_COMPILER_UNUSED(info);

    if (request == NULL) return NPT_FAILURE;

    // is it a forward message ?
    if (request->GetHeaders().GetHeader("X_SsdpProxy")) 
        return NPT_SUCCESS;

    // forward in broadcast
    // copy request and add extra header so we know it's a forward
    NPT_HttpRequest* new_request = new NPT_HttpRequest(NPT_HttpUrl("255.255.255.255", 1900, request->GetUrl().GetPath()), request->GetMethod(), request->GetProtocol());
    NPT_List<NPT_HttpHeader*>::Iterator headers = request->GetHeaders().GetHeaders().GetFirstItem();
    while (headers) {
        new_request->GetHeaders().AddHeader((*headers)->GetName(), (*headers)->GetValue());
        ++headers;
    }

    // override MX to force a fast response
    long MX;
    if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetMX(new_request, MX))) {
        PLT_UPnPMessageHelper::SetMX(new_request, 1);
    }

    new_request->GetHeaders().AddHeader("X_SsdpProxy", "forwarded");

    // create search task to send request in unicast
    // and forward responses back to remote
    PLT_SsdpProxyForwardTask* task = new PLT_SsdpProxyForwardTask(
        new NPT_UdpSocket(),
        new_request,
        50000,
        info.remote_address); 
    StartTask(task);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_SsdpProxy::OnUnicastSsdpPacket
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpProxy::OnUnicastSsdpPacket(NPT_HttpRequest* request, NPT_SocketInfo info) 
{
    // is it a forward message ?
    if (request->GetHeaders().GetHeader("X_SsdpProxy")) 
        return NPT_SUCCESS;

    // look on which interface we received it and send the ssdp search request on this only
    NPT_List<NPT_NetworkInterface*> if_list;
    NPT_List<NPT_NetworkInterface*>::Iterator net_if;
    NPT_List<NPT_NetworkInterfaceAddress>::Iterator net_if_addr;

    NPT_CHECK(NPT_NetworkInterface::GetNetworkInterfaces(if_list));

    for (net_if = if_list.GetFirstItem(); net_if; net_if++) {
        if (!((*net_if)->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_MULTICAST) ||
              (*net_if)->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_LOOPBACK) {
        //if (!((*net_if)->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_MULTICAST)) {
            continue;
        }       

        for (net_if_addr = (*net_if)->GetAddresses().GetFirstItem(); net_if_addr; net_if_addr++) {
            // by using the netmask on each interface, we can figure out if the remote IP address
            // we received the request from matches
            int i=0;
            while (i<4) {
                if ((info.remote_address.GetIpAddress().AsBytes()[i] & (*net_if_addr).GetNetMask().AsBytes()[i]) != 
                    ((*net_if_addr).GetPrimaryAddress().AsBytes()[i] & (*net_if_addr).GetNetMask().AsBytes()[i])) {
                    break;
                }
                i++;
            }

            /* check that we have a match */
            if (i != 4) {
                //continue;
            }

            // override MX to force a fast response
            long MX;
            if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetMX(request, MX))) {
                PLT_UPnPMessageHelper::SetMX(request, 1);
            }

            // unicast
            // WMC doesn't respond to multicast searches issued from the same machine
            // a work around is to send a unicast request!
            // copy request and change host header
            NPT_HttpRequest* unicast_request = new NPT_HttpRequest(NPT_HttpUrl((*net_if_addr).GetPrimaryAddress().ToString(), 1900, request->GetUrl().GetPath()), request->GetMethod(), request->GetProtocol());
            NPT_List<NPT_HttpHeader*>::Iterator unicast_headers = request->GetHeaders().GetHeaders().GetFirstItem();
            while (unicast_headers) {
                unicast_request->GetHeaders().AddHeader((*unicast_headers)->GetName(), (*unicast_headers)->GetValue());
                ++unicast_headers;
            }
            unicast_request->GetHeaders().AddHeader("X_SsdpProxy", "forwarded");

            // create unicast socket
            NPT_UdpSocket* unicast_socket = new NPT_UdpSocket();
            // create search task to send request in unicast
            // and forward responses back to remote
            PLT_SsdpProxyForwardTask* uncast_task = new PLT_SsdpProxyForwardTask(
                unicast_socket,
                unicast_request,
                50000,
                info.remote_address); 
            StartTask(uncast_task);

            // multicast
            // simply redirect request to 239.255.255.250
            NPT_HttpRequest* new_request = new NPT_HttpRequest(NPT_HttpUrl("239.255.255.250", 1900, request->GetUrl().GetPath()), request->GetMethod(), request->GetProtocol());
            NPT_List<NPT_HttpHeader*>::Iterator headers = request->GetHeaders().GetHeaders().GetFirstItem();
            while (headers) {
                new_request->GetHeaders().AddHeader((*headers)->GetName(), (*headers)->GetValue());
                ++headers;
            }
            new_request->GetHeaders().AddHeader("X_SsdpProxy", "forwarded");

            // create multicast socket
            NPT_UdpMulticastSocket* socket = new NPT_UdpMulticastSocket();
            socket->SetInterface((*net_if_addr).GetPrimaryAddress());
            socket->SetTimeToLive(4);

            // create search task to send request in multicast
            // and forward responses back to remote
            PLT_SsdpProxyForwardTask* task = new PLT_SsdpProxyForwardTask(
                socket,
                new_request,
                50000,
                info.remote_address); 
            StartTask(task);
        }
    }

    if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_SsdpUnicastListener::OnSsdpPacket
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpUnicastListener::OnSsdpPacket(NPT_HttpRequest* request, NPT_SocketInfo info) 
{
    return m_Proxy->OnUnicastSsdpPacket(request, info);
}

/*----------------------------------------------------------------------
|   PLT_SsdpProxyForwardTask::PLT_SsdpProxyForwardTask
+---------------------------------------------------------------------*/
PLT_SsdpProxyForwardTask::PLT_SsdpProxyForwardTask(NPT_UdpSocket*     socket,
                                                   NPT_HttpRequest*   request, 
                                                   NPT_Timeout        timeout,
                                                   NPT_SocketAddress& forward_address) : 
    PLT_SsdpSearchTask(socket, NULL, request, timeout, false),
    m_ForwardAddress(forward_address) 
{
}

/*----------------------------------------------------------------------
|   PLT_SsdpProxyForwardTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SsdpProxyForwardTask::ProcessResponse(NPT_Result        res, 
                                          NPT_HttpRequest*  request, 
                                          NPT_SocketInfo&   info, 
                                          NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(info);
    NPT_COMPILER_UNUSED(request);

    if (NPT_FAILED(res) || response == NULL) return NPT_FAILURE;

    // use a memory stream to write the response 
    NPT_MemoryStream stream;
    NPT_CHECK(response->Emit(stream));

    // copy stream into a data packet and forward it
    NPT_Size size;
    stream.GetSize(size);
    NPT_DataBuffer packet(stream.GetData(), size);
    NPT_UdpSocket socket;
    return socket.Send(packet, &m_ForwardAddress);
}

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(char** args)
{
    fprintf(stderr, "usage: %s [-p <port>] \n", args[0]);
    fprintf(stderr, "-p : optional upnp unicast ssdp port (default: 1901)\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseCommandLine
+---------------------------------------------------------------------*/
static void
ParseCommandLine(char** args)
{
    const char* arg;
    char** tmp = args+1;

    /* default values */
    Options.port = 1900;

    while ((arg = *tmp++)) {
        if (!strcmp(arg, "-p")) {
            if (NPT_FAILED(NPT_ParseInteger(*tmp++, Options.port, false))) {
                fprintf(stderr, "ERROR: invalid argument\n");
                PrintUsageAndExit(args);
            }
        } else {
            fprintf(stderr, "ERROR: invalid arguments\n");
            PrintUsageAndExit(args);
        }
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
    NPT_COMPILER_UNUSED(argc);

    //PLT_SetLogLevel(4);
    PLT_SsdpProxy proxy;    
    
    /* parse command line */
    ParseCommandLine(argv);

    NPT_Result res = proxy.Start(Options.port);
    if (res == NPT_ERROR_BIND_FAILED) {
        fprintf(stderr, "ERROR: couldn't bind to port %ld\n", Options.port);
        return -1;
    } else if (NPT_FAILED(res)) {
        fprintf(stderr, "ERROR: unknown (%d)\n", res);
        return -1;
    }

    fprintf(stdout, "Listening for SSDP unicast packets on port %ld\n", Options.port);
    fprintf(stdout, "Enter q to quit\n");

    char buf[256];
    while (gets(buf)) {
        if (*buf == 'q')
            break;
    }

    return 0;
}
