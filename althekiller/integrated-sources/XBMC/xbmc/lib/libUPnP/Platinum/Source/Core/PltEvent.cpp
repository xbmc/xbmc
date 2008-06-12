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
    m_EventKey(0),
    m_SubscriberTask(NULL)
{
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriber::~PLT_EventSubscriber
+---------------------------------------------------------------------*/
PLT_EventSubscriber::~PLT_EventSubscriber() 
{
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
    return NPT_ContainerFind(m_CallbackURLs, 
                             NPT_StringFinder(callback_url), 
                             res);
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
PLT_EventSubscriber::Notify(NPT_List<PLT_StateVariable*>& vars)
{
    // verify we have eventable variables
    bool foundVars = false;
    NPT_XmlElementNode* propertyset = new NPT_XmlElementNode("e", "propertyset");
    NPT_CHECK_SEVERE(propertyset->SetNamespaceUri("e", "urn:schemas-upnp-org:event-1-0"));

    NPT_List<PLT_StateVariable*>::Iterator var = vars.GetFirstItem();
    while (var) {
        if ((*var)->IsSendingEvents()) {
            NPT_XmlElementNode* property = new NPT_XmlElementNode("e", "property");
            propertyset->AddChild(property);
            PLT_XmlHelper::AddChildText(property, (*var)->GetName(), (*var)->GetValue());
            foundVars = true;
        }
        ++var;
    }

    // no eventable state variables found!
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
    NPT_HttpRequest* request = new NPT_HttpRequest(url,
                                                   "NOTIFY",
                                                    NPT_HTTP_PROTOCOL_1_0);
    request->GetHeaders().SetHeader(NPT_HTTP_HEADER_CONNECTION, "keep-alive");

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

    PLT_HttpHelper::SetBody(request, xml);

    // start the task now if not started already
    if (!m_SubscriberTask) {
        m_SubscriberTask = new PLT_HttpClientSocketTask(request, true);
        m_TaskManager->StartTask(m_SubscriberTask);
    } else {
        m_SubscriberTask->AddRequest(request);
    }
     
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_EventSubscriberFinderByService::operator()
+---------------------------------------------------------------------*/
bool 
PLT_EventSubscriberFinderByService::operator()(PLT_EventSubscriber* const & eventSub) const 
{
    return m_Service->GetDevice()->GetUUID().Compare(
        eventSub->GetService()->GetDevice()->GetUUID(), true) ? false : true;
}
