/*****************************************************************
|
|   Platinum - Control Point
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
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
    PLT_CtrlPointListenerOnActionResponseIterator(NPT_Result res, 
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
    PLT_CtrlPointListenerOnEventNotifyIterator(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars) :
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
    m_TaskManager(task_manager), m_CtrlPoint(ctrl_point), m_Device(device) {}

    NPT_Result operator()(PLT_Service*& service) const {
        // look for the host and port of the device
        PLT_DeviceData* device = service->GetDevice();
        NPT_String SCPDURLPath = service->GetSCPDURL();

        // if the SCPD Url starts with a "/", this means we should not append it to the base URI
        // but instead go there directly
        if (!SCPDURLPath.StartsWith("/")) {
            SCPDURLPath = device->GetURLBase().GetPath() + SCPDURLPath;
        }
        NPT_HttpUrl url(device->GetURLBase().GetHost(), device->GetURLBase().GetPort(), SCPDURLPath);

        PLT_CtrlPointGetSCPDTask* task = new PLT_CtrlPointGetSCPDTask(url, m_CtrlPoint, (PLT_DeviceDataReference&)m_Device);
        return m_TaskManager->StartTask(task);
    }

private:
    PLT_TaskManager*        m_TaskManager;
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_DeviceDataReference m_Device;
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
        NPT_CHECK(device->m_Services.ApplyUntil(PLT_ServiceReadyIterator(), 
                                                NPT_UntilResultNotEquals(NPT_SUCCESS)));
        NPT_CHECK(device->m_EmbeddedDevices.ApplyUntil(PLT_DeviceReadyIterator(), 
                                                       NPT_UntilResultNotEquals(NPT_SUCCESS)));
        return NPT_SUCCESS;
    }
};

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::PLT_CtrlPoint
+---------------------------------------------------------------------*/
PLT_CtrlPoint::PLT_CtrlPoint(const char* autosearch /* = "upnp:rootdevice" */) :
    m_ReferenceCount(1),
    m_HouseKeepingTask(NULL),
    m_EventHttpServer(new PLT_HttpServer()),
    m_TaskManager(NULL),
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
    Stop();

    delete m_EventHttpServer;
    delete m_EventHttpServerHandler; 
    m_Subscribers.Apply(NPT_ObjectDeleter<PLT_EventSubscriber>());
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
PLT_CtrlPoint::Start(PLT_TaskManager* task_manager)
{
    m_TaskManager = task_manager;
    m_EventHttpServer->Start();

    m_HouseKeepingTask = new PLT_CtrlPointHouseKeepingTask(this);
    m_TaskManager->StartTask(m_HouseKeepingTask, NULL, false);
        
    return m_AutoSearch.GetLength()?Search(NPT_HttpUrl("239.255.255.250", 1900, "*"), m_AutoSearch):NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Stop()
{
    if (m_HouseKeepingTask) {
        m_HouseKeepingTask->Kill();
        m_HouseKeepingTask = NULL;
    }

    {
        NPT_List<PLT_SsdpSearchTask*>::Iterator task = m_SsdpSearchTasks.GetFirstItem();
        while (task) {
            (*task)->Kill();
            ++task;
        }
        m_SsdpSearchTasks.Clear();
    }

    m_EventHttpServer->Stop();

    // empty device list
    m_Devices.Clear();

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
                                const NPT_Cardinal   MX, 
                                const NPT_IpAddress& address)
{
    // create socket
    NPT_UdpMulticastSocket* socket = new NPT_UdpMulticastSocket();
    socket->SetInterface(address);
    socket->SetTimeToLive(4);

    // create request
    NPT_HttpRequest* request = new NPT_HttpRequest(url, "M-SEARCH", "HTTP/1.1");
    PLT_UPnPMessageHelper::SetMX(request, MX);
    PLT_UPnPMessageHelper::SetST(request, target);
    PLT_UPnPMessageHelper::SetMAN(request, "\"ssdp:discover\"");

    // create task
    PLT_SsdpSearchTask* task = new PLT_SsdpSearchTask(
        socket,
        this, 
        request,
        MX<1?10000:MX*10000);
    return task;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Search
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Search(const NPT_HttpUrl& url, const char* target, const NPT_Cardinal MX /* = 5 */)
{
    NPT_List<NPT_NetworkInterface*> if_list;
    NPT_List<NPT_NetworkInterface*>::Iterator net_if;
    NPT_List<NPT_NetworkInterfaceAddress>::Iterator net_if_addr;

    NPT_CHECK_SEVERE(NPT_NetworkInterface::GetNetworkInterfaces(if_list));

    for (net_if = if_list.GetFirstItem(); net_if; net_if++) {
        // make sure the interface is at least broadcast or multicast
        if (!((*net_if)->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_MULTICAST) &&
            !((*net_if)->GetFlags() & NPT_NETWORK_INTERFACE_FLAG_BROADCAST)) {
            continue;
        }       
            
        for (net_if_addr = (*net_if)->GetAddresses().GetFirstItem(); net_if_addr; net_if_addr++) {
            // don't advertise on disconnected interfaces
            if (!(*net_if_addr).GetPrimaryAddress().ToString().Compare("0.0.0.0"))
                continue;

            // create task
            PLT_SsdpSearchTask* task = CreateSearchTask(url, 
                target, 
                MX, 
                (*net_if_addr).GetPrimaryAddress());
            m_SsdpSearchTasks.Add(task);
            m_TaskManager->StartTask(task, NULL, false);
        }
    }

    if (m_SsdpSearchTasks.GetItemCount() == 0) {
        // create task on 127.0.0.1
        NPT_IpAddress address;
        address.ResolveName("127.0.0.1");

        PLT_SsdpSearchTask* task = CreateSearchTask(url, 
            target, 
            MX, 
            address);
        m_SsdpSearchTasks.Add(task);
        m_TaskManager->StartTask(task, NULL, false);
    }

    if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Discover
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Discover(const NPT_HttpUrl& url, 
                        const char*        target, 
                        const NPT_Cardinal MX /* = 5 */,
                        NPT_Timeout        repeat /* = 50000 */)
{
    // make sure MX is at least 1
    const NPT_Cardinal mx = MX<1?1:MX;

    // create socket
    NPT_UdpSocket* socket = new NPT_UdpSocket();

    // create request
    NPT_HttpRequest* request = new NPT_HttpRequest(url, "M-SEARCH", "HTTP/1.1");
    PLT_UPnPMessageHelper::SetMX(request, mx);
    PLT_UPnPMessageHelper::SetST(request, target);
    PLT_UPnPMessageHelper::SetMAN(request, "\"ssdp:discover\"");

    // force HOST to be the regular multicast address:port
    // Some servers do care (like WMC) otherwise they won't response to us
    request->GetHeaders().SetHeader(NPT_HTTP_HEADER_HOST, "239.255.255.250:1900");

    // create task
    PLT_SsdpSearchTask* task = new PLT_SsdpSearchTask(
        socket,
        this, 
        request,
        repeat<(NPT_Timeout)mx*5000?(NPT_Timeout)mx*5000:repeat);  /* repeat no less than every 5 secs */
    m_SsdpSearchTasks.Add(task);
    return m_TaskManager->StartTask(task, NULL, false);
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::DoHouseKeeping
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::DoHouseKeeping()
{
    NPT_AutoLock lock(m_Devices);
    NPT_TimeStamp now;
    int count = m_Devices.GetItemCount();
    NPT_System::GetCurrentTimeStamp(now);

    PLT_DeviceDataReference device;
    while (count--) {
        NPT_Result res = m_Devices.PopHead(device);
        if (NPT_SUCCEEDED(res)) {
            NPT_TimeStamp lastUpdate = device->GetLeaseTimeLastUpdate();
            NPT_TimeInterval leaseTime = device->GetLeaseTime();

            // check if device lease time has expired
            if (now > lastUpdate + NPT_TimeInterval((unsigned long)(((float)leaseTime)*2), 0)) {
                NPT_AutoLock lock(m_ListenerList);
                m_ListenerList.Apply(PLT_CtrlPointListenerOnDeviceRemovedIterator(device));
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
    NPT_List<PLT_DeviceDataReference>::Iterator it = m_Devices.Find(PLT_DeviceDataFinder(uuid));
    if (it) {
        device = (*it);
        return NPT_SUCCESS;
    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessHttpRequest(NPT_HttpRequest&  request,
                                  NPT_HttpResponse& response,
                                  NPT_SocketInfo&   client_info)
{
    NPT_COMPILER_UNUSED(client_info);

    NPT_List<PLT_StateVariable*> vars;
    PLT_EventSubscriber*         sub = NULL;    
    NPT_String                   str;
    NPT_XmlElementNode*          xml = NULL;
    NPT_String                   sid, NT, NTS, content_type;
    NPT_String                   callbackUri;
    NPT_String                   uuid;
    NPT_String                   serviceId;
    long                         seq = 0;
    PLT_Service*                 service = NULL;
    PLT_DeviceData*              device = NULL;

    NPT_AutoLock lock(m_Subscribers);

    NPT_String strMehod    = request.GetMethod();
    NPT_String strUri      = request.GetUrl().GetPath();
    NPT_String strProtocol = request.GetProtocol();

//     NPT_LOG_FINE_3("CtrlPoint received %s request from %s:%d\r\n", 
//         request.GetMethod(), 
//         client_info.remote_address.GetIpAddress(), 
//         client_info.remote_address.GetPort());
// 
//     PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);
// 
    PLT_UPnPMessageHelper::GetSID(&request, sid);
    PLT_HttpHelper::GetContentType(&request, content_type);
    PLT_UPnPMessageHelper::GetNT(&request, NT);
    PLT_UPnPMessageHelper::GetNTS(&request, NTS);

    // look for the subscriber with that subscription url
    if (NPT_FAILED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderBySID(sid), sub))) {
        NPT_LOG_FINE_1("Subscriber %s not found\n", (const char*)sid);
        goto bad_request;
    }

    // verify the request is syntactically correct
    service =  sub->GetService();
    device = service->GetDevice();

    uuid = device->GetUUID();
    serviceId = service->GetServiceID();

    // callback uri for this sub
    callbackUri = "/" + uuid + "/" + serviceId;

    if (strMehod.Compare("NOTIFY") || strUri.Compare(callbackUri, true) ||
        NT.Compare("upnp:event", true) || NTS.Compare("upnp:propchange", true)) {
        goto bad_request;
    }

    // if the sequence number is less than our current one, we got it out of order
    // so we disregard it
    PLT_UPnPMessageHelper::GetSeq(&request, seq);
    if (sub->GetEventKey() && seq <= (int)sub->GetEventKey()) {
        goto bad_request;
    }

    // parse body
    if (NPT_FAILED(PLT_HttpHelper::ParseBody(&request, xml))) {
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
    NPT_XmlElementNode *property;
    PLT_StateVariable* var;
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
        if (NPT_FAILED(var->SetValue(property->GetText()?*property->GetText():"", false))) {
            goto bad_request;
        }
        vars.Add(var);
    }    

    // update sequence
    sub->SetEventKey(seq);

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
PLT_CtrlPoint::ProcessSsdpSearchResponse(NPT_Result        res, 
                                         NPT_SocketInfo&   info, 
                                         NPT_HttpResponse* response)
{
    if (NPT_FAILED(res) || response == NULL) {
        return NPT_FAILURE;
    }

    NPT_String ip_address  = info.remote_address.GetIpAddress().ToString();
    NPT_String strProtocol = response->GetProtocol();
    
    NPT_LOG_FINE_2("CtrlPoint received SSDP search response from %s:%d",
        (const char*)info.remote_address.GetIpAddress().ToString() , 
        info.remote_address.GetPort());
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);
    
    if (response->GetStatusCode() == 200) {
        NPT_String ST;
        NPT_String USN;
        NPT_String EXT;
        NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetST(response, ST));
        NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetUSN(response, USN));
        NPT_CHECK_SEVERE(response->GetHeaders().GetHeaderValue("EXT", EXT));
        
        NPT_String UUID;
        // if we get an advertisement other than UUID
        // verify it's formatted properly
        if (USN != ST) {
            char uuid[200];
            char st[200];
            int  ret;
            // FIXME: We can't use sscanf directly!
            ret = sscanf(((const char*)USN)+5, "%[^::]::%s",
                uuid, 
                st);
            if (ret != 2)
                return NPT_FAILURE;
            
            if (ST.Compare(st, true))
                return NPT_FAILURE;
            
            UUID = uuid;
        } else {
            UUID = ((const char*)USN)+5;
        }
        
        if (m_UUIDsToIgnore.Find(NPT_StringFinder(UUID))) {
            NPT_LOG_FINE_1("CtrlPoint received a search response from ourselves (%s)\n", (const char*)UUID);
            return NPT_SUCCESS;
        }

        return ProcessSsdpMessage(response, info, UUID);    
    }
    
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::OnSsdpPacket
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::OnSsdpPacket(NPT_HttpRequest& request, 
                            NPT_SocketInfo   info)
{
    return ProcessSsdpNotify(request, info);
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessSsdpNotify
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessSsdpNotify(NPT_HttpRequest& request, 
                                 NPT_SocketInfo   info)
{
    // get the address of who sent us some data back
    NPT_String ip_address = info.remote_address.GetIpAddress().ToString();
    NPT_String strMehod = request.GetMethod();
    NPT_String strUri = (const char*)request.GetUrl().GetPath();
    NPT_String strProtocol = request.GetProtocol();

    if (strMehod.Compare("NOTIFY") == 0) {
        NPT_LOG_FINE_2("CtrlPoint received SSDP Notify from %s:%d",
            (const char*)info.remote_address.GetIpAddress().ToString(), 
            info.remote_address.GetPort());
        PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);

        if ((strUri.Compare("*") != 0) || (strProtocol.Compare("HTTP/1.1") != 0))
            return NPT_FAILURE;
        
        NPT_String NTS;
        NPT_String NT;
        NPT_String USN;
        NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetNTS(&request, NTS));
        NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetNT(&request, NT));
        NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetUSN(&request, USN));

        NPT_String UUID;
        // if we get an advertisement other than UUID
        // verify it's formatted properly
        if (USN != NT) {
            char uuid[200];
            char nt[200];
            int  ret;
            //FIXME: no sscanf!
            ret = sscanf(((const char*)USN)+5, "%[^::]::%s",
                uuid, 
                nt);
            if (ret != 2)
                return NPT_FAILURE;
            
            if (NT.Compare(nt, true))
                return NPT_FAILURE;
            
            UUID = uuid;
        } else {
            UUID = ((const char*)USN)+5;
        }

        if (m_UUIDsToIgnore.Find(NPT_StringFinder(UUID))) {
            NPT_LOG_FINE_1("CtrlPoint received a Notify request from ourselves (%s)\n", (const char*)UUID);
            return NPT_SUCCESS;
        }

        NPT_LOG_FINE_1("CtrlPoint received a Notify request from %s\n", (const char*)UUID);
        
        // if it's a byebye, remove the device and return right away
        if (NTS.Compare("ssdp:byebye", true) == 0) {
            NPT_AutoLock lock(m_Devices);
            PLT_DeviceDataReference data;
            NPT_ContainerFind(m_Devices, PLT_DeviceDataFinder(UUID), data);
            if (!data.IsNull()) {
                m_Devices.Remove(data);

                NPT_AutoLock lock(m_ListenerList);
                m_ListenerList.Apply(PLT_CtrlPointListenerOnDeviceRemovedIterator(data));

                // remove any subscribers on all services associated with this device
                for (NPT_Cardinal i=0; i<data->m_Services.GetItemCount(); i++) {
                    PLT_EventSubscriber* sub;
                    PLT_Service* service = data->m_Services[i];
                    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderByService(service), sub))) {
                        m_Subscribers.Remove(sub, true);
                        delete sub;
                    }
                }

                // remove all embededed devices
                for (NPT_Cardinal j=0; j < data->m_EmbeddedDevices.GetItemCount(); j++) {
                    PLT_DeviceDataReference data2 = data->m_EmbeddedDevices[j];
                    if (m_Devices.Contains(data2)) {
                        m_Devices.Remove(data2);
                        m_ListenerList.Apply(PLT_CtrlPointListenerOnDeviceRemovedIterator(data2));

                        for (NPT_Cardinal i=0; i<data2->m_Services.GetItemCount(); i++) {
                            PLT_EventSubscriber* sub;
                            PLT_Service* service = data2->m_Services[i];
                            if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderByService(service), sub))) {
                                m_Subscribers.Remove(sub, true);
                                delete sub;
                            }
                        }
                    }
                }

            }
            return NPT_SUCCESS;
        }
        
        return ProcessSsdpMessage(&request, info, UUID);
    }
    
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessSsdpMessage
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessSsdpMessage(NPT_HttpMessage* message, NPT_SocketInfo& info, NPT_String& uuid)
{
    NPT_COMPILER_UNUSED(info);

    if (m_UUIDsToIgnore.Find(NPT_StringFinder(uuid))) {
        return NPT_SUCCESS;
    }
    
    NPT_String location;
    NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetLocation(message, location));

    NPT_HttpUrl url(location);
    if (!url.IsValid()) return NPT_FAILURE;
    
    // be nice and assume a default lease time if not found
    NPT_Timeout leasetime;
    if (NPT_FAILED(PLT_UPnPMessageHelper::GetLeaseTime(message, leasetime))) {
        leasetime = 1800;
    }
    
    NPT_AutoLock lock(m_Devices);
    PLT_DeviceDataReference data;
    // is it a new device?
    if (NPT_FAILED(NPT_ContainerFind(m_Devices, PLT_DeviceDataFinder(uuid), data))) {
        data = new PLT_DeviceData(url, uuid, NPT_TimeInterval(leasetime, 0));
        data->SetURLBase(location);
        m_Devices.Add(data);
        
        // Start a task to retrieve the description
        PLT_CtrlPointGetDescriptionTask* task = new PLT_CtrlPointGetDescriptionTask(
            url,
            this, 
            data);
        m_TaskManager->StartTask(task);
        return NPT_SUCCESS;
    } else {
        // renew expiration time
        data->SetLeaseTime(NPT_TimeInterval(leasetime, 0));

        // the url may have changed
        if (data->GetDescriptionUrl() != url.ToString()) {
            data->SetDescriptionUrl(url);

            // Start a task to retrieve the description
            PLT_CtrlPointGetDescriptionTask* task = new PLT_CtrlPointGetDescriptionTask(
                url,
                this, 
                data);
            m_TaskManager->StartTask(task);
        }

        NPT_LOG_INFO_1("Device (%s) expiration time renewed..", (const char*)uuid);
        return NPT_SUCCESS;
    }
    
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::ProcessGetDescriptionResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::ProcessGetDescriptionResponse(NPT_Result               res, 
                                             NPT_HttpResponse*        response, 
                                             PLT_DeviceDataReference& device)
{
    NPT_String   desc;
    NPT_AutoLock lock(m_Devices);
    
    NPT_LOG_FINER_1("CtrlPoint received Device Description response (result = %d)", res);

    // make sure we've seen this device
    PLT_DeviceDataReference data;
    if (NPT_FAILED(NPT_ContainerFind(m_Devices, PLT_DeviceDataFinder(device->GetUUID()), data)) || (device != data)) {
        return NPT_FAILURE;
    }

    if (NPT_FAILED(res) || response == NULL) {
        goto bad_response;
    }
    
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);

    // get body
    if (NPT_FAILED(res = PLT_HttpHelper::GetBody(response, desc))) {
        goto bad_response;
    }
    
    // set the device description
    if (NPT_FAILED(data->SetDescription(desc))) {
        goto bad_response;
    }

    // Get SCPD of root device services
    if (NPT_FAILED(data->m_Services.Apply(PLT_AddGetSCPDRequestIterator(m_TaskManager, this, data)))) {
        goto bad_response;
    }

    
    // add all embeded devices
    for (NPT_Cardinal i=0; i < data->m_EmbeddedDevices.GetItemCount(); i++) {
        // check so we've haven't seen it before
      PLT_DeviceDataReference data2 = data->m_EmbeddedDevices[i];
      if (NPT_FAILED(NPT_ContainerFind(m_Devices, PLT_DeviceDataFinder(data2->GetUUID()), data2))) {          
        if (NPT_FAILED(data2->m_Services.Apply(PLT_AddGetSCPDRequestIterator(m_TaskManager, this, data2)))) {
          NPT_LOG_SEVERE("Bad Description response from sub device skipped");
          continue;
        }
        m_Devices.Add(data2);
      }
    }

    return NPT_SUCCESS;

