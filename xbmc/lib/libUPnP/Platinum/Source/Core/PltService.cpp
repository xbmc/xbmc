/*****************************************************************
|
|   Platinum - Service
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltService.h"
#include "PltSsdp.h"
#include "PltUPnP.h"
#include "PltDeviceData.h"
#include "PltXmlHelper.h"

NPT_SET_LOCAL_LOGGER("platinum.core.service")

/*----------------------------------------------------------------------
|   PLT_ServiceSCPDURLFinder::operator()
+---------------------------------------------------------------------*/
bool 
PLT_ServiceSCPDURLFinder::operator()(PLT_Service* const & service) const 
{
    NPT_String url = service->GetSCPDURL();
    if (!url.StartsWith("/")) {
        url = service->GetDevice()->GetURLBase().GetPath() + url;
    }
    return m_URL.Compare(url, true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceControlURLFinder::operator()
+---------------------------------------------------------------------*/
bool 
PLT_ServiceControlURLFinder::operator()(PLT_Service* const & service) const 
{
    NPT_String url = service->GetControlURL();
    if (!url.StartsWith("/")) {
        url = service->GetDevice()->GetURLBase().GetPath() + url;
    }
    return m_URL.Compare(url, true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceEventSubURLFinder::operator()
+---------------------------------------------------------------------*/
bool
PLT_ServiceEventSubURLFinder::operator()(PLT_Service* const & service) const 
{
    NPT_String url = service->GetEventSubURL();
    if (!url.StartsWith("/")) {
        url = service->GetDevice()->GetURLBase().GetPath() + url;
    }
    return m_URL.Compare(url, true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceIDFinder::operator()
+---------------------------------------------------------------------*/
bool
PLT_ServiceIDFinder::operator()(PLT_Service* const & service) const 
{
    return m_id.Compare(service->GetServiceID(), true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceTypeFinder::operator()
+---------------------------------------------------------------------*/
bool 
PLT_ServiceTypeFinder::operator()(PLT_Service* const & service) const 
{
    return m_type.Compare(service->GetServiceType(), true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_GetLastChangeXMLIterator::operator()
+---------------------------------------------------------------------*/
NPT_Result
PLT_LastChangeXMLIterator::operator()(PLT_StateVariable* const &var) const
{   
    if(var->IsSendingEvents()) return NPT_SUCCESS;
    NPT_XmlElementNode* variable = new NPT_XmlElementNode((const char*)var->GetName());
    NPT_CHECK_SEVERE(m_Node->AddChild(variable));
    NPT_CHECK_SEVERE(variable->SetAttribute("val", var->GetValue()));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::PLT_Service
+---------------------------------------------------------------------*/
PLT_Service::PLT_Service(PLT_DeviceData* device,
                         const char*     type, 
                         const char*     id) :  
    m_Device(device),
    m_ServiceType(type),
    m_ServiceID(id)
{
}

/*----------------------------------------------------------------------
|   PLT_Service::~PLT_Service
+---------------------------------------------------------------------*/
PLT_Service::~PLT_Service()
{
    Cleanup();
}

/*----------------------------------------------------------------------
 |   PLT_Service::~PLT_Service
 +---------------------------------------------------------------------*/
 void
 PLT_Service::Cleanup()
 {
     m_ActionDescs.Apply(NPT_ObjectDeleter<PLT_ActionDesc>());
     m_StateVariables.Apply(NPT_ObjectDeleter<PLT_StateVariable>());
     m_Subscribers.Apply(NPT_ObjectDeleter<PLT_EventSubscriber>());
 }

/*----------------------------------------------------------------------
|   PLT_Service::GetSCPDXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::GetSCPDXML(NPT_String& scpd)
{
    // it is required to have at least 1 state variable
    if (m_StateVariables.GetItemCount() == 0) return NPT_FAILURE;

    NPT_XmlElementNode* top = new NPT_XmlElementNode("scpd");
    NPT_CHECK_SEVERE(top->SetNamespaceUri("", "urn:schemas-upnp-org:service-1-0"));

    // add spec version
    NPT_XmlElementNode* spec = new NPT_XmlElementNode("specVersion");
    NPT_CHECK_SEVERE(top->AddChild(spec));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(spec, "major", "1"));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(spec, "minor", "0"));

    // add actions
    NPT_XmlElementNode* actionList = new NPT_XmlElementNode("actionList");
    NPT_CHECK_SEVERE(top->AddChild(actionList));
    NPT_CHECK_SEVERE(m_ActionDescs.ApplyUntil(PLT_GetSCPDXMLIterator<PLT_ActionDesc>(actionList), 
        NPT_UntilResultNotEquals(NPT_SUCCESS)));

    // add service state table
    NPT_XmlElementNode* serviceStateTable = new NPT_XmlElementNode("serviceStateTable");
    NPT_CHECK_SEVERE(top->AddChild(serviceStateTable));
    NPT_CHECK_SEVERE(m_StateVariables.ApplyUntil(PLT_GetSCPDXMLIterator<PLT_StateVariable>(serviceStateTable), 
        NPT_UntilResultNotEquals(NPT_SUCCESS)));

    // serialize node
    NPT_String tmp;
    NPT_CHECK_SEVERE(PLT_XmlHelper::Serialize(*top, tmp));
    delete top;

    // add preprocessor
    scpd = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" + tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::ToXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::GetDescription(NPT_XmlElementNode* parent, NPT_XmlElementNode** service_out /* = NULL */)
{
    NPT_XmlElementNode* service = new NPT_XmlElementNode("service");
    if (service_out) {
        *service_out = service;
    }
    NPT_CHECK_SEVERE(parent->AddChild(service));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "serviceType", m_ServiceType));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "serviceId", m_ServiceID));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "SCPDURL", GetSCPDURL()));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "controlURL", GetControlURL()));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "eventSubURL", GetEventSubURL()));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::InitURLs
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::InitURLs(const char* service_name, 
                      const char* device_uuid)
{
    m_SCPDURL = service_name + NPT_String("/scpd.xml");
    m_SCPDURL += "?uuid=" + NPT_String(device_uuid);
    m_ControlURL = service_name + NPT_String("/control.xml");
    m_ControlURL += "?uuid=" + NPT_String(device_uuid);
    m_EventSubURL = service_name + NPT_String("/event.xml");
    m_EventSubURL += "?uuid=" + NPT_String(device_uuid);
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::SetSCPDXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::SetSCPDXML(const char* scpd)
{
    if (scpd == NULL) return NPT_FAILURE;

    Cleanup();

    NPT_XmlParser parser;
    NPT_XmlNode*  tree = NULL;
    NPT_Result    res;
    NPT_Array<NPT_XmlElementNode*> stateVariables;
    NPT_Array<NPT_XmlElementNode*> actions;

    res = parser.Parse(scpd, tree);
    if (NPT_FAILED(res)) {
        delete tree;
        return res;
    }

    // make sure root tag is right
    NPT_XmlElementNode* root = tree->AsElementNode();
    if (!root || NPT_String::Compare(root->GetTag(), "scpd")) {
        delete tree;
        return NPT_FAILURE;
    }

    // make sure we have required children presents
    NPT_XmlElementNode* actionList = PLT_XmlHelper::GetChild(root, "actionList");
    NPT_XmlElementNode* stateTable = PLT_XmlHelper::GetChild(root, "serviceStateTable");
    if (!actionList || !stateTable || !actionList->GetChildren().GetItemCount() || !stateTable->GetChildren().GetItemCount()) {
        goto failure;
    }

    // stateVariable table
    if (NPT_FAILED(PLT_XmlHelper::GetChildren(stateTable, stateVariables, "stateVariable"))) {
        goto failure;
    }

    for( int k = 0 ; k < (int)stateVariables.GetItemCount(); k++)
    {
        NPT_String name, type, send;
        PLT_XmlHelper::GetChildText(stateVariables[k], "name", name);
        PLT_XmlHelper::GetChildText(stateVariables[k], "dataType", type);
        PLT_XmlHelper::GetAttribute(stateVariables[k], "sendEvents", send);
        if (name.GetLength() == 0 || type.GetLength() == 0) {
            goto failure;
        }
        PLT_StateVariable* variable = new PLT_StateVariable(this);
        m_StateVariables.Add(variable);

        variable->m_Name = name;
        variable->m_DataType = type;
        variable->m_IsSendingEvents = IsTrue(send); // could it be true/false ?
        PLT_XmlHelper::GetChildText(stateVariables[k], "defaultValue", variable->m_DefaultValue);

        NPT_XmlElementNode* allowedValueList = PLT_XmlHelper::GetChild(stateVariables[k], "allowedValueList");
        if (allowedValueList) {
            NPT_Array<NPT_XmlElementNode*> allowedValues;
            PLT_XmlHelper::GetChildren(allowedValueList, allowedValues, "allowedValue");
            for( int l = 0 ; l < (int)allowedValues.GetItemCount(); l++) {
                const NPT_String* text = allowedValues[l]->GetText();
                if (text) {
                    variable->m_AllowedValues.Add(new NPT_String(*text));
                }
            }
        } else {
            NPT_XmlElementNode* allowedValueRange = PLT_XmlHelper::GetChild(stateVariables[k], "allowedValueRange");
            if (allowedValueRange) {
                NPT_String min, max, step;
                PLT_XmlHelper::GetChildText(allowedValueRange, "minimum", min);
                PLT_XmlHelper::GetChildText(allowedValueRange, "maximum", max);
                PLT_XmlHelper::GetChildText(allowedValueRange, "step", step);
                if (min.GetLength() == 0 || max.GetLength() == 0) {
                    goto failure;
                }
                variable->m_AllowedValueRange = new NPT_AllowedValueRange;
                NPT_ParseInteger(min, variable->m_AllowedValueRange->min_value);
                NPT_ParseInteger(max, variable->m_AllowedValueRange->max_value);
                if (step.GetLength() != 0) {
                    NPT_ParseInteger(step, variable->m_AllowedValueRange->step);
                }
            }
        }
    }

    // actions
    if (NPT_FAILED(PLT_XmlHelper::GetChildren(actionList, actions, "action"))) {
        goto failure;
    }

    for( int i = 0 ; i < (int)actions.GetItemCount(); i++)
    {
        NPT_String action_name;
        PLT_XmlHelper::GetChildText(actions[i],  "name", action_name);

        // action arguments
        NPT_XmlElementNode* argumentList = PLT_XmlHelper::GetChild(actions[i], "argumentList");
        if (action_name.GetLength() == 0 || argumentList == NULL || !argumentList->GetChildren().GetItemCount()) {
            goto failure;
        }

        PLT_ActionDesc* action_desc = new PLT_ActionDesc(action_name, this);
        m_ActionDescs.Add(action_desc);

        NPT_Array<NPT_XmlElementNode*> arguments;
        NPT_CHECK_SEVERE(PLT_XmlHelper::GetChildren(argumentList, arguments, "argument"));
        bool foundRetValue = false;
        for( int j = 0 ; j < (int)arguments.GetItemCount(); j++)
        {
            NPT_String name, direction, relatedStateVar;
            PLT_XmlHelper::GetChildText(arguments[j], "name", name);
            PLT_XmlHelper::GetChildText(arguments[j], "direction", direction);
            PLT_XmlHelper::GetChildText(arguments[j], "relatedStateVariable", relatedStateVar);
            if (name.GetLength() == 0 || direction.GetLength() == 0 || relatedStateVar.GetLength() == 0) {
                goto failure;
            }

            // make sure the related state variable exists
            PLT_StateVariable* variable = FindStateVariable(relatedStateVar);
            if (variable == NULL) {
                goto failure;
            }

            bool bReturnValue = false;
            NPT_XmlElementNode* retval_node = PLT_XmlHelper::GetChild(arguments[j], "retVal");
            if (retval_node) {
                // verify this is the only retVal we've had
                if (foundRetValue) {
                    goto failure;
                } else {
                    bReturnValue = true;
                    foundRetValue = true;
                }
            }
            action_desc->GetArgumentDescs().Add(new PLT_ArgumentDesc(name, direction, variable, bReturnValue));
        }
    }

    // delete the tree
    delete tree;

    return NPT_SUCCESS;

failure:
    delete tree;
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_Service::FindActionDesc
+---------------------------------------------------------------------*/
PLT_ActionDesc*
PLT_Service::FindActionDesc(const char* name)
{
    PLT_ActionDesc* action = NULL;
    NPT_ContainerFind(m_ActionDescs, PLT_ActionDescNameFinder(this, name), action);
    return action;
}

/*----------------------------------------------------------------------
|   PLT_Service::FindStateVariable
+---------------------------------------------------------------------*/
PLT_StateVariable*
PLT_Service::FindStateVariable(const char* name)
{
    PLT_StateVariable* stateVariable = NULL;
    NPT_ContainerFind(m_StateVariables, PLT_StateVariableNameFinder(name), stateVariable);
    return stateVariable;
}

/*----------------------------------------------------------------------
|   PLT_Service::IsSubscribable
+---------------------------------------------------------------------*/
bool
PLT_Service::IsSubscribable()
{
    for(NPT_Cardinal i=0; i<m_StateVariables.GetItemCount(); i++) {
        if (m_StateVariables[i]->m_IsSendingEvents)
            return true;
    }
    return false;
}

/*----------------------------------------------------------------------
|   PLT_Service::SetStateVariable
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::SetStateVariable(const char* name, const char* value, bool publish)
{
    PLT_StateVariable* stateVariable = NULL;
    NPT_ContainerFind(m_StateVariables, PLT_StateVariableNameFinder(name), stateVariable);
    if (stateVariable == NULL)
        return NPT_FAILURE;

    stateVariable->SetValue(value, publish);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::IncStateVariable
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::IncStateVariable(const char* name, bool publish)
{
    PLT_StateVariable* stateVariable = NULL;
    NPT_ContainerFind(m_StateVariables, PLT_StateVariableNameFinder(name), stateVariable);
    if (stateVariable == NULL)
        return NPT_FAILURE;

    NPT_String value = stateVariable->GetValue();
    long num;
    if (value.GetLength() == 0 || NPT_FAILED(value.ToInteger(num))) {
        return NPT_FAILURE;
    }

    // convert value to int
    return stateVariable->SetValue(NPT_String::FromInteger(num+1), publish);
}

/*----------------------------------------------------------------------
|   PLT_Service::ProcessNewSubscription
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::ProcessNewSubscription(PLT_TaskManager*   task_manager,
                                    NPT_SocketAddress& addr, 
                                    NPT_String&        callback_urls, 
                                    int                timeout, 
                                    NPT_HttpResponse&  response)
{
//    // first look if we don't have a subscriber with same callbackURL
//    PLT_EventSubscriber* subscriber = NULL;
//    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderByCallbackURL(strCallbackURL),
//        subscriber))) {
//        // update local interface and timeout
//        subscriber->m_local_if.SetIpAddress((unsigned long) addr.GetIpAddress());
//        subscriber->m_ExpirationTime = NPT_Time(NULL) + timeout;
//
//        PLT_UPnPMessageHelper::SetSID("uuid:" + subscriber->m_SID);
//        PLT_UPnPMessageHelper::SetTimeOut(timeout);
//        return NPT_SUCCESS;
//    }
//
    // reject if we have too many subscribers already
    if (m_Subscribers.GetItemCount() > 30) {
        response.SetStatus(500, "Internal Server Error");
        return NPT_FAILURE;
    }

    PLT_EventSubscriber* subscriber = new PLT_EventSubscriber(task_manager, this);
    // parse the callback URLs
    bool reachable = false;
    if (callback_urls[0] == '<') {
        char* szURLs = (char*)(const char*)callback_urls;
        char* brackL = szURLs;
        char* brackR = szURLs;
        while (++brackR < szURLs + callback_urls.GetLength()) {
            if (*brackR == '>') {
                NPT_String strCallbackURL = NPT_String(brackL+1, (NPT_Size)(brackR-brackL-1));
                NPT_HttpUrl url(strCallbackURL);
                if (url.IsValid()) {
                    subscriber->AddCallbackURL(strCallbackURL);
                    reachable = true;
                }
                brackL = ++brackR;
            }
        }
    }

    if (reachable == false) {
        response.SetStatus(412, "Precondition Failed");
        return NPT_FAILURE;
    }

    // keep track of which interface we receive the request, we will use this one
    // when notifying
    subscriber->SetLocalIf(addr);

    // keep track of subscriber lifetime
    // -1 means infinite so we set an expiration time of 0
    if (timeout == -1) {
        subscriber->SetExpirationTime(NPT_TimeStamp(0, 0));
    } else {
        NPT_TimeStamp life;
        NPT_System::GetCurrentTimeStamp(life);
        life += NPT_TimeInterval(timeout, 0);
        subscriber->SetExpirationTime(life);    
    }

    // generate a unique subscriber ID
    NPT_String sid;
    PLT_UPnPMessageHelper::GenerateUUID(19, sid);
    subscriber->SetSID("uuid:" + sid);

    PLT_UPnPMessageHelper::SetSID(&response, subscriber->GetSID());
    PLT_UPnPMessageHelper::SetTimeOut(&response, timeout);

    // we must set LastChanged state to all non evented
    // variables, by doing this first, we will get all data
    // in one event to the new subscriber
    m_StateChanged = m_StateVariables;
    NotifyChanged();

    m_Subscribers.Add(subscriber);
    subscriber->Notify(m_StateVariables);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::ProcessRenewSubscription
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::ProcessRenewSubscription(NPT_SocketAddress& addr, 
                                      NPT_String&        sid, 
                                      int                timeout, 
                                      NPT_HttpResponse&  response)
{
    // first look if we don't have a subscriber with same callbackURL
    PLT_EventSubscriber* subscriber = NULL;
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderBySID(sid), subscriber))) {
        // update local interface and timeout
        subscriber->SetLocalIf(addr);

        // keep track of subscriber lifetime
        // -1 means infinite so we set an expiration time of 0
        if (timeout == -1) {
            subscriber->SetExpirationTime(NPT_TimeStamp(0, 0));
        } else {
            NPT_TimeStamp life;
            NPT_System::GetCurrentTimeStamp(life);
            life += NPT_TimeInterval(timeout, 0);
            subscriber->SetExpirationTime(life);
        }

        PLT_UPnPMessageHelper::SetSID(&response, subscriber->GetSID());
        PLT_UPnPMessageHelper::SetTimeOut(&response, timeout);
        return NPT_SUCCESS;
    }

    // didn't find a valid Subscriber in our list
    response.SetStatus(412, "Precondition Failed");
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_Service::ProcessCancelSubscription
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::ProcessCancelSubscription(NPT_SocketAddress& /* addr */, 
                                       NPT_String&        sid, 
                                       NPT_HttpResponse&  response)
{
    // first look if we don't have a subscriber with same callbackURL
    PLT_EventSubscriber* subscriber = NULL;
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderBySID(sid), subscriber))) {

        // update local interface and timeout
        m_Subscribers.Remove(subscriber);
        delete subscriber; // what if a task is sending info to it at the time, does it use the sub?
        return NPT_SUCCESS;
    }

    // didn't find a valid Subscriber in our list
    response.SetStatus(412, "Precondition Failed");
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_Service::NotifySubs
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::NotifySubs(PLT_StateVariable* var /* = NULL */)
{
    // no params means notify all variable states to all subs
    NPT_Array<PLT_StateVariable*>* vars;
    NPT_Array<PLT_StateVariable*>  OneVarVector;
    if (var) {
        OneVarVector.Add(var);
        vars = &OneVarVector;
    } else {
        vars = &m_StateVariables;
    }
    
    PLT_EventSubscriber* sub;
    int i = 0;
    int count = m_Subscribers.GetItemCount();
    while (i++ < count) {
        // forget sub if it didn't renew in time or if notification failed
        if (NPT_SUCCEEDED(m_Subscribers.PopHead(sub))) {
            NPT_TimeStamp now, expiration;
            NPT_System::GetCurrentTimeStamp(now);
            expiration = sub->GetExpirationTime();

            sub->Notify(*vars);

            // for now subscribers never expire
            // renabled to keep down overhead  -- elupus
            if (expiration != NPT_TimeStamp() && expiration < now) {
                delete sub;
            } else {
                // add back subscriber to end of list
                m_Subscribers.Add(sub);
            }
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::NotifySub
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::NotifySub(PLT_EventSubscriber* sub, PLT_StateVariable* var /* = NULL */)
{
    // no params means notify all variable states to all subs
    NPT_Array<PLT_StateVariable*>* vars;
    NPT_Array<PLT_StateVariable*>  OneVarVector;
    if (var) {
        OneVarVector.Add(var);
        vars = &OneVarVector;
    } else {
        vars = &m_StateVariables;
    }

    return sub->Notify(*vars);
}

/*----------------------------------------------------------------------
|   PLT_Service::AddChanged
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::AddChanged(PLT_StateVariable* var /* = NULL */)
{
    if (var->IsSendingEvents()) {
        return NotifySubs(var);
    }

    if (!m_StateChanged.Contains(var)) {
        m_StateChanged.Add(var);
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::NotifyChanged
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::NotifyChanged()
{
    PLT_StateVariable* var = FindStateVariable("LastChange");
    if (var == NULL)
        return NPT_FAILURE;

    if (m_StateChanged.GetItemCount() == 0)
        return NPT_SUCCESS;

    NPT_XmlElementNode* top = new NPT_XmlElementNode("Event");
    NPT_CHECK_SEVERE(top->SetNamespaceUri("", "urn:schemas-upnp-org:metadata-1-0/AVT_RCS"));

    NPT_XmlElementNode* instance = new NPT_XmlElementNode("InstanceID");
    NPT_CHECK_SEVERE(top->AddChild(instance));
    NPT_CHECK_SEVERE(instance->SetAttribute("val", "0"));
    
    // build list of changes
    NPT_CHECK_SEVERE(m_StateChanged.Apply(PLT_LastChangeXMLIterator(instance)));
    m_StateChanged.Clear();

     // serialize node
    NPT_String tmp;
    NPT_CHECK_SEVERE(PLT_XmlHelper::Serialize(*top, tmp));
    delete top;

    // add preprocessor
    tmp.Insert("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n", 0);

    // set the state change
    var->SetValue((const char*)tmp, true);
    return NPT_SUCCESS;
}