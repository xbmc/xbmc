/*****************************************************************
|
|   Platinum - Device Host
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptUtils.h"
#include "PltService.h"
#include "PltDeviceHost.h"
#include "PltUPnP.h"
#include "PltXmlHelper.h"
#include "PltSsdp.h"
#include "PltHttpServer.h"
#include "PltVersion.h"

/*----------------------------------------------------------------------
|   PLT_HttpServerHandler::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServerHandler::ProcessHttpRequest(NPT_HttpRequest*   request, 
                                          NPT_SocketInfo     info, 
                                          NPT_HttpResponse*& response) 
{
    response = new NPT_HttpResponse(200, "OK");
    response->GetHeaders().SetHeader("Server", 
        "Platinum/" PLT_PLATINUM_VERSION_STRING);

    return m_Device->ProcessHttpRequest(request, info, response);
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::PLT_DeviceHost
+---------------------------------------------------------------------*/
PLT_DeviceHost::PLT_DeviceHost(const char*  description_path, 
                               const char*  uuid,
                               const char*  device_type,
                               const char*  friendly_name,
                               unsigned int port) :
    PLT_DeviceData(NPT_HttpUrl(NULL, 0, description_path), uuid, NPT_TimeInterval(1800, 0), device_type, friendly_name), 
    m_TaskManager(NULL),
    m_HttpServerHandler(NULL),
    m_HttpServer(NULL),
    m_SsdpAnnounceTask(NULL),
    m_Broadcast(false)
{
    m_HttpServerHandler = new PLT_HttpServerHandler(this);
    m_HttpServer = new PLT_HttpServer(m_HttpServerHandler, port);
}
    
