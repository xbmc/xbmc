/*****************************************************************
|
|   Platinum - Control Point
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
#include "PltCtrlPoint.h"
#include "PltUPnP.h"
#include "PltDeviceData.h"
#include "PltXmlHelper.h"
#include "PltCtrlPointTask.h"
#include "PltSsdp.h"
#include "PltHttpServer.h"

NPT_SET_LOCAL_LOGGER("platinum.core.ctrlpoint")

/*----------------------------------------------------------------------
|   typedef
+---------------------------------------------------------------------*/
typedef PLT_HttpRequestHandler<PLT_CtrlPoint> PLT_HttpCtrlPointRequestHandler;

/*----------------------------------------------------------------------
|   PLT_CtrlPointListenerOnDeviceAddedIterator class
+---------------------------------------------------------------------*/
class PLT_CtrlPointListenerOnDeviceAddedIterator
{
public:
    PLT_CtrlPointListenerOnDeviceAddedIterator(PLT_DeviceDataReference& device) :
        m_Device(device) {}

    NPT_Result operator()(PLT_CtrlPointListener*& listener) const {
        return listener->OnDeviceAdded(m_Device);
    }

private:
    PLT_DeviceDataReference& m_Device;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointListenerOnDeviceRemovedIterator class
+---------------------------------------------------------------------*/
class PLT_CtrlPointListenerOnDeviceRemovedIterator
{
public:
    PLT_CtrlPointListenerOnDeviceRemovedIterator(PLT_DeviceDataReference& device) :
        m_Device(device) {}

    NPT_Result operator()(PLT_CtrlPointListener*& listener) const {
        return listener->OnDeviceRemoved(m_Device);
    }

private:
    PLT_DeviceDataReference& m_Device;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointListenerOnActionResponseIterator class
+---------------------------------------------------------------------*/
class PLT_CtrlPointListenerOnActionResponseIterator
{
public:
    PLT_CtrlPointListenerOnActionResponseIterator(NPT_Result           res, 
                                                  PLT_ActionReference& action, 
                                                  void*                userdata) :
        m_Res(res), m_Action(action), m_Userdata(userdata) {}

    NPT_Result operator()(PLT_CtrlPointListener*& listener) const {
        return listener->OnActionResponse(m_Res, m_Action, m_Userdata);
    }

private:
    NPT_Result           m_Res;
    PLT_ActionReference& m_Action;
    void*                m_Userdata;
};

/*----------------------------------------------------------------------
|   PLT_CtrlPointListenerOnEventNotifyIterator class
+---------------------------------------------------------------------*/
class PLT_CtrlPointListenerOnEventNotifyIterator
{
public:
    PLT_CtrlPointListenerOnEventNotifyIterator(PLT_Service*                  service, 
                                               NPT_List<PLT_StateVariable*>* vars) :
        m_Service(service), m_Vars(vars) {}

    NPT_Result operator()(PLT_CtrlPointListener*& listener) const {
        return listener->OnEventNotify(m_Service, m_Vars);
    }

private:
    PLT_Service*                  m_Service;
    NPT_List<PLT_StateVariable*>* m_Vars;
};

/*----------------------------------------------------------------------
|   PLT_AddGetSCPDRequestIterator class
+---------------------------------------------------------------------*/
class PLT_AddGetSCPDRequestIterator
{
public:
    PLT_AddGetSCPDRequestIterator(PLT_TaskManager*         task_manager, 
                                  PLT_CtrlPoint*           ctrl_point, 
                                  PLT_DeviceDataReference& device) :
        m_TaskManager(task_manager), 
        m_CtrlPoint(ctrl_point), 
        m_Device(device) {}

    NPT_Result operator()(PLT_Service*& service) const {
        // look for the host and port of the device
        PLT_DeviceData* device = service->GetDevice();
        NPT_String scpd_url    = service->GetSCPDURL();

        NPT_LOG_INFO_2("Fetching SCPD for service \"%s\" of device \"%s\"", 
            (const char*)service->GetServiceID(),
            (const char*)device->GetFriendlyName());

        // if the SCPD Url starts with a "/", this means we should not append it to the base URI
        // but instead go there directly
        if (!scpd_url.StartsWith("/")) {
            scpd_url = device->GetURLBase().GetPath() + scpd_url;
        }

        NPT_HttpUrl url(device->GetURLBase().GetHost(), 
                        device->GetURLBase().GetPort(), 
                        scpd_url);
        
        // Add a delay, some devices need it (aka Rhapsody)
        NPT_TimeInterval delay(0.1f);
        return m_TaskManager->StartTask(
            new PLT_CtrlPointGetSCPDTask(url, 
                                         m_CtrlPoint, 
                                         (PLT_DeviceDataReference&)m_Device),
            &delay);
    }

private:
    PLT_TaskManager*        m_TaskManager;
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_DeviceDataReference m_Device;
};

/*----------------------------------------------------------------------
|   PLT_EventSubscriberRemoverIterator class
+---------------------------------------------------------------------*/
class PLT_EventSubscriberRemoverIterator
{
public:
    PLT_EventSubscriberRemoverIterator(PLT_CtrlPoint* ctrl_point) : 
        m_CtrlPoint(ctrl_point) { 
        m_CtrlPoint->m_Subscribers.Lock();
    }
    ~PLT_EventSubscriberRemoverIterator() {
        m_CtrlPoint->m_Subscribers.Unlock();
    }

