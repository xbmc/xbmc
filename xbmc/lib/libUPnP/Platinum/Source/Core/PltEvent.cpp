/*****************************************************************
|
|   Platinum - Control/Event
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltTaskManager.h"
#include "PltEvent.h"
#include "PltService.h"
#include "PltUPnP.h"
#include "PltDeviceData.h"
#include "PltXmlHelper.h"

NPT_SET_LOCAL_LOGGER("platinum.core.event")

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::PLT_EventSubscriber
+---------------------------------------------------------------------*/
PLT_EventSubscriber::PLT_EventSubscriber(PLT_TaskManager* task_manager, PLT_Service* service) : 
    m_TaskManager(task_manager), 
    m_Service(service), 
    m_EventKey(0) 
{
    m_SubscriberTask = new PLT_EventSubscriberTask();
    m_TaskManager->StartTask(m_SubscriberTask,0, false);
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::~PLT_EventSubscriber
+---------------------------------------------------------------------*/
PLT_EventSubscriber::~PLT_EventSubscriber() 
{
    m_TaskManager->StopTask(m_SubscriberTask);
    delete m_SubscriberTask;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::GetService
+---------------------------------------------------------------------*/
PLT_Service*
PLT_EventSubscriber::GetService() 
{
    return m_Service;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::GetEventKey
+---------------------------------------------------------------------*/
NPT_Ordinal
PLT_EventSubscriber::GetEventKey() 
{
    return m_EventKey;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::SetEventKey
+---------------------------------------------------------------------*/
NPT_Result
PLT_EventSubscriber::SetEventKey(NPT_Ordinal value) 
{
    m_EventKey = value;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::GetLocalIf
+---------------------------------------------------------------------*/
NPT_SocketAddress
PLT_EventSubscriber::GetLocalIf() 
{
    return m_LocalIf;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::SetLocalIf
+---------------------------------------------------------------------*/
NPT_Result
PLT_EventSubscriber::SetLocalIf(NPT_SocketAddress value) 
{
    m_LocalIf = value;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::GetExpirationTime
+---------------------------------------------------------------------*/
// a TimeStamp of 0 means no expiration
NPT_TimeStamp
PLT_EventSubscriber::GetExpirationTime()
{
    return m_ExpirationTime;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::SetExpirationTime
+---------------------------------------------------------------------*/
NPT_Result
PLT_EventSubscriber::SetExpirationTime(NPT_TimeStamp value) 
{
    m_ExpirationTime = value;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::SetSID
+---------------------------------------------------------------------*/
NPT_Result
PLT_EventSubscriber::SetSID(NPT_String value) 
{
    m_SID = value;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::FindCallbackURL
+---------------------------------------------------------------------*/
NPT_Result
PLT_EventSubscriber::FindCallbackURL(const char* callback_url) 
{
    NPT_String res;
    return NPT_ContainerFind(m_CallbackURLs, NPT_StringFinder(callback_url), res);
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::AddCallbackURL
+---------------------------------------------------------------------*/
NPT_Result
PLT_EventSubscriber::AddCallbackURL(const char* callback_url) 
{
    return m_CallbackURLs.Add(callback_url);
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::Notify
+---------------------------------------------------------------------*/
NPT_Result
PLT_EventSubscriber::Notify(NPT_Array<PLT_StateVariable*>& vars)
{
    // verify we have eventable variables
    bool foundVars = false;
    NPT_XmlElementNode* propertyset = new NPT_XmlElementNode("e", "propertyset");
    NPT_CHECK_SEVERE(propertyset->SetNamespaceUri("e", "urn:schemas-upnp-org:event-1-0"));

    for(unsigned int i=0; i<vars.GetItemCount(); i++) {
        if (vars[i]->IsSendingEvents()) {
            NPT_XmlElementNode* property = new NPT_XmlElementNode("e", "property");
            NPT_CHECK_SEVERE(propertyset->AddChild(property));
            NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(property, vars[i]->GetName(), vars[i]->GetValue()));

            foundVars = true;
        }
    }

    if (foundVars == false) {
        delete propertyset;
        return NPT_FAILURE;
    }

    // parse the callback url
    NPT_HttpUrl url(m_CallbackURLs[0]);
    if (!url.IsValid()) {
        delete propertyset;
        return NPT_FAILURE;
    }

    // format request
    NPT_HttpRequest* request = new NPT_HttpRequest(
        url,
        "NOTIFY",
        "HTTP/1.1");

    // add the extra headers
    PLT_HttpHelper::SetContentType(request, "text/xml");
    PLT_UPnPMessageHelper::SetNT(request, "upnp:event");
    PLT_UPnPMessageHelper::SetNTS(request, "upnp:propchange");
    PLT_UPnPMessageHelper::SetSID(request, m_SID);
    PLT_UPnPMessageHelper::SetSeq(request, m_EventKey);

    // wrap around sequence to 1
    if (++m_EventKey == 0) m_EventKey = 1;
    
    // format the body with the xml
    NPT_String xml;
    if (NPT_FAILED(PLT_XmlHelper::Serialize(*propertyset, xml))) {
        delete propertyset;
        return NPT_FAILURE;
    }
    delete propertyset;

    xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>" + xml;
    PLT_HttpHelper::SetBody(request, xml);

    return m_SubscriberTask->AddRequest(request);;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriberFinderByService::operator()
+---------------------------------------------------------------------*/
bool 
PLT_EventSubscriberFinderByService::operator()(PLT_EventSubscriber* const & eventSub) const 
{
    return m_Service->GetDevice()->GetUUID().Compare(eventSub->GetService()->GetDevice()->GetUUID(), true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriberTask::PLT_EventSubscriberTask()
+---------------------------------------------------------------------*/
PLT_EventSubscriberTask::PLT_EventSubscriberTask()
    : m_Requests(10)
{
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriberTask::~PLT_EventSubscriberTask()
+---------------------------------------------------------------------*/
PLT_EventSubscriberTask::~PLT_EventSubscriberTask()
{
  // delete any outstanding requests
  NPT_HttpRequest* request;
  while (NPT_SUCCEEDED(m_Requests.Pop(request, false))) {
      delete request;
  }
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriberTask::DoRun()
+---------------------------------------------------------------------*/
void PLT_EventSubscriberTask::DoRun()
{
  
    PLT_HttpClient client;

    NPT_OutputStreamReference output_stream;
    NPT_InputStreamReference  input_stream;

    NPT_HttpRequest*               request;

    NPT_String host;
    NPT_UInt16 port = 0;

    
    while (NPT_SUCCEEDED(m_Requests.Pop(request, true))) {
        NPT_SocketInfo                  info;
        NPT_HttpResponse*               response;
        NPT_Reference<NPT_HttpResponse> response_holder;
        NPT_Reference<NPT_HttpRequest>  request_holder(request);

        // check if we should abort
        if(IsAborting(0)) return;

        // create a socket and connection to the host
        if (m_Socket.IsNull() 
        ||  request->GetUrl().GetPort() != port 
        ||  request->GetUrl().GetHost() != host ) {
            m_Socket = new NPT_TcpClientSocket();
            m_Socket->SetReadTimeout(30000);
            m_Socket->SetWriteTimeout(30000);
            NPT_CHECK_LABEL(client.Connect(m_Socket.AsPointer(), *request), failed);
            NPT_CHECK_LABEL(m_Socket->GetOutputStream(output_stream), failed);
            NPT_CHECK_LABEL(m_Socket->GetInputStream(input_stream), failed);
            port = request->GetUrl().GetPort();
            host = request->GetUrl().GetHost();

        }

        NPT_LOG_FINER("PLT_EventSubscriberTask sending:");
        PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, request);
    
        NPT_CHECK_LABEL(client.SendRequest(output_stream, *request), failed);
        NPT_CHECK_LABEL(client.WaitForResponse(input_stream, *request, info, response), failed);
        response_holder = response;        

        NPT_LOG_FINE("PLT_EventSubscriberTask receiving:");
        PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);        

        NPT_CHECK_LABEL(ProcessResponse(request, info, response), failed);

        // if it's no keep alive, we reopen on next attempt
        if (!PLT_HttpHelper::IsConnectionKeepAlive(response)) {
            m_Socket = NULL;
        }

        continue;
failed:
        m_Socket = NULL;
    }

}

void PLT_EventSubscriberTask::DoAbort()
{
    // push null reference, this will abort
    m_Requests.Push(NULL);
    
    // if we could pause the thread, this would be safe
    //if(!m_Socket.IsNull())
    //    m_Socket->Disconnect();
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_EventSubscriberTask::ProcessResponse(NPT_HttpRequest*  request,
                                         NPT_SocketInfo&   info, 
                                         NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);

    NPT_HttpEntity* entity = response->GetEntity();
    NPT_InputStreamReference body;

    // if there is no entity data, we are done
    if (!entity) return NPT_SUCCESS;

    NPT_CHECK_SEVERE(entity->GetInputStream(body));

    // dump body into memory (if no content-length specified, read until disconnection)
    NPT_MemoryStream output;
    NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(*body, output, 0, entity->GetContentLength()));

    return NPT_SUCCESS;
}

