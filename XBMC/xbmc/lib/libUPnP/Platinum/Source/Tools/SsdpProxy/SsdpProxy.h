/*****************************************************************
|
|   Platinum - Ssdp Proxy tool
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _SSDP_PROXY_H_
#define _SSDP_PROXY_H_

#include "Neptune.h"
#include "PltTaskManager.h"
#include "PltSsdp.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_SsdpUnicastListener;

/*----------------------------------------------------------------------
|   PLT_SsdpProxy class
+---------------------------------------------------------------------*/
class PLT_SsdpProxy : public PLT_TaskManager,
                      public PLT_SsdpPacketListener
{
public:
    PLT_SsdpProxy();
    ~PLT_SsdpProxy();

    NPT_Result Start(NPT_UInt32 port);

    // PLT_SsdpPacketListener method
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest& request, NPT_SocketInfo info);

    // PLT_SsdpUnicastListener redirect
    virtual NPT_Result OnUnicastSsdpPacket(NPT_HttpRequest& request, NPT_SocketInfo info);

private:
    PLT_SsdpUnicastListener* m_UnicastListener;
};

/*----------------------------------------------------------------------
|   PLT_SsdpUnicastListener class
+---------------------------------------------------------------------*/
class PLT_SsdpUnicastListener :  public PLT_SsdpPacketListener
{
public:
    PLT_SsdpUnicastListener(PLT_SsdpProxy* proxy) : m_Proxy(proxy) {}

    // PLT_SsdpPacketListener method
    NPT_Result OnSsdpPacket(NPT_HttpRequest& request, NPT_SocketInfo info);

private:
    PLT_SsdpProxy* m_Proxy;
};

/*----------------------------------------------------------------------
|   PLT_SsdpProxySearchResponseListener class
+---------------------------------------------------------------------*/
class PLT_SsdpProxyForwardTask : public PLT_SsdpSearchTask 
{
public:
    PLT_SsdpProxyForwardTask(NPT_UdpSocket*     socket,
                             NPT_HttpRequest*   request, 
                             NPT_Timeout        timeout,
                             NPT_SocketAddress& forward_address);

    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response);

private:
    NPT_SocketAddress m_ForwardAddress;
};

#endif // _SSDP_PROXY_H_