    NPT_Result operator()(PLT_Service*& service) const {
        PLT_EventSubscriber* sub = NULL;
        if (NPT_SUCCEEDED(NPT_ContainerFind(m_CtrlPoint->m_Subscribers, 
                                            PLT_EventSubscriberFinderByService(service), sub))) {
            m_CtrlPoint->m_Subscribers.Remove(sub);
            delete sub;
        }

        return NPT_SUCCESS;
    }

private:
    PLT_CtrlPoint* m_CtrlPoint;
};

/*----------------------------------------------------------------------
|   PLT_ServiceReadyIterator class
+---------------------------------------------------------------------*/
class PLT_ServiceReadyIterator
{
public:
    PLT_ServiceReadyIterator() {}

    NPT_Result operator()(PLT_Service*& service) const {
        return service->IsInitted()?NPT_SUCCESS:NPT_FAILURE;
    }
};

/*----------------------------------------------------------------------
|   PLT_DeviceReadyIterator class
+---------------------------------------------------------------------*/
class PLT_DeviceReadyIterator
{
public:
    PLT_DeviceReadyIterator() {}
    NPT_Result operator()(PLT_DeviceDataReference& device) const {
        NPT_CHECK(device->m_Services.ApplyUntil(
            PLT_ServiceReadyIterator(), 
            NPT_UntilResultNotEquals(NPT_SUCCESS)));

        NPT_CHECK(device->m_EmbeddedDevices.ApplyUntil(
            PLT_DeviceReadyIterator(), 
            NPT_UntilResultNotEquals(NPT_SUCCESS)));

        return NPT_SUCCESS;
    }
};

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::PLT_CtrlPoint
+---------------------------------------------------------------------*/
PLT_CtrlPoint::PLT_CtrlPoint(const char* autosearch /* = "upnp:rootdevice" */) :
    m_EventHttpServer(new PLT_HttpServer()),
    m_AutoSearch(autosearch)
{
    m_EventHttpServerHandler = new PLT_HttpCtrlPointRequestHandler(this);
    m_EventHttpServer->AddRequestHandler(m_EventHttpServerHandler, "/", true);
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::~PLT_CtrlPoint
+---------------------------------------------------------------------*/
PLT_CtrlPoint::~PLT_CtrlPoint()
{
    delete m_EventHttpServer;
    delete m_EventHttpServerHandler; 
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::IgnoreUUID
+---------------------------------------------------------------------*/
void
PLT_CtrlPoint::IgnoreUUID(const char* uuid)
{
    if (!m_UUIDsToIgnore.Find(NPT_StringFinder(uuid))) {
        m_UUIDsToIgnore.Add(uuid);
    }
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Start(PLT_SsdpListenTask* task)
{
    m_EventHttpServer->Start();

    // house keeping task
    m_TaskManager.StartTask(new PLT_CtrlPointHouseKeepingTask(this));

    task->AddListener(this);
    return m_AutoSearch.GetLength()?Search(NPT_HttpUrl("239.255.255.250", 1900, "*"), m_AutoSearch):NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Stop(PLT_SsdpListenTask* task)
{
    task->RemoveListener(this);

    m_TaskManager.StopAllTasks();
    m_EventHttpServer->Stop();

    // we can safely clear everything without a lock
    // as there are no more tasks pending
    m_Devices.Clear();

    m_Subscribers.Apply(NPT_ObjectDeleter<PLT_EventSubscriber>());
    m_Subscribers.Clear();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::AddListener
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::AddListener(PLT_CtrlPointListener* listener) 
{
    NPT_AutoLock lock(m_ListenerList);
    if (!m_ListenerList.Contains(listener)) {
        m_ListenerList.Add(listener);
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::RemoveListener
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::RemoveListener(PLT_CtrlPointListener* listener)
{
    NPT_AutoLock lock(m_ListenerList);
    m_ListenerList.Remove(listener);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::CreateSearchTask
+---------------------------------------------------------------------*/
PLT_SsdpSearchTask*
PLT_CtrlPoint::CreateSearchTask(const NPT_HttpUrl&   url, 
                                const char*          target, 
                                NPT_Cardinal         mx, 
                                const NPT_IpAddress& address)
{
    // make sure mx is at least 1
    if (mx<1) mx=1;

    // create socket
    NPT_UdpMulticastSocket* socket = new NPT_UdpMulticastSocket();
    socket->SetInterface(address);
    socket->SetTimeToLive(4);

    // create request
    NPT_HttpRequest* request = new NPT_HttpRequest(url, "M-SEARCH", NPT_HTTP_PROTOCOL_1_1);
    PLT_UPnPMessageHelper::SetMX(*request, mx);
    PLT_UPnPMessageHelper::SetST(*request, target);
    PLT_UPnPMessageHelper::SetMAN(*request, "\"ssdp:discover\"");

    // create task
    PLT_SsdpSearchTask* task = new PLT_SsdpSearchTask(
        socket,
        this, 
        request,
        mx*10000);
    return task;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Search
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Search(const NPT_HttpUrl& url, 
                      const char*        target, 
                      NPT_Cardinal       mx /* = 5 */)
{
    NPT_List<NPT_NetworkInterface*> if_list;
    NPT_List<NPT_NetworkInterface*>::Iterator net_if;
    NPT_List<NPT_NetworkInterfaceAddress>::Iterator net_if_addr;

    NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetNetworkInterfaces(if_list));

    for (net_if = if_list.GetFirstItem(); net_if; net_if++) {
        // make sure the interface is at least broadcast or multicast
        if (!((*net_if)->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_MULTICAST) &&
            !((*net_if)->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_BROADCAST)) {
            continue;
        }       
            
        for (net_if_addr = (*net_if)->GetAddresses().GetFirstItem(); net_if_addr; net_if_addr++) {
            // create task
            PLT_SsdpSearchTask* task = CreateSearchTask(url, 
                target, 
                mx, 
                (*net_if_addr).GetPrimaryAddress());
            m_TaskManager.StartTask(task);
        }
    }

//     {
//         // create task on 127.0.0.1
//         NPT_IpAddress address;
//         address.ResolveName("127.0.0.1");
// 
//         PLT_ThreadTask* task = CreateSearchTask(url, 
//             target, 
//             mx, 
//             address);
//         m_TaskManager.StartTask(task);
//     }

    if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Discover
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Discover(const NPT_HttpUrl& url, 
                        const char*        target, 
                        NPT_Cardinal       mx /* = 5 */,
                        NPT_Timeout        repeat /* = 50000 */)
{
    // make sure mx is at least 1
    if (mx<1) mx = 1;

    // create socket
    NPT_UdpSocket* socket = new NPT_UdpSocket();

    // create request
    NPT_HttpRequest* request = new NPT_HttpRequest(url, "M-SEARCH", NPT_HTTP_PROTOCOL_1_1);
    PLT_UPnPMessageHelper::SetMX(*request, mx);
    PLT_UPnPMessageHelper::SetST(*request, target);
    PLT_UPnPMessageHelper::SetMAN(*request, "\"ssdp:discover\"");

    // force HOST to be the regular multicast address:port
    // Some servers do care (like WMC) otherwise they won't respond to us
    request->GetHeaders().SetHeader(NPT_HTTP_HEADER_HOST, "239.255.255.250:1900");

    // create task
    PLT_ThreadTask* task = new PLT_SsdpSearchTask(
        socket,
        this, 
        request,
        repeat<(NPT_Timeout)mx*5000?(NPT_Timeout)mx*5000:repeat);  /* repeat no less than every 5 secs */
    return m_TaskManager.StartTask(task);
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::DoHouseKeeping
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::DoHouseKeeping()
{
    NPT_AutoLock lock_devices(m_Devices);
    NPT_TimeStamp now;
    int count = m_Devices.GetItemCount();
    NPT_System::GetCurrentTimeStamp(now);

    PLT_DeviceDataReference device;
    while (count--) {
        NPT_Result res = m_Devices.PopHead(device);
        if (NPT_SUCCEEDED(res)) {
            NPT_TimeStamp last_update = device->GetLeaseTimeLastUpdate();
            NPT_TimeInterval lease_time = device->GetLeaseTime();

            // check if device lease time has expired
            if (now > last_update + NPT_TimeInterval((unsigned long)(((float)lease_time)*2), 0)) {
                RemoveDevice(device);
            } else {
                // add the device back to our list since it is still alive
                m_Devices.Add(device);
            }
        } else {
            NPT_LOG_SEVERE("DoHouseKeeping failure!");
            return NPT_FAILURE;
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::FindDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::FindDevice(const char*              uuid, 
                          PLT_DeviceDataReference& device) 
{
    NPT_AutoLock lock(m_Devices);
    return NPT_ContainerFind(m_Devices, PLT_DeviceDataFinder(uuid), device);
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessHttpRequest(NPT_HttpRequest&              request,
                                  const NPT_HttpRequestContext& context,
                                  NPT_HttpResponse&             response)
{
    NPT_COMPILER_UNUSED(context);
    if (!request.GetMethod().Compare("NOTIFY")) {
        return ProcessHttpNotify(request, context, response);
    }

    NPT_LOG_SEVERE("CtrlPoint received bad http request\r\n");
    response.SetStatus(412, "Precondition Failed");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessHttpNotify
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessHttpNotify(NPT_HttpRequest&              request,
                                 const NPT_HttpRequestContext& context,
                                 NPT_HttpResponse&             response)
{
    NPT_COMPILER_UNUSED(context);

    NPT_List<PLT_StateVariable*> vars;
    PLT_EventSubscriber*         sub = NULL;    
    NPT_String                   str;
    NPT_XmlElementNode*          xml = NULL;
    NPT_String                   callback_uri;
    NPT_String                   uuid;
    NPT_String                   service_id;
    NPT_UInt32                   seq = 0;
    PLT_Service*                 service = NULL;
    PLT_DeviceData*              device = NULL;
    NPT_String                   content_type;

    NPT_String method   = request.GetMethod();
    NPT_String uri      = request.GetUrl().GetPath();

//     NPT_LOG_FINE_3("CtrlPoint received %s request from %s:%d\r\n", 
//         request.GetMethod(), 
//         client_info.remote_address.GetIpAddress(), 
//         client_info.remote_address.GetPort());
// 
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, &request);

    const NPT_String* sid = PLT_UPnPMessageHelper::GetSID(request);
    const NPT_String* nt  = PLT_UPnPMessageHelper::GetNT(request);
    const NPT_String* nts = PLT_UPnPMessageHelper::GetNTS(request);
    PLT_HttpHelper::GetContentType(request, content_type);

    {
        NPT_AutoLock lock_subs(m_Subscribers);

        // look for the subscriber with that subscription url
        if (!sid || NPT_FAILED(NPT_ContainerFind(m_Subscribers, 
                                                 PLT_EventSubscriberFinderBySID(*sid), 
                                                 sub))) {
            NPT_LOG_FINE_1("Subscriber %s not found\n", (const char*)sid);
            goto bad_request;
        }

        // verify the request is syntactically correct
        service = sub->GetService();
        device  = service->GetDevice();

        uuid = device->GetUUID();
        service_id = service->GetServiceID();

        // callback uri for this sub
        callback_uri = "/" + uuid + "/" + service_id;

        if (uri.Compare(callback_uri, true) ||
            !nt || nt->Compare("upnp:event", true) || 
            !nts || nts->Compare("upnp:propchange", true)) {
            goto bad_request;
        }

        // if the sequence number is less than our current one, we got it out of order
        // so we disregard it
        PLT_UPnPMessageHelper::GetSeq(request, seq);
        if (sub->GetEventKey() && seq <= sub->GetEventKey()) {
            goto bad_request;
        }

        // parse body
        if (NPT_FAILED(PLT_HttpHelper::ParseBody(request, xml))) {
            goto bad_request;
        }

        // check envelope
        if (xml->GetTag().Compare("propertyset", true))
            goto bad_request;

        // check namespace
    //    xml.GetAttrValue("xmlns:e", str);
    //    if (str.Compare("urn:schemas-upnp-org:event-1-0"))
    //        goto bad_request;

        // check property set
        // keep a vector of the state variables that changed
        NPT_XmlElementNode* property;
        PLT_StateVariable*  var;
        for (NPT_List<NPT_XmlNode*>::Iterator children = xml->GetChildren().GetFirstItem(); children; children++) {
            NPT_XmlElementNode* child = (*children)->AsElementNode();
            if (!child) continue;

            // check property
            if (child->GetTag().Compare("property", true))
                goto bad_request;

            if (NPT_FAILED(PLT_XmlHelper::GetChild(child, property))) {
                goto bad_request;
            }

            var = service->FindStateVariable(property->GetTag());
            if (var == NULL) {
                goto bad_request;
            }

            if (NPT_FAILED(var->SetValue(property->GetText()?*property->GetText():""))) {
                goto bad_request;
            }
            vars.Add(var);
        }    

        // update sequence
        sub->SetEventKey(seq);
    }

    // notify listener we got an update
    if (vars.GetItemCount()) {
        NPT_AutoLock lock(m_ListenerList);
        m_ListenerList.Apply(PLT_CtrlPointListenerOnEventNotifyIterator(service, &vars));
    }

    delete xml;
    return NPT_SUCCESS;

bad_request:
    NPT_LOG_SEVERE("CtrlPoint received bad request\r\n");
    response.SetStatus(412, "Precondition Failed");
    delete xml;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessSsdpSearchResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessSsdpSearchResponse(NPT_Result                    res, 
                                         const NPT_HttpRequestContext& context, 
                                         NPT_HttpResponse*             response)
{
    NPT_CHECK_SEVERE(res);
    NPT_CHECK_POINTER_SEVERE(response);

    NPT_String ip_address = context.GetRemoteAddress().GetIpAddress().ToString();
    NPT_String protocol   = response->GetProtocol();
    
    NPT_LOG_FINE_2("CtrlPoint received SSDP search response from %s:%d",
        (const char*)context.GetRemoteAddress().GetIpAddress().ToString() , 
        context.GetRemoteAddress().GetPort());
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);
    
    if (response->GetStatusCode() == 200) {
        const NPT_String* st  = response->GetHeaders().GetHeaderValue("st");
        const NPT_String* usn = response->GetHeaders().GetHeaderValue("usn");
        const NPT_String* ext = response->GetHeaders().GetHeaderValue("ext");
        NPT_CHECK_POINTER_SEVERE(st);
        NPT_CHECK_POINTER_SEVERE(usn);
        NPT_CHECK_POINTER_SEVERE(ext);
        
        NPT_String uuid;
        // if we get an advertisement other than uuid
        // verify it's formatted properly
        if (usn != st) {
            char tmp_uuid[200];
            char tmp_st[200];
            int  ret;
            // FIXME: We can't use sscanf directly!
            ret = sscanf(((const char*)*usn)+5, "%[^::]::%s",
                tmp_uuid, 
                tmp_st);
            if (ret != 2)
                return NPT_FAILURE;
            
            if (st->Compare(tmp_st, true))
                return NPT_FAILURE;
            
            uuid = tmp_uuid;
        } else {
            uuid = ((const char*)*usn)+5;
        }
        
        if (m_UUIDsToIgnore.Find(NPT_StringFinder(uuid))) {
            NPT_LOG_FINE_1("CtrlPoint received a search response from ourselves (%s)\n", (const char*)uuid);
            return NPT_SUCCESS;
        }

        return ProcessSsdpMessage(response, context, uuid);    
    }
    
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::OnSsdpPacket
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::OnSsdpPacket(NPT_HttpRequest&              request,
                            const NPT_HttpRequestContext& context)
{
    return ProcessSsdpNotify(request, context);
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessSsdpNotify
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessSsdpNotify(NPT_HttpRequest&              request, 
                                 const NPT_HttpRequestContext& context)
{
    // get the address of who sent us some data back
    NPT_String ip_address = context.GetRemoteAddress().GetIpAddress().ToString();
    NPT_String method     = request.GetMethod();
    NPT_String uri        = (const char*)request.GetUrl().GetPath();
    NPT_String protocol   = request.GetProtocol();

    if (method.Compare("NOTIFY") == 0) {
        NPT_LOG_FINE_2("CtrlPoint received SSDP Notify from %s:%d",
            context.GetRemoteAddress().GetIpAddress().ToString().GetChars(), 
            context.GetRemoteAddress().GetPort());
        PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);

        if ((uri.Compare("*") != 0) || (protocol.Compare("HTTP/1.1") != 0))
            return NPT_FAILURE;
        
        const NPT_String* nts = PLT_UPnPMessageHelper::GetNTS(request);
        const NPT_String* nt  = PLT_UPnPMessageHelper::GetNT(request);
        const NPT_String* usn = PLT_UPnPMessageHelper::GetUSN(request);
        NPT_CHECK_POINTER_SEVERE(nts);
        NPT_CHECK_POINTER_SEVERE(nt);
        NPT_CHECK_POINTER_SEVERE(usn);

        NPT_String uuid;
        // if we get an advertisement other than uuid
        // verify it's formatted properly
        if (*usn != *nt) {
            char tmp_uuid[200];
            char tmp_nt[200];
            int  ret;
            //FIXME: no sscanf!
            ret = sscanf(((const char*)*usn)+5, "%[^::]::%s",
                tmp_uuid, 
                tmp_nt);
            if (ret != 2)
                return NPT_FAILURE;
            
            if (nt->Compare(tmp_nt, true))
                return NPT_FAILURE;
            
            uuid = tmp_uuid;
        } else {
            uuid = ((const char*)*usn)+5;
        }

        if (m_UUIDsToIgnore.Find(NPT_StringFinder(uuid))) {
            NPT_LOG_FINE_1("CtrlPoint received a Notify request from ourselves (%s)\n", (const char*)uuid);
            return NPT_SUCCESS;
        }

        NPT_LOG_FINE_1("CtrlPoint received a Notify request from %s\n", (const char*)uuid);
        
        // if it's a byebye, remove the device and return right away
        if (nts->Compare("ssdp:byebye", true) == 0) {
            PLT_DeviceDataReference data;
            if (NPT_SUCCEEDED(FindDevice(uuid, data))) {
                NPT_AutoLock lock_devices(m_Devices);
                RemoveDevice(data);
            }

            return NPT_SUCCESS;
        }
        
        return ProcessSsdpMessage(&request, context, uuid);
    }
    
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::RemoveDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::RemoveDevice(PLT_DeviceDataReference& data)
{
    /* recursively remove embedded devices */
    NPT_Array<PLT_DeviceDataReference> embedded_devices = 
        data->GetEmbeddedDevices();
    for(NPT_Cardinal i=0;i>embedded_devices.GetItemCount();i++) {
        RemoveDevice(embedded_devices[i]);
    }

    /* remove from list */
    m_Devices.Remove(data);

    /* unsubscribe from services */
    data->m_Services.Apply(PLT_EventSubscriberRemoverIterator(this));

    /* notify listeners */
    {
        NPT_AutoLock lock(m_ListenerList);
        m_ListenerList.Apply(PLT_CtrlPointListenerOnDeviceRemovedIterator(data));
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessSsdpMessage
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessSsdpMessage(NPT_HttpMessage*              message, 
                                  const NPT_HttpRequestContext& context,
                                  NPT_String&                   uuid)
{
    NPT_COMPILER_UNUSED(context);
    NPT_CHECK_POINTER_SEVERE(message);

    if (m_UUIDsToIgnore.Find(NPT_StringFinder(uuid))) return NPT_SUCCESS;

    const NPT_String* location = PLT_UPnPMessageHelper::GetLocation(*message);
    NPT_CHECK_POINTER_SEVERE(location);
    
    // be nice and assume a default lease time if not found
    NPT_Timeout leasetime;
    if (NPT_FAILED(PLT_UPnPMessageHelper::GetLeaseTime(*message, leasetime))) {
        leasetime = 1800;
    }

    return InspectDevice(*location, uuid, leasetime);
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::InspectDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::InspectDevice(const char* location, 
                             const char* uuid, 
                             NPT_Timeout leasetime)
{
    NPT_HttpUrl url(location);
    if (!url.IsValid()) return NPT_FAILURE;

    // is it a new device?
    PLT_DeviceDataReference data;
    if (NPT_FAILED(FindDevice(uuid, data))) {
        NPT_AutoLock lock(m_Devices);

        NPT_LOG_INFO_2("New device \"%s\" detected @ %s", uuid, location);

        data = new PLT_DeviceData(url, uuid, NPT_TimeInterval(leasetime, 0));
        m_Devices.Add(data);
        
        // Start a task to retrieve the description
        PLT_CtrlPointGetDescriptionTask* task = new PLT_CtrlPointGetDescriptionTask(
            url,
            this, 
            data);
        // Add a delay, some devices need it (aka Rhapsody)
        NPT_TimeInterval delay(0.2f);
        m_TaskManager.StartTask(task, &delay);

        return NPT_SUCCESS;
    }
    
    // in case we missed the byebye and the device description has changed (ip or port)
    // reset base and assumes device is the same (same number of services and SCPDs)
    // FIXME: The right way is to remove the device and rescan it though
    if (data->GetDescriptionUrl().Compare(location, true)) {
        NPT_LOG_INFO_2("Old device \"%s\" detected @ new location %s", 
            (const char*)data->GetFriendlyName(), 
            location);
        data->SetURLBase(url);
    }

    // renew expiration time
    data->SetLeaseTime(NPT_TimeInterval(leasetime, 0));
    NPT_LOG_FINE_1("Device (%s) expiration time renewed..", (const char*)uuid);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessGetDescriptionResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessGetDescriptionResponse(NPT_Result                    res, 
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse*             response, 
                                             PLT_DeviceDataReference&      device)
{    
    NPT_LOG_INFO_2("Received device description for %s (result = %d)", 
        (const char*)device->GetUUID(), 
        res);

    NPT_CHECK_FATAL(res);
    NPT_CHECK_POINTER_FATAL(response);

    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);

    // make sure we have seen this device before
    PLT_DeviceDataReference root_device;
    res = FindDevice(device->GetUUID(), root_device);
    NPT_CHECK_WARNING(res);

    // get body
    NPT_String desc;
    res = PLT_HttpHelper::GetBody(*response, desc);
    NPT_CHECK_LABEL_FATAL(res, bad_response);
    
    // set the device description
    res = root_device->SetDescription(desc, context.GetLocalAddress().GetIpAddress());
    NPT_CHECK_LABEL_FATAL(res, bad_response);

    NPT_LOG_INFO_2("Device \"%s\" is now known as \"%s\"", 
        (const char*)device->GetUUID(), 
        (const char*)device->GetFriendlyName());

    // add embedded devices to list of devices
    // and fetch their services scpd
    for (NPT_Cardinal i = 0;
         i<root_device->m_EmbeddedDevices.GetItemCount();
         i++) {

         PLT_DeviceDataReference embedded_device = root_device->m_EmbeddedDevices[i];
         PLT_DeviceDataReference data;
         if (NPT_FAILED(FindDevice(embedded_device->GetUUID(), data))) {
             NPT_AutoLock lock(m_Devices);
             m_Devices.Add(embedded_device);
         }

         res = embedded_device->m_Services.Apply(PLT_AddGetSCPDRequestIterator(
             &m_TaskManager, 
             this, 
             embedded_device));
         NPT_CHECK_LABEL_FATAL(res, bad_response);
    }

    // Get SCPD of root device services now
    res = root_device->m_Services.Apply(PLT_AddGetSCPDRequestIterator(
        &m_TaskManager, 
        this, 
        root_device));
    NPT_CHECK_LABEL_FATAL(res, bad_response);

    return NPT_SUCCESS;

bad_response:
    NPT_LOG_SEVERE_2("Bad Description response for device \"%s\": %s", 
        (const char*)device->GetUUID(),
        (const char*)desc);

    if (!root_device.IsNull()) {
        NPT_AutoLock lock(m_Devices);    
        RemoveDevice(root_device);
    }
    return res;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessGetSCPDResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessGetSCPDResponse(NPT_Result               res, 
                                      NPT_HttpRequest*         request,
                                      NPT_HttpResponse*        response,
                                      PLT_DeviceDataReference& device)
{
    PLT_DeviceReadyIterator device_tester;   
    PLT_Service*            service = NULL;
    
    NPT_LOG_INFO_2("Received SCPD response for %s (result = %d)", 
        (const char*)device->GetUUID(), 
        res);

    NPT_CHECK_FATAL(res);
    NPT_CHECK_POINTER_FATAL(request);
    NPT_CHECK_POINTER_FATAL(response);
    
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);

    PLT_DeviceDataReference data;
    NPT_CHECK_WARNING(FindDevice(device->GetUUID(), data));

    // get body
    NPT_String scpd;
    res = PLT_HttpHelper::GetBody(*response, scpd);
    NPT_CHECK_LABEL_FATAL(res, bad_response);

    // look for the service based on the SCPD uri
    res = data->FindServiceByDescriptionURI(request->GetUrl().GetPath(), service);
    NPT_CHECK_LABEL_FATAL(res, bad_response);
    
    {
        // lock using listener list before testing
        // to make sure an scpd is not getting set while we test
        NPT_AutoLock lock(m_ListenerList);

        // set the service scpd
        res = service->SetSCPDXML(scpd);
        NPT_CHECK_LABEL_FATAL(res, bad_response);

        if (NPT_SUCCEEDED(device_tester(data))) {
            // notify that the device is ready to use
            m_ListenerList.Apply(PLT_CtrlPointListenerOnDeviceAddedIterator(data));
        }

        // if device is not root, notify listeners now 
        // if root is ready
        if (!data->GetParentUUID().IsEmpty()) {
            PLT_DeviceDataReference parent;
            NPT_CHECK_WARNING(FindDevice(data->GetParentUUID(), parent));

            // lock using listener list before testing
            // to make sure an scpd is not getting set while we test
            if (NPT_SUCCEEDED(device_tester(parent))) {
                // notify that the root device is ready to use
                m_ListenerList.Apply(PLT_CtrlPointListenerOnDeviceAddedIterator(parent));
            }
        }
    }
    
    return NPT_SUCCESS;

bad_response:
    NPT_LOG_SEVERE_2("Bad SCPD response for device \"%s\":%s", 
        (const char*)device->GetFriendlyName(),
        (const char*)scpd);

    if (!data.IsNull()) {
        NPT_AutoLock lock(m_Devices);
        RemoveDevice(data);
    }
    return res;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Subscribe
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Subscribe(PLT_Service* service, bool cancel, void* userdata)
{
    NPT_AutoLock lock(m_Subscribers);

    if (!service->IsSubscribable()) return NPT_FAILURE;

    // look for the host and port of the device
    PLT_DeviceData* device = service->GetDevice();

    // get the relative event subscription url
    // if URL starts with '/', it's not to be appended to base URL
    NPT_String event_sub_url = service->GetEventSubURL();
    if (!event_sub_url.StartsWith("/")) {
        event_sub_url = device->GetURLBase().GetPath() + event_sub_url;
    }

    NPT_HttpUrl url(device->GetURLBase().GetHost(), 
                    device->GetURLBase().GetPort(), 
                    event_sub_url);

    // look for the subscriber with that service to decide if it's a renewal or not
    PLT_EventSubscriber* sub = NULL;
    NPT_ContainerFind(m_Subscribers, 
                      PLT_EventSubscriberFinderByService(service), 
                      sub);

    // create the request
    NPT_HttpRequest* request = NULL;

    if (cancel == false) {
        // renewal?
        if (sub) {
            NPT_LOG_FINE_3("Renewing subscriber \"%s\" for service \"%s\" of device \"%s\"", 
                (const char*)sub->GetSID(),
                (const char*)service->GetServiceID(),
                (const char*)device->GetFriendlyName());

            // create the request
            request = new NPT_HttpRequest(url, "SUBSCRIBE");

            PLT_UPnPMessageHelper::SetSID(*request, sub->GetSID());
            PLT_UPnPMessageHelper::SetTimeOut(*request, 1800);
        } else {
            NPT_LOG_INFO_2("Subscribing to service \"%s\" of device \"%s\"",
                (const char*)service->GetServiceID(),
                (const char*)service->GetDevice()->GetFriendlyName());

            // prepare the callback url
            NPT_String uuid         = device->GetUUID();
            NPT_String service_id   = service->GetServiceID();
            NPT_String callback_uri = "/" + uuid + "/" + service_id;

            // create the request
            request = new NPT_HttpRequest(url, "SUBSCRIBE");
            // specify callback url using ip of interface used when 
            // retrieving device description
            NPT_HttpUrl callbackUrl(device->m_LocalIfaceIp.ToString(), 
                m_EventHttpServer->GetPort(), 
                callback_uri);

            // set the required headers for a new subscription
            PLT_UPnPMessageHelper::SetNT(*request, "upnp:event");
            PLT_UPnPMessageHelper::SetCallbacks(*request, 
                "<" + callbackUrl.ToString() + ">");
            PLT_UPnPMessageHelper::SetTimeOut(*request, 1800);
        }
    } else {
        NPT_LOG_INFO_3("Unsubscribing subscriber \"%s\" for service \"%s\" of device \"%s\"",
            (const char*)(sub?sub->GetSID().GetChars():"unknown"),
            (const char*)service->GetServiceID(),
            (const char*)service->GetDevice()->GetFriendlyName());        
        
        // cancellation
        if (!sub)return NPT_FAILURE;

        // create the request
        request = new NPT_HttpRequest(url, "UNSUBSCRIBE");
        PLT_UPnPMessageHelper::SetSID(*request, sub->GetSID());

        // remove from list now
        m_Subscribers.Remove(sub, true);
        delete sub;
    }

    NPT_CHECK_POINTER_FATAL(request);

    // Prepare the request
    // create a task to post the request
    PLT_ThreadTask* task = new PLT_CtrlPointSubscribeEventTask(
        request,
        this, 
        service, 
        userdata);
    m_TaskManager.StartTask(task);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessSubscribeResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessSubscribeResponse(NPT_Result        res, 
                                        NPT_HttpResponse* response,
                                        PLT_Service*      service,
                                        void*             /* userdata */)
{
    const NPT_String*    sid = NULL;
    NPT_Int32            timeout;
    PLT_EventSubscriber* sub = NULL;

    NPT_AutoLock lock(m_Subscribers);

    NPT_LOG_INFO_2("Received subscription response for service \"%s\" (result = %d)", 
        (const char*)service->GetServiceID(),
        res);
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);

    // if there's a failure or it's a response to a cancellation
    // we get out
    if (NPT_FAILED(res) || response == NULL || response->GetStatusCode() != 200) {
        NPT_CHECK_LABEL_SEVERE(NPT_FAILED(res)?res:NPT_FAILURE, failure);
    }
        
    if (!(sid = PLT_UPnPMessageHelper::GetSID(*response)) || 
        NPT_FAILED(PLT_UPnPMessageHelper::GetTimeOut(*response, timeout))) {
        NPT_CHECK_LABEL_SEVERE(NPT_ERROR_INVALID_SYNTAX, failure);
    }

    // look for the subscriber with that sid
    if (NPT_FAILED(NPT_ContainerFind(m_Subscribers, 
                                     PLT_EventSubscriberFinderBySID(*sid), 
                                     sub))) {
        NPT_LOG_INFO_3("Creating new subscriber \"%s\" for service \"%s\" of device \"%s\"",
            (const char*)*sid,
            (const char*)service->GetServiceID(),
            (const char*)service->GetDevice()->GetFriendlyName());

        sub = new PLT_EventSubscriber(&m_TaskManager, service, *sid);
        m_Subscribers.Add(sub);
    }

    sub->SetTimeout(timeout);
    return NPT_SUCCESS;

failure:
    NPT_LOG_SEVERE_3("(un)subscription failed of sub \"%s\" for service \"%s\" of device \"%s\"", 
        (const char*)(sid?*sid:"?"),
        (const char*)service->GetServiceID(),
        (const char*)service->GetDevice()->GetFriendlyName());

    // in case it was a renewal look for the subscriber with that service and remove it from the list
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, 
                                        PLT_EventSubscriberFinderByService(service), 
                                        sub))) {
        m_Subscribers.Remove(sub);
        delete sub;
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::InvokeAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::InvokeAction(PLT_ActionReference& action, 
                            void*                userdata)
{
    PLT_Service*    service = action->GetActionDesc()->GetService();
    PLT_DeviceData* device  = service->GetDevice();

    // look for the service control url
    NPT_String control_url = service->GetControlURL();

    // if URL starts with a "/", it's not to be appended to base URL
    if (!control_url.StartsWith("/")) {
        control_url = device->GetURLBase().GetPath() + control_url;
    }

    // create the request
    // FIXME: hack use HTTP/1.0 for now because of WMC that returning 100 Continue when using HTTP/1.1
    // and this screws up the http processing right now
    NPT_HttpUrl url(device->GetURLBase().GetHost(), device->GetURLBase().GetPort(), control_url);
    NPT_HttpRequest* request = new NPT_HttpRequest(url, "POST", NPT_HTTP_PROTOCOL_1_0);
    
    // create a memory stream for our request body
    NPT_MemoryStreamReference stream(new NPT_MemoryStream);
    action->FormatSoapRequest(*stream);

    // set the request body
    NPT_InputStreamReference input = stream;
    PLT_HttpHelper::SetBody(*request, input);

    PLT_HttpHelper::SetContentType(*request, "text/xml; charset=\"utf-8\"");
    NPT_String service_type = service->GetServiceType();
    NPT_String action_name   = action->GetActionDesc()->GetName();
    request->GetHeaders().SetHeader("SOAPAction", "\"" + service_type + "#" + action_name + "\"");

    // create a task to post the request
    PLT_CtrlPointInvokeActionTask* task = new PLT_CtrlPointInvokeActionTask(
        request,
        this, 
        action, 
        userdata);

    // queue the request
    m_TaskManager.StartTask(task);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessActionResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessActionResponse(NPT_Result           res, 
                                     NPT_HttpResponse*    response,
                                     PLT_ActionReference& action,
                                     void*                userdata)
{
    NPT_String          service_type;
    NPT_String          str;
    NPT_XmlElementNode* xml = NULL;
    NPT_String          name;
    NPT_String          soap_action_name;
    NPT_XmlElementNode* soap_action_response;
    NPT_XmlElementNode* soap_body;
    NPT_XmlElementNode* fault;
    const NPT_String*   attr = NULL;
    PLT_ActionDesc*     action_desc = action->GetActionDesc();

    // reset the error code and desc
    action->SetError(0, "");

    // check context validity
    if (NPT_FAILED(res) || response == NULL) {
        goto failure;
    }

    NPT_LOG_FINE("Received Action Response:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);

    NPT_LOG_FINER("Reading/Parsing Action Response Body...");
    if (NPT_FAILED(PLT_HttpHelper::ParseBody(*response, xml))) {
        goto failure;
    }

    NPT_LOG_FINER("Analyzing Action Response Body...");

    // read envelope
    if (xml->GetTag().Compare("Envelope", true))
        goto failure;

    // check namespace
    if (!xml->GetNamespace() || xml->GetNamespace()->Compare("http://schemas.xmlsoap.org/soap/envelope/"))
        goto failure;

    // check encoding
    attr = xml->GetAttribute("encodingStyle", "http://schemas.xmlsoap.org/soap/envelope/");
    if (!attr || attr->Compare("http://schemas.xmlsoap.org/soap/encoding/"))
        goto failure;

    // read action
    soap_body = PLT_XmlHelper::GetChild(xml, "Body");
    if (soap_body == NULL)
        goto failure;

    // check if an error occurred
    fault = PLT_XmlHelper::GetChild(soap_body, "Fault");
    if (fault != NULL) {
        // we have an error
        ParseFault(action, fault);
        goto failure;
    }

    if (NPT_FAILED(PLT_XmlHelper::GetChild(soap_body, soap_action_response)))
        goto failure;

    // verify action name is identical to SOAPACTION header
    if (soap_action_response->GetTag().Compare(action_desc->GetName() + "Response", true))
        goto failure;

    // verify namespace
    if (!soap_action_response->GetNamespace() ||
         soap_action_response->GetNamespace()->Compare(action_desc->GetService()->GetServiceType()))
         goto failure;

    // read all the arguments if any
    for (NPT_List<NPT_XmlNode*>::Iterator args = soap_action_response->GetChildren().GetFirstItem(); args; args++) {
        NPT_XmlElementNode* child = (*args)->AsElementNode();
        if (!child) continue;

        action->SetArgumentValue(child->GetTag(), child->GetText()?*child->GetText():"");
        if (NPT_FAILED(res)) goto failure; 
    }

    // create a buffer for our response body and call the service
    res = action->VerifyArguments(false);
    if (NPT_FAILED(res)) goto failure; 

    goto cleanup;

failure:
    // override res with failure if necessary
    if (NPT_SUCCEEDED(res)) res = NPT_ERROR_INVALID_FORMAT;
    // fallthrough

cleanup:
    {
        NPT_AutoLock lock(m_ListenerList);
        m_ListenerList.Apply(PLT_CtrlPointListenerOnActionResponseIterator(res, action, userdata));
    }
    
    delete xml;
    return res;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ParseFault
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ParseFault(PLT_ActionReference& action,
                          NPT_XmlElementNode*  fault)
{
    NPT_XmlElementNode* detail = fault->GetChild("detail");
    if (detail == NULL) return NPT_FAILURE;

    NPT_XmlElementNode *upnp_error, *error_code, *error_desc;
    upnp_error = detail->GetChild("upnp_error");
    if (upnp_error == NULL) return NPT_FAILURE;

    error_code = upnp_error->GetChild("errorCode");
    error_desc = upnp_error->GetChild("errorDescription");
    NPT_Int32  code = 501;    
    NPT_String desc;
    if (error_code && error_code->GetText()) {
        NPT_String value = *error_code->GetText();
        value.ToInteger(code);
    }
    if (error_desc && error_desc->GetText()) {
        desc = *error_desc->GetText();
    }
    action->SetError(code, desc);
    return NPT_SUCCESS;
}