/*----------------------------------------------------------------------
|   PLT_DeviceHost::~PLT_DeviceHost
+---------------------------------------------------------------------*/
PLT_DeviceHost::~PLT_DeviceHost() 
{
    Stop();

    delete m_HttpServer;
    delete m_HttpServerHandler;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::Start(PLT_TaskManager* task_manager)
{
    // keep the task manager around
    m_TaskManager = task_manager;

    // start the server
    m_HttpServer->Start();

    // read back assigned port in case we passed 0
    m_URLDescription.SetPort(m_HttpServer->GetPort());

    // we should not advertise right away, spec says randomly less than 100ms
    NPT_TimeInterval delay(0, NPT_System::GetRandomInteger() % 100000000);

    // calculate when we should send another announcement
    // if we're using broadcast mode (XBOX), we send every 7 secs
    NPT_Size         leaseTime = (NPT_Size)(float)GetLeaseTime();
    NPT_TimeInterval repeat;
    if (m_Broadcast) {
        repeat.m_Seconds = 7;
    } else {
        repeat.m_Seconds = leaseTime?(int)((leaseTime >> 1) + ((unsigned short)NPT_System::GetRandomInteger() % (leaseTime >> 2))):30;
    }

    m_SsdpAnnounceTask = new PLT_SsdpDeviceAnnounceTask(this, repeat, true, m_Broadcast);
    m_TaskManager->StartTask(m_SsdpAnnounceTask, &delay, false);
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::Stop()
{
    NPT_Result res;    
    NPT_List<NPT_NetworkInterface*> if_list;
    
    if (m_SsdpAnnounceTask) {
        //m_TaskManager->StopTask(m_SsdpAnnounceTask);
        m_SsdpAnnounceTask->Kill();
        m_SsdpAnnounceTask = NULL;
    }

    m_HttpServer->Stop();

    res = NPT_NetworkInterface::GetNetworkInterfaces(if_list);
    if (NPT_FAILED(res)) goto cleanup;

    if_list.Apply(PLT_SsdpAnnounceInterfaceIterator(this, true, m_Broadcast));

cleanup:
    if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
    return res;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::Announce
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::Announce(PLT_DeviceData* device, NPT_HttpRequest& req, NPT_UdpSocket& socket, bool byebye)
{
    NPT_Result res = NPT_SUCCESS;

    if (byebye == false) {
        // get location URL based on ip address of interface
        PLT_UPnPMessageHelper::SetNTS(&req, "ssdp:alive");
        PLT_UPnPMessageHelper::SetLeaseTime(&req, (NPT_Timeout)(float)device->GetLeaseTime());
        PLT_UPnPMessageHelper::SetServer(&req, "Platinum/" PLT_PLATINUM_VERSION_STRING);
    } else {
        PLT_UPnPMessageHelper::SetNTS(&req, "ssdp:byebye");
    }

    // target address
    NPT_IpAddress ip;
    if (NPT_FAILED(res = ip.ResolveName(req.GetUrl().GetHost()))) {
        return res;
    }
    NPT_SocketAddress addr(ip, req.GetUrl().GetPort());

    // upnp:rootdevice
    if (device->m_Root == true) {
        PLT_SsdpSender::SendSsdp(req,
            NPT_String("uuid:" + device->m_UUID + "::upnp:rootdevice"), 
            "upnp:rootdevice",
            socket,
            true, 
            &addr);
    }

    // uuid:device-UUID::urn:schemas-upnp-org:device:deviceType:ver
    PLT_SsdpSender::SendSsdp(req,
        NPT_String("uuid:" + device->m_UUID + "::" + device->m_DeviceType), 
        device->m_DeviceType,
        socket,
        true,
        &addr);

    // services
    for (int i=0; i < (int)device->m_Services.GetItemCount(); i++) {
        // uuid:device-UUID::urn:schemas-upnp-org:service:serviceType:ver
        PLT_SsdpSender::SendSsdp(req,
            NPT_String("uuid:" + device->m_UUID + "::" + device->m_Services[i]->GetServiceType()), 
            device->m_Services[i]->GetServiceType(),
            socket,
            true, 
            &addr);        
    }

    // uuid:device-UUID
    PLT_SsdpSender::SendSsdp(req,
        "uuid:" + device->m_UUID, 
        "uuid:" + device->m_UUID, 
        socket, 
        true, 
        &addr);


    // embedded devices
    for (int j=0; j < (int)device->m_EmbeddedDevices.GetItemCount(); j++) {
        Announce(device->m_EmbeddedDevices[j].AsPointer(), 
            req, 
            socket, 
            byebye);
    }

//    res = m_EmbeddedDevices.ApplyUntilFailure(PLT_SsdpAnnounceIterator<PLT_DeviceHost>(req, stream, byebye));
//    if (NPT_FAILED(res)) return res;

    return res;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::ProcessHttpRequest(NPT_HttpRequest*   request, 
                                   NPT_SocketInfo     info,
                                   NPT_HttpResponse*  response) 
{
    // get the address of who sent us some data back*/
    NPT_String ip_address  = info.remote_address.GetIpAddress().ToString();
    NPT_String strMehod    = request->GetMethod();
    NPT_String strProtocol = request->GetProtocol(); 

    PLT_Log(PLT_LOG_LEVEL_3, "PLT_DeviceHost Received Request:\r\n");
    PLT_HttpHelper::ToLog(request, PLT_LOG_LEVEL_3);
   
    if (strMehod.Compare("GET") == 0 || strMehod.Compare("HEAD") == 0) {
        return ProcessHttpGetHeadRequest(request, info, response);
    } else if (strMehod.Compare("POST") == 0) {
        return ProcessHttpPostRequest(request, info, response);
    } else if (strMehod.Compare("SUBSCRIBE") == 0 || strMehod.Compare("UNSUBSCRIBE") == 0) {
        return ProcessHttpSubscriberRequest(request, info, response);
    } else if (strMehod.Compare("NOTIFY") == 0) {
        return ProcessHttpEventRequest(request, info, response);
    } else {
        response->SetStatus(400, "Bad Request");
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::ProcessHttpGetHeadRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::ProcessHttpGetHeadRequest(NPT_HttpRequest*   request, 
                                          NPT_SocketInfo     info,
                                          NPT_HttpResponse*  response) 
{
    NPT_String   ip_address = info.local_address.GetIpAddress().ToString();
    NPT_String   strMehod = request->GetMethod();
    NPT_String   url = request->GetUrl().ToRequestString(true);
    NPT_String   strProtocol = request->GetProtocol();
    NPT_String   body;
    PLT_Service* service;

    // FIXME: This will not work if description url containe queries
    if (url.Compare(m_URLDescription.GetPath(), true) == 0) {            
        // Device Description requested
        GetDescription(body);
    } else if (NPT_SUCCEEDED(FindServiceByDescriptionURI(url, service))) {
        // Service Description requested (SCPD)
        service->GetSCPDXML(body);
    }        

    if (body.GetLength() != 0) {
        // add the right headers and write them to the stream
        PLT_HttpHelper::SetContentType(response, "text/xml");
        // only if it's a GET, write the body now
        if (strMehod.Compare("HEAD") != 0) {
            PLT_HttpHelper::SetBody(response, body);
        } else {
            PLT_HttpHelper::SetContentLength(response, body.GetLength());
        }
        return NPT_SUCCESS;
    } else {
        response->SetStatus(404, "File Not Found");
    }
    
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::ProcessPostRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::ProcessHttpPostRequest(NPT_HttpRequest*   request, 
                                       NPT_SocketInfo     info,
                                       NPT_HttpResponse*  response) 
{
    NPT_Result                res;
    //PLT_Arguments             args;
    NPT_String                service_type;
    NPT_String                str;
    NPT_XmlElementNode*       xml = NULL;
    NPT_String                soap_action_header;
    PLT_Service*              service;
    //NPT_List<NPT_XmlNode*>    children;
    NPT_XmlElementNode*       soap_body;
    NPT_XmlElementNode*       soap_action;
    const NPT_String*         attr;
    PLT_ActionDesc*           action_desc;
    NPT_MemoryStreamReference resp(new NPT_MemoryStream);
    NPT_String                ip_address = info.remote_address.GetIpAddress().ToString();
    NPT_String                strMehod = request->GetMethod();
    NPT_String                url = request->GetUrl().ToRequestString(true);
    NPT_String                strProtocol = request->GetProtocol();
    PLT_ActionReference       action;

    if (NPT_FAILED(FindServiceByControlURI(url, service)))
        goto bad_request;

    if (NPT_FAILED(request->GetHeaders().GetHeaderValue("SOAPAction", soap_action_header)))
        goto bad_request;

    // extract the soap action name from the header
    soap_action_header.TrimLeft('"');
    soap_action_header.TrimRight('"');
    char prefix[200];
    char soap_action_name[100];
    int  ret;
    //FIXME: no sscanf
    ret = sscanf(soap_action_header, "%[^#]#%s",
                 prefix, 
                 soap_action_name);
    if (ret != 2)
        goto bad_request;

    // read the xml body and parse it
    if (NPT_FAILED(PLT_HttpHelper::ParseBody(*request, xml)))
        goto bad_request;

    // check envelope
    if (xml->GetTag().Compare("Envelope", true))
        goto bad_request;

    // check namespace
    if (!xml->GetNamespace() || xml->GetNamespace()->Compare("http://schemas.xmlsoap.org/soap/envelope/"))
        goto bad_request;

    // check encoding
    attr = xml->GetAttribute("encodingStyle", "http://schemas.xmlsoap.org/soap/envelope/");
    if (!attr || attr->Compare("http://schemas.xmlsoap.org/soap/encoding/"))
        goto bad_request;

    // read action
    soap_body = PLT_XmlHelper::GetChild(xml, "Body");
    if (soap_body == NULL)
        goto bad_request;

    PLT_XmlHelper::GetChild(soap_body, soap_action);
    if (soap_action == NULL)
        goto bad_request;

    // verify action name is identical to SOAPACTION header*/
    if (soap_action->GetTag().Compare(soap_action_name, true))
        goto bad_request;

    // verify namespace
    if (!soap_action->GetNamespace() || soap_action->GetNamespace()->Compare(service->GetServiceType()))
        goto bad_request;

    // create a buffer for our response body and call the service
    if ((action_desc = service->FindActionDesc(soap_action_name)) == NULL) {
        res = NPT_FAILURE;
        // create a bastard soap response
        PLT_Action::FormatSoapError(401, "Invalid Action", *resp);
        goto error;
    }

    action = new PLT_Action(action_desc);

    // read all the arguments if any
    for (NPT_List<NPT_XmlNode*>::Iterator args = soap_action->GetChildren().GetFirstItem(); args; args++) {
        NPT_XmlElementNode* child = (*args)->AsElementNode();
        if (!child) continue;

        res = action->SetArgumentValue(
            child->GetTag(),
            child->GetText()?*child->GetText():"");

        if (NPT_FAILED(res)) {
            // FIXME: incorrect upnp error?
            PLT_Action::FormatSoapError(401, "Invalid Action", *resp);
            goto error;
        }
    }

    if (NPT_FAILED(action->VerifyArguments(true))) {
        action->SetError(402, "Invalid Args");
        goto error;
    }
    
    // call the virtual function, it's all good
    // set the error in case the it wasn't done
    if (NPT_FAILED(OnAction(action, &info))) {
        goto error;
    }

    // create the soap response now
    action->FormatSoapResponse(*resp);
    goto done;

error:
    if (action->GetErrorCode() == 0) {
        action->SetError(501, "Action Failed");
    }
    response->SetStatus(500, "Internal Server Error");

done:
    //args.Apply(NPT_ObjectDeleter<PLT_Argument>());

    NPT_Size resp_body_size;    
    if (NPT_SUCCEEDED(resp->GetAvailable(resp_body_size))) {
        PLT_HttpHelper::SetContentType(response, "text/xml; charset=\"utf-8\"");
        response->GetHeaders().SetHeader("Ext", ""); // should only be for M-POST but oh well
        PLT_HttpHelper::SetBody(response, (NPT_InputStreamReference&)resp);
    }    
    
    delete xml;
    return NPT_SUCCESS;

bad_request:
    delete xml;
    response->SetStatus(400, "Bad Request");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::ProcessHttpSubscriberRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::ProcessHttpSubscriberRequest(NPT_HttpRequest*   request, 
                                             NPT_SocketInfo     info,
                                             NPT_HttpResponse*  response) 
{
    NPT_String  ip_address = info.remote_address.GetIpAddress().ToString();
    NPT_String  strMehod = request->GetMethod();
    NPT_String  url = request->GetUrl().ToRequestString(true);
    NPT_String  strProtocol = request->GetProtocol();
    
    PLT_Service* service;
    if (NPT_FAILED(FindServiceByEventSubURI(url, service))) {
        goto cleanup;
    }

    if (strMehod.Compare("SUBSCRIBE") == 0) {
        NPT_String strNT, strCallbackURLs, strSID;

        // Do we have a SID
        if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetSID(request, strSID))) {
            // make sure we don't have a callback nor a NT
            if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetNT(request, strNT)) ||
                NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetCallbacks(request, strCallbackURLs))) {
                goto cleanup;
            }
          
            NPT_Timeout timeout;
            if (NPT_FAILED(PLT_UPnPMessageHelper::GetTimeOut(request, timeout))) {
                timeout = 1800;
            }
            // subscription renewed
            // send the info to the service
            service->ProcessRenewSubscription(info.local_address, 
                                              strSID, 
                                              timeout, 
                                              response);
            return NPT_SUCCESS;
        } else {
            // new subscription ?
            // verify NT is present and valid
            if (NPT_FAILED(PLT_UPnPMessageHelper::GetNT(request, strNT)) || 
                strNT.Compare("upnp:event", true)) {
                response->SetStatus(412, "Precondition failed");
                return NPT_FAILURE;
            }
            // verify callback is present
            if (NPT_FAILED(PLT_UPnPMessageHelper::GetCallbacks(request, strCallbackURLs))) {
                response->SetStatus(412, "Precondition failed");
                return NPT_FAILURE;
            }

            NPT_Timeout timeout;
            if (NPT_FAILED(PLT_UPnPMessageHelper::GetTimeOut(request, timeout))) {
                timeout = 1800;
            }

            // send the info to the service
            service->ProcessNewSubscription(m_TaskManager,
                                            info.local_address, 
                                            strCallbackURLs, 
                                            timeout, 
                                            response);
            return NPT_SUCCESS;
        }
    } else if (strMehod.Compare("UNSUBSCRIBE") == 0) {
        NPT_String strNT, strCallbackURLs, strSID;

        // Do we have a SID
        if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetSID(request, strSID))) {
            // make sure we don't have a callback nor a NT
            if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetNT(request, strNT)) ||
                NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetCallbacks(request, strCallbackURLs))) {
                goto cleanup;
            }

            // subscription cancelled
            // send the info to the service
            service->ProcessCancelSubscription(info.local_address, 
                                               strSID, 
                                               response);
            return NPT_SUCCESS;
        }
    }