bad_response:
    NPT_LOG_SEVERE("Bad Description response");
    m_Devices.Remove(data);
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
    PLT_DeviceReadyIterator deviceTester;   
    NPT_String              scpd;
    NPT_AutoLock            lock(m_Devices);

    NPT_LOG_FINER_1("CtrlPoint received SCPD response (result = %d)", res);

    PLT_DeviceDataReference data;
    if (NPT_FAILED(NPT_ContainerFind(m_Devices, PLT_DeviceDataFinder(device->GetUUID()), data)) || (data != device)) {
        return NPT_FAILURE;
    }

    if (NPT_FAILED(res) || response == NULL || request  == NULL) {
        goto bad_response;
    }
    
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);

    // get body
    if (NPT_FAILED(res = PLT_HttpHelper::GetBody(response, scpd))) {
        goto bad_response;
    }

    // look for the service based on the SCPD uri
    PLT_Service* service;
    if (NPT_FAILED(data->FindServiceByDescriptionURI(request->GetUrl().GetPath(), service))) {
        goto bad_response;
    }    
    
    // set the service scpd
    if (NPT_FAILED(service->SetSCPDXML(scpd))) {
        goto bad_response;
    }
    
    // if all scpds have been retrieved, notify listeners
    // new device is discovered
    if (NPT_SUCCEEDED(deviceTester(data))) {
        // notify that the device is ready to use
        NPT_AutoLock lock(m_ListenerList);
        m_ListenerList.Apply(PLT_CtrlPointListenerOnDeviceAddedIterator(data));
    }
    
    return NPT_SUCCESS;

