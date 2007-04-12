/*****************************************************************
|
|   Platinum - Device Host
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_DEVICE_HOST_H_
#define _PLT_DEVICE_HOST_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDefs.h"
#include "NptStrings.h"
#include "NptHttp.h"
#include "PltDeviceData.h"
#include "PltHttpServerListener.h"
#include "PltSsdpListener.h"
#include "PltTaskManager.h"
#include "PltAction.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServer;
class PLT_HttpServerHandler;
class PLT_SsdpDeviceAnnounceTask;

/*----------------------------------------------------------------------
|   PLT_DeviceHost class
+---------------------------------------------------------------------*/
class PLT_DeviceHost : public PLT_DeviceData,
                       public PLT_SsdpPacketListener
{
public:
    PLT_DeviceHost(
        const char*  description_path = "/",
        const char*  uuid = "",
        const char*  device_type = "",
        const char*  friendly_name = "",
        unsigned int port = 0);

    virtual void SetBroadcast(bool broadcast) { m_Broadcast = broadcast; }

    // Overridables
    virtual NPT_Result OnAction(PLT_ActionReference& action, 
                                NPT_SocketInfo*      info = NULL);

    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest*  request, 
                                          NPT_SocketInfo    info, 
                                          NPT_HttpResponse* response);
    
    // PLT_SsdpDeviceAnnounceTask & PLT_SsdpDeviceAnnounceUnicastTask
    virtual NPT_Result Announce(PLT_DeviceData*  device, 
                                NPT_HttpRequest& request, 
                                NPT_UdpSocket&   socket, 
                                bool             byebye);

    virtual NPT_Result Announce(NPT_HttpRequest& request, 
                                NPT_UdpSocket&   socket, 
                                bool             byebye) {
        return Announce(this, request, socket, byebye);
    }

    // PLT_SsdpPacketListener method
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest* request, 
                                    NPT_SocketInfo   info);

    // PLT_SsdpDeviceSearchListenTask
    virtual NPT_Result ProcessSsdpSearchRequest(NPT_HttpRequest* request, 
                                                NPT_SocketInfo   info);

    // PLT_SsdpDeviceSearchResponseTask
    virtual NPT_Result SendSsdpSearchResponse(PLT_DeviceData*   device, 
                                              NPT_HttpResponse& response, 
                                              NPT_UdpSocket&    socket, 
                                              const char*       st,
                                              NPT_SocketAddress* addr  = NULL);
    virtual NPT_Result SendSsdpSearchResponse(NPT_HttpResponse& response, 
                                              NPT_UdpSocket&    socket, 
                                              const char*       ST,
                                              NPT_SocketAddress* addr = NULL) {
        return SendSsdpSearchResponse(this, response, socket, ST, addr);
    }
    
protected:
    virtual ~PLT_DeviceHost();
    
    virtual NPT_Result Start(PLT_TaskManager* task_manager);
    virtual NPT_Result Stop();

    virtual NPT_Result ProcessHttpGetHeadRequest(NPT_HttpRequest*  request, 
                                                 NPT_SocketInfo    info, 
                                                 NPT_HttpResponse* response);

    virtual NPT_Result ProcessHttpPostRequest(NPT_HttpRequest*  request, 
                                              NPT_SocketInfo    info, 
                                              NPT_HttpResponse* response);

    virtual NPT_Result ProcessHttpSubscriberRequest(NPT_HttpRequest* request, 
                                                   NPT_SocketInfo    info, 
                                                   NPT_HttpResponse* response);

    virtual NPT_Result ProcessHttpEventRequest(NPT_HttpRequest*  request, 
                                               NPT_SocketInfo    info, 
                                               NPT_HttpResponse* response);

protected:
    friend class PLT_UPnP;
    friend class PLT_UPnP_DeviceStartIterator;
    friend class PLT_UPnP_DeviceStopIterator;
    friend class PLT_Service;
    friend class NPT_Reference<PLT_DeviceHost>;

private:
    PLT_TaskManager*            m_TaskManager;
    PLT_HttpServerHandler*      m_HttpServerHandler;
    PLT_HttpServer*             m_HttpServer;
    PLT_SsdpDeviceAnnounceTask* m_SsdpAnnounceTask;
    bool                        m_Broadcast;
};

typedef NPT_Reference<PLT_DeviceHost> PLT_DeviceHostReference;

/*----------------------------------------------------------------------
|   PLT_HttpServerHandler
+---------------------------------------------------------------------*/
class PLT_HttpServerHandler : public PLT_HttpServerListener
{
public:
    PLT_HttpServerHandler(PLT_DeviceHost* device) : m_Device(device) {}
    virtual ~PLT_HttpServerHandler() {}

    // PLT_HttpServerListener methods
    NPT_Result ProcessHttpRequest(NPT_HttpRequest*   request, 
                                  NPT_SocketInfo     info, 
                                  NPT_HttpResponse*& response);

private:
    PLT_DeviceHost* m_Device;
};

#endif /* _PLT_DEVICE_HOST_H_ */