cleanup:
    response->SetStatus(400, "Bad Request");
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::ProcessHttpEventRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::ProcessHttpEventRequest(NPT_HttpRequest*   /* request */, 
                                        NPT_SocketInfo     /* info */,
                                        NPT_HttpResponse*  /* response */) 
{ 
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::OnSsdpPacket
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::OnSsdpPacket(NPT_HttpRequest*   request, 
                             NPT_SocketInfo     info)
{
    return ProcessSsdpSearchRequest(request, info);
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::ProcessSsdpSearchRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::ProcessSsdpSearchRequest(NPT_HttpRequest* request, 
                                         NPT_SocketInfo   info) 
{
    // get the address of who sent us some data back*/
    NPT_String  ip_address = info.remote_address.GetIpAddress().ToString();
    NPT_String  strMehod = request->GetMethod();
    NPT_String  url = request->GetUrl().ToRequestString(true);
    NPT_String  strProtocol = request->GetProtocol();

    PLT_Log(PLT_LOG_LEVEL_4, "PLT_DeviceHost Received Request:\r\n");
    PLT_HttpHelper::ToLog(request, PLT_LOG_LEVEL_4);

    if (strMehod.Compare("M-SEARCH") == 0) {
        if (url.Compare("*") || strProtocol.Compare("HTTP/1.1"))
            return NPT_FAILURE;

        PLT_Log(PLT_LOG_LEVEL_4, "PLT_DeviceHost received a M-SEARCH request from %s\n", (const char*) ip_address);

        NPT_String st;
        if (NPT_FAILED(PLT_UPnPMessageHelper::GetST(request, st)))
            return NPT_FAILURE;

        NPT_String MAN;
        if (NPT_FAILED(PLT_UPnPMessageHelper::GetMAN(request, MAN)) || 
            MAN != "\"ssdp:discover\"")
            return NPT_FAILURE;

        long MX;
        if (NPT_FAILED(PLT_UPnPMessageHelper::GetMX(request, MX)) || 
            MX < 0)
            return NPT_FAILURE;

        // create a task to respond to the request
        NPT_TimeInterval timer((MX==0)?0:((int)(0 + ((unsigned short)NPT_System::GetRandomInteger() % ((MX>10)?10:MX)))), 0);
        PLT_SsdpDeviceSearchResponseTask* task = new PLT_SsdpDeviceSearchResponseTask(this, info.remote_address, st);
        m_TaskManager->StartTask(task, &timer);
        return NPT_SUCCESS;
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::SendSsdpSearchResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::SendSsdpSearchResponse(PLT_DeviceData*    device, 
                                       NPT_HttpResponse&  response, 
                                       NPT_UdpSocket&     socket, 
                                       const char*        st,
                                       NPT_SocketAddress* addr /* = NULL */)
{    
    PLT_Log(PLT_LOG_LEVEL_4, "PLT_DeviceHost responding to a M-SEARCH request.\n");

    // ssdp:all or upnp:rootdevice
    if (NPT_String::Compare(st, "ssdp:all") == 0 || NPT_String::Compare(st, "upnp:rootdevice") == 0) {
        if (device->m_Root) {
           // upnp:rootdevice
           PLT_SsdpSender::SendSsdp(response, 
                    NPT_String("uuid:" + device->m_UUID + "::upnp:rootdevice"), 
                    "upnp:rootdevice",
                    socket,
                    false,
                    addr);
        }
    }

    // uuid:device-UUID
    if (NPT_String::Compare(st, "ssdp:all", false) == 0 || NPT_String::Compare(st, "uuid:" + device->m_UUID, false) == 0) {
        // uuid:device-UUID
        PLT_SsdpSender::SendSsdp(response, 
                 "uuid:" + device->m_UUID, 
                 "uuid:" + device->m_UUID, 
                 socket, 
                 false,
                 addr);
    }

    // urn:schemas-upnp-org:device:deviceType:ver
    if (NPT_String::Compare(st, "ssdp:all", false) == 0 || NPT_String::Compare(st, device->m_DeviceType, false) == 0) {
        // uuid:device-UUID::urn:schemas-upnp-org:device:deviceType:ver
        PLT_SsdpSender::SendSsdp(response, 
                 NPT_String("uuid:" + device->m_UUID + "::" + device->m_DeviceType), 
                 device->m_DeviceType,
                 socket,
                 false,
                 addr);
    }

    // services
    for (int i=0; i < (int)device->m_Services.GetItemCount(); i++) {
        if (NPT_String::Compare(st, "ssdp:all", false) == 0 || NPT_String::Compare(st, device->m_Services[i]->GetServiceType(), false) == 0) {
            // uuid:device-UUID::urn:schemas-upnp-org:service:serviceType:ver
            PLT_SsdpSender::SendSsdp(response, 
                     NPT_String("uuid:" + device->m_UUID + "::" + device->m_Services[i]->GetServiceType()), 
                     device->m_Services[i]->GetServiceType(),
                     socket,
                     false,
                     addr);
        }
    }

    // embedded devices
    for (int j=0; j < (int)device->m_EmbeddedDevices.GetItemCount(); j++) {
        SendSsdpSearchResponse(device->m_EmbeddedDevices[j].AsPointer(), response, socket, st, addr);
    }
    
    //m_EmbeddedDevices.Apply(NPT_ProcessSearchResponseIterator<PLT_DeviceHost>(response, stream, st, addr));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceHost::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceHost::OnAction(PLT_ActionReference& action, NPT_SocketInfo* /*info*/ /* = NULL */)
{
    action->SetError(401, "Invalid Action");
    return NPT_FAILURE;
}