bad_response:
    NPT_LOG_SEVERE("Bad SCPD response");
    m_Devices.Remove(data);
    return res;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::Subscribe
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::Subscribe(PLT_Service* service, bool cancel, void* userdata)
{
    NPT_AutoLock lock(m_Subscribers);

    if (!service->IsSubscribable())
        return NPT_FAILURE;

    // look for the host and port of the device
    PLT_DeviceData* device = service->GetDevice();

    // get the relative event subscription url
    // if URL starts with '/', it's not to be appended to base URL
    NPT_String eventSubUrl = service->GetEventSubURL();
    if (!eventSubUrl.StartsWith("/")) {
        eventSubUrl = device->GetURLBase().GetPath() + eventSubUrl;
    }

    NPT_HttpUrl url(device->GetURLBase().GetHost(), 
                    device->GetURLBase().GetPort(), 
                    eventSubUrl);

    // look for the subscriber with that service to decide if it's a renewal or not
    PLT_EventSubscriber* sub = NULL;
    NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderByService(service), sub);

    // create the request
    NPT_HttpRequest* request = NULL;

    if (cancel == false) {
        // renewal?
        if (sub) {
            NPT_LOG_INFO_1("Renewing subscriber (%s)", (const char*)sub->GetSID());

            // create the request
            request = new NPT_HttpRequest(url, "SUBSCRIBE");

            PLT_UPnPMessageHelper::SetSID(request, sub->GetSID());
            PLT_UPnPMessageHelper::SetTimeOut(request, 1800);
        } else {
            NPT_LOG_INFO("Subscribing");

            NPT_String uuid = device->GetUUID();
            NPT_String serviceId = service->GetServiceID();

            // prepare the callback url
            NPT_String callbackUri = "/" + uuid + "/" + serviceId;

            // for now we use only the first interface we found
            // (we should instead use the one we know we've used to talk to the device)
            NPT_List<NPT_NetworkInterface*> if_list;
            NPT_CHECK_SEVERE(NPT_NetworkInterface::GetNetworkInterfaces(if_list));
            NPT_List<NPT_NetworkInterface*>::Iterator net_if = if_list.GetFirstItem();
            if (net_if) {
                NPT_List<NPT_NetworkInterfaceAddress>::Iterator niaddr = (*net_if)->GetAddresses().GetFirstItem();
                if (niaddr) {
                    // FIXME: This could be a disconnected interface, we should check for "0.0.0.0"
                    NPT_String ip = (*niaddr).GetPrimaryAddress().ToString();

                    // create the request
                    request = new NPT_HttpRequest(url, "SUBSCRIBE");
                    NPT_HttpUrl callbackUrl(ip, m_EventHttpServer->GetPort(), callbackUri);

                    // set the required headers for a new subscription
                    PLT_UPnPMessageHelper::SetNT(request, "upnp:event");
                    PLT_UPnPMessageHelper::SetCallbacks(request, "<" + callbackUrl.ToString() + ">");
                    PLT_UPnPMessageHelper::SetTimeOut(request, 1800);
                }
            }
            if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());

        }
    } else {
        // cancellation
        if (!sub) {
            NPT_LOG_WARNING("Unsubscribing subscriber (not found)");
            return NPT_FAILURE;
        }

        // create the request
        request = new NPT_HttpRequest(url, "UNSUBSCRIBE");
        PLT_UPnPMessageHelper::SetSID(request, sub->GetSID());

        // remove from list now
        m_Subscribers.Remove(sub, true);
        delete sub;
    }

    if (!request) {
        NPT_LOG_WARNING("Subscribe failed");
        return NPT_FAILURE;
    }

    // Prepare the request
    // create a task to post the request
    PLT_CtrlPointSubscribeEventTask* task = new PLT_CtrlPointSubscribeEventTask(
        request,
        this, 
        service, 
        userdata);
    m_TaskManager->StartTask(task);

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
    NPT_String  sid;
    NPT_Timeout timeout;
    PLT_EventSubscriber* sub = NULL;
    NPT_AutoLock lock(m_Subscribers);

   NPT_LOG_FINER_1("CtrlPoint received Subscription response (result = %d)", res);

   if (response) {
       PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);
   }

    // if there's a failure or it's a response to a cancellation
    // we get out
    if (NPT_FAILED(res) || response == NULL                      || 
        response->GetStatusCode() != 200                         || 
        NPT_FAILED(PLT_UPnPMessageHelper::GetSID(response, sid)) || 
        NPT_FAILED(PLT_UPnPMessageHelper::GetTimeOut(response, timeout))) {
        NPT_LOG_WARNING("(un)Subscription failed.");
        goto failure;
    }

    // look for the subscriber with that sid
    if (NPT_FAILED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderBySID(sid), sub))) {
        NPT_LOG_WARNING("Subscriber not found .. creating new one.");
        sub = new PLT_EventSubscriber(m_TaskManager, service);
        sub->SetSID(sid);
        m_Subscribers.Add(sub);
    }

    // keep track of subscriber lifetime
    // -1 means infinite so we set an expiration time of 0
    if (timeout == NPT_TIMEOUT_INFINITE) {
        sub->SetExpirationTime(NPT_TimeStamp(0, 0));
    } else {
        NPT_TimeStamp now;
        NPT_System::GetCurrentTimeStamp(now);
        sub->SetExpirationTime(now + NPT_TimeInterval(timeout, 0));
    }

    return NPT_SUCCESS;

