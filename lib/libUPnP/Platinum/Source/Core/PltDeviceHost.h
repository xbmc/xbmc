/*****************************************************************
|
|   Platinum - Device Host
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

#ifndef _PLT_DEVICE_HOST_H_
#define _PLT_DEVICE_HOST_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltDeviceData.h"
#include "PltHttpServerListener.h"
#include "PltSsdpListener.h"
#include "PltTaskManager.h"
#include "PltAction.h"
#include "PltHttp.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServer;
class PLT_HttpServerHandler;
class PLT_SsdpDeviceAnnounceTask;
class PLT_SsdpListenTask;

/*----------------------------------------------------------------------
|   PLT_DeviceHost class
+---------------------------------------------------------------------*/
class PLT_DeviceHost : public PLT_DeviceData,
                       public PLT_SsdpPacketListener
{
public:
    PLT_DeviceHost(const char*  description_path = "/",
                   const char*  uuid = "",
                   const char*  device_type = "",
                   const char*  friendly_name = "",
                   bool         show_ip = false,
                   NPT_UInt16   port = 0,
                   bool         port_rebind = false);

    // public methods
    virtual void       SetBroadcast(bool broadcast) { m_Broadcast = broadcast; }
    virtual NPT_UInt16 GetPort() { return m_Port; };

    // NPT_HttpRequestHandler forward for control/event requests
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request,
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);
    
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
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest&              request, 
                                    const NPT_HttpRequestContext& context);

    // PLT_SsdpDeviceSearchListenTask
    virtual NPT_Result ProcessSsdpSearchRequest(NPT_HttpRequest&              request, 
                                                const NPT_HttpRequestContext& context);

    // PLT_SsdpDeviceSearchResponseTask
    virtual NPT_Result SendSsdpSearchResponse(PLT_DeviceData*   device, 
                                              NPT_HttpResponse& response, 
                                              NPT_UdpSocket&    socket, 
                                              const char*       st,
                                              const NPT_SocketAddress* addr  = NULL);
    virtual NPT_Result SendSsdpSearchResponse(NPT_HttpResponse& response, 
                                              NPT_UdpSocket&    socket, 
                                              const char*       ST,
                                              const NPT_SocketAddress* addr = NULL) {
        return SendSsdpSearchResponse(this, response, socket, ST, addr);
    }
    
protected:
    virtual ~PLT_DeviceHost();

    // pure methods
    virtual NPT_Result SetupServices(PLT_DeviceData& data) = 0;
    
    // setup methods
    virtual NPT_Result SetupIcons();
    virtual NPT_Result SetupDevice();
    
    // overridable methods
    virtual NPT_Result AddIcon(const PLT_DeviceIcon& icon, 
                               const char*           filepath);
    virtual NPT_Result AddIcon(const PLT_DeviceIcon& icon, 
                               const void*           data, 
                               NPT_Size              size, 
                               bool                  copy = true);
    virtual NPT_Result Start(PLT_SsdpListenTask* task);
    virtual NPT_Result Stop(PLT_SsdpListenTask* task);
    virtual NPT_Result SetupServiceSCPDHandler(PLT_Service* service);
    virtual NPT_Result OnAction(PLT_ActionReference&          action, 
                                const PLT_HttpRequestContext& context);
    virtual NPT_Result ProcessGetDescription(NPT_HttpRequest&              request,
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response);
    virtual NPT_Result ProcessHttpGetRequest(NPT_HttpRequest&              request,
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response);
    virtual NPT_Result ProcessHttpPostRequest(NPT_HttpRequest&              request,
                                              const NPT_HttpRequestContext& context,
                                              NPT_HttpResponse&             response);
    virtual NPT_Result ProcessHttpSubscriberRequest(NPT_HttpRequest&              request,
                                                    const NPT_HttpRequestContext& context,
                                                    NPT_HttpResponse&             response);

protected:
    friend class PLT_UPnP;
    friend class PLT_UPnP_DeviceStartIterator;
    friend class PLT_UPnP_DeviceStopIterator;
    friend class PLT_Service;
    friend class NPT_Reference<PLT_DeviceHost>;

private:
    PLT_TaskManager                   m_TaskManager;
    PLT_HttpServer*                   m_HttpServer;
    bool                              m_Broadcast;
    NPT_UInt16                        m_Port;
    bool                              m_PortRebind;
    NPT_List<NPT_HttpRequestHandler*> m_RequestHandlers;
};

typedef NPT_Reference<PLT_DeviceHost> PLT_DeviceHostReference;

#endif /* _PLT_DEVICE_HOST_H_ */
