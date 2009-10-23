/*****************************************************************
|
|   Platinum - Service
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

#ifndef _PLT_SERVICE_H_
#define _PLT_SERVICE_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltEvent.h"
#include "PltArgument.h"
#include "PltStateVariable.h"
#include "PltAction.h"

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
class PLT_DeviceData;

/*----------------------------------------------------------------------
|    PLT_Service class
+---------------------------------------------------------------------*/
class PLT_Service
{
public:
    // methods
    PLT_Service(PLT_DeviceData* device,
                const char*     type = NULL, 
                const char*     id = NULL,
                const char*     last_change_namespace = NULL);
    ~PLT_Service();
    
    // class methods
    NPT_Result  InitURLs(const char* service_name, const char* device_uuid);
    bool        IsInitted() {
        return (m_ActionDescs.GetItemCount() > 0);
    }
    NPT_Result  PauseEventing(bool paused = true);

    // static methods
    static bool IsTrue(const NPT_String& value) {
        if (value.Compare("1", true)    && 
            value.Compare("true", true) && 
            value.Compare("yes", true)) {
            return false;
        }
        return true;
    }

    // accessor methods
    NPT_String          GetSCPDURL(bool absolute = false);
    NPT_String          GetControlURL(bool absolute = false);
    NPT_String          GetEventSubURL(bool absolute = false);
    const NPT_String&   GetServiceID()    const   { return m_ServiceID;   }
    const NPT_String&   GetServiceType()  const   { return m_ServiceType; }    
    PLT_DeviceData*     GetDevice()               { return m_Device;      }
    NPT_Result          ForceVersion(NPT_Cardinal version);

    // XML
    NPT_Result          GetSCPDXML(NPT_String& xml);
    NPT_Result          SetSCPDXML(const char* xml);
    NPT_Result          GetDescription(NPT_XmlElementNode* parent, NPT_XmlElementNode** service = NULL);

    // State Variables
    NPT_Result          SetStateVariable(const char* name, const char* value);
    NPT_Result          SetStateVariableRate(const char* name, NPT_TimeInterval rate);
	NPT_Result          SetStateVariableExtraAttribute(const char* name, 
                                                       const char* key,
                                                       const char* value);
    NPT_Result          IncStateVariable(const char* name);
    PLT_StateVariable*  FindStateVariable(const char* name);
    NPT_Result          GetStateVariableValue(const char* name, NPT_String& value);
    bool                IsSubscribable();

    // Actions
    PLT_ActionDesc*     FindActionDesc(const char* name);

private:    
    void                Cleanup();
    NPT_Result          AddChanged(PLT_StateVariable* var);
    NPT_Result          UpdateLastChange(NPT_List<PLT_StateVariable*>& vars);
    NPT_Result          NotifyChanged();


    /*----------------------------------------------------------------------
    |    PLT_ServiceEventTask
    +---------------------------------------------------------------------*/
    class PLT_ServiceEventTask : public PLT_ThreadTask
    {
    public:
        PLT_ServiceEventTask(PLT_Service* service) : m_Service(service) {}

        void DoRun() { 
            while (!IsAborting(100)) m_Service->NotifyChanged();
        }

    private:
        PLT_Service* m_Service;
    };

    // Events
    NPT_Result ProcessNewSubscription(
        PLT_TaskManager*         task_manager,
        const NPT_SocketAddress& addr, 
        const NPT_String&        callback_urls, 
        int                      timeout, 
        NPT_HttpResponse&        response);

    NPT_Result ProcessRenewSubscription(
        const NPT_SocketAddress& addr, 
        const NPT_String&        sid, 
        int                      timeout,
        NPT_HttpResponse&        response);
    
    NPT_Result ProcessCancelSubscription(
        const NPT_SocketAddress& addr, 
        const NPT_String&        sid, 
        NPT_HttpResponse&        response);


protected:
    friend class PLT_StateVariable; // so that we can call AddChanged from StateVariable
    friend class PLT_DeviceHost;
    friend class PLT_DeviceData;
    
    //members
    PLT_DeviceData* m_Device;
    NPT_String      m_ServiceType;
    NPT_String      m_ServiceID;
    NPT_String      m_SCPDURL;
    NPT_String      m_ControlURL;
    NPT_String      m_EventSubURL;

    PLT_ServiceEventTask*           m_EventTask;
    NPT_Array<PLT_ActionDesc*>      m_ActionDescs;
    NPT_List<PLT_StateVariable*>    m_StateVars;
    NPT_Mutex                       m_Lock;
    NPT_List<PLT_StateVariable*>    m_StateVarsChanged;
    NPT_List<PLT_StateVariable*>    m_StateVarsToPublish;
    NPT_List<PLT_EventSubscriber*>  m_Subscribers;
    bool                            m_EventingPaused;
    NPT_String                      m_LastChangeNamespace;
};

/*----------------------------------------------------------------------
|    PLT_ServiceSCPDURLFinder
+---------------------------------------------------------------------*/
class PLT_ServiceSCPDURLFinder
{
public:
    // methods
    PLT_ServiceSCPDURLFinder(const char* url) : m_URL(url) {}
    virtual ~PLT_ServiceSCPDURLFinder() {}
    bool operator()(PLT_Service* const & service) const;

private:
    // members
    NPT_String m_URL;
};

/*----------------------------------------------------------------------
|    PLT_ServiceControlURLFinder
+---------------------------------------------------------------------*/
class PLT_ServiceControlURLFinder
{
public:
    // methods
    PLT_ServiceControlURLFinder(const char* url) : m_URL(url) {}
    virtual ~PLT_ServiceControlURLFinder() {}
    bool operator()(PLT_Service* const & service) const;

private:
    // members
    NPT_String m_URL;
};

/*----------------------------------------------------------------------
|    PLT_ServiceEventSubURLFinder
+---------------------------------------------------------------------*/
class PLT_ServiceEventSubURLFinder
{
public:
    // methods
    PLT_ServiceEventSubURLFinder(const char* url) : m_URL(url) {}
    virtual ~PLT_ServiceEventSubURLFinder() {}
    bool operator()(PLT_Service* const & service) const;

private:
    // members
    NPT_String m_URL;
};

/*----------------------------------------------------------------------
|    PLT_ServiceIDFinder
+---------------------------------------------------------------------*/
class PLT_ServiceIDFinder
{
public:
    // methods
    PLT_ServiceIDFinder(const char* id) : m_Id(id) {}
    virtual ~PLT_ServiceIDFinder() {}
    bool operator()(PLT_Service* const & service) const;

private:
    // members
    NPT_String m_Id;
};

/*----------------------------------------------------------------------
|    PLT_ServiceTypeFinder
+---------------------------------------------------------------------*/
class PLT_ServiceTypeFinder
{
public:
    // methods
    PLT_ServiceTypeFinder(const char* type) : m_Type(type) {}
    virtual ~PLT_ServiceTypeFinder() {}
    bool operator()(PLT_Service* const & service) const;

private:
    // members
    NPT_String m_Type;
};

/*----------------------------------------------------------------------
|    PLT_LastChangeXMLIterator
+---------------------------------------------------------------------*/
class PLT_LastChangeXMLIterator
{
public:
    // methods
    PLT_LastChangeXMLIterator(NPT_XmlElementNode* node) : m_Node(node) {}
    virtual ~PLT_LastChangeXMLIterator() {}

    NPT_Result operator()(PLT_StateVariable* const & var) const;

private:
    NPT_XmlElementNode* m_Node;
};

#endif /* _PLT_SERVICE_H_ */