failure:
    // look for the subscriber with that service and remove it from the list
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderByService(service), sub))) {
        m_Subscribers.Remove(sub, true);
        delete sub;
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_CtrlPoint::InvokeAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_CtrlPoint::InvokeAction(PLT_ActionReference& action, 
                            void*                    userdata)
{
    PLT_Service*    service = action->GetActionDesc()->GetService();
    PLT_DeviceData* device  = service->GetDevice();

    // look for the service control url
    NPT_String serviceUrl = service->GetControlURL();

    // if URL starts with a "/", it's not to be appended to base URL
    if (!serviceUrl.StartsWith("/")) {
        serviceUrl = device->GetURLBase().GetPath() + serviceUrl;
    }

    // create the request
    // FIXME: hack use HTTP/1.0 for now because of WMC that returning 100 Continue when using HTTP/1.1
    // and this screws up the http processing right now
    NPT_HttpUrl url(device->GetURLBase().GetHost(), device->GetURLBase().GetPort(), serviceUrl);
    NPT_HttpRequest* request = new NPT_HttpRequest(url, "POST");
    
    // create a memory stream for our request body
    NPT_MemoryStreamReference stream(new NPT_MemoryStream);
    action->FormatSoapRequest(*stream);

    // set the request body
    PLT_HttpHelper::SetBody(request, (NPT_InputStreamReference&)stream);

    PLT_HttpHelper::SetContentType(request, "text/xml; charset=\"utf-8\"");
    NPT_String serviceType = service->GetServiceType();
    NPT_String actionName = action->GetActionDesc()->GetName();
    request->GetHeaders().SetHeader("SOAPAction", "\"" + serviceType + "#" + actionName + "\"");

    // create a task to post the request
    PLT_CtrlPointInvokeActionTask* task = new PLT_CtrlPointInvokeActionTask(
        request,
        this, 
        action, 
        userdata);

    // queue the request
    m_TaskManager->StartTask(task);

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

    if (m_ListenerList.GetItemCount() == 0) {
        return NPT_SUCCESS;
    }

    // reset the error code and desc
    action->SetError(0, "");

    // check context validity
    if (NPT_FAILED(res) || response == NULL) {
        goto failure;
    }

    NPT_LOG_FINE("Received Action Response:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);

    NPT_LOG_FINER("Reading/Parsing Action Response Body...");
    if (NPT_FAILED(PLT_HttpHelper::ParseBody(response, xml))) {
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
                          NPT_XmlElementNode*      fault)
{
    NPT_XmlElementNode* detail = fault->GetChild("detail");
    if (detail == NULL) return NPT_FAILURE;

    NPT_XmlElementNode *UPnPError, *errorCode, *errorDesc;
    UPnPError = detail->GetChild("UPnPError");
    if (UPnPError == NULL) return NPT_FAILURE;

    errorCode = UPnPError->GetChild("errorCode");
    errorDesc = UPnPError->GetChild("errorDescription");
    long code = 501;    
    NPT_String desc;
    if (errorCode && errorCode->GetText()) {
        NPT_String value = *errorCode->GetText();
        value.ToInteger(code);
    }
    if (errorDesc && errorDesc->GetText()) {
        desc = *errorDesc->GetText();
    }
    action->SetError(code, desc);
    return NPT_SUCCESS;
}
