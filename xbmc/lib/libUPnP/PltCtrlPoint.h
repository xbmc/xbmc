/*****************************************************************
|
|   Platinum - Control Point
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_CONTROL_POINT_H_
#define _PLT_CONTROL_POINT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDefs.h"
#include "NptStrings.h"
#include "PltService.h"
#include "PltHttpServerListener.h"
#include "PltSsdpListener.h"
#include "PltDeviceData.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServer;
class PLT_CtrlPointHouseKeepingTask;

/*----------------------------------------------------------------------
|   PLT_CtrlPointListener class
+---------------------------------------------------------------------*/
class PLT_CtrlPointListener
{
public:
    virtual ~PLT_CtrlPointListener() {}

    virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device) = 0;
    virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device) = 0;
    virtual NPT_Result OnActionResponse(NPT_Result res, PLT_Action* action, void* userdata) = 0;
    virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars) = 0;
};

typedef NPT_List<PLT_CtrlPointListener*> PLT_CtrlPointListenerList;

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
    PLT_CtrlPointListenerOnActionResponseIterator(NPT_Result res, PLT_Action* action, void* userdata) :
      m_Res(res), m_Action(action), m_Userdata(userdata) {}

    NPT_Result operator()(PLT_CtrlPointListener*& listener) const {
        return listener->OnActionResponse(m_Res, m_Action, m_Userdata);
    }

private:
    NPT_Result  m_Res;
    PLT_Action* m_Action;
    void*       m_Userdata;
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
|   PLT_CtrlPoint class
+---------------------------------------------------------------------*/
class PLT_CtrlPoint : public PLT_HttpServerListener,
                      public PLT_SsdpPacketListener,
                      public PLT_SsdpSearchResponseListener
{
public:
    PLT_CtrlPoint(const char* uuid_to_ignore = NULL, const char* autosearch = "upnp:rootdevice");

    NPT_Result   AddListener(PLT_CtrlPointListener* listener);
    NPT_Result   RemoveListener(PLT_CtrlPointListener* listener);

    NPT_Result   Start(PLT_TaskManager* task_manager);
    NPT_Result   Stop();

    NPT_Result   Search(const NPT_HttpUrl& url = NPT_HttpUrl("239.255.255.250", 1900, "*"), 
                        const char*        target = "upnp:rootdevice", 
                        const NPT_Cardinal MX = 5);
    
    NPT_Result   Discover(const NPT_HttpUrl& url = NPT_HttpUrl("239.255.255.250", 1900, "*"), 
                          const char*        target = "ssdp:all", 
                          const NPT_Cardinal MX = 5,
                          NPT_Timeout        repeat = 50000);
    
    NPT_Result   InvokeAction(PLT_Action* action, PLT_Arguments& arguments, void* userdata = NULL);

    NPT_Result   Subscribe(PLT_Service* service, bool renew = false, void* userdata = NULL);

    // PLT_HttpServerListener methods
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest* request, NPT_SocketInfo info, NPT_HttpResponse*& response);

    // PLT_SsdpSearchResponseListener methods
    NPT_Result   ProcessSsdpSearchResponse(NPT_Result        res, 
                                           NPT_SocketInfo&   info, 
                                           NPT_HttpResponse* response);

    // PLT_SsdpPacketListener method
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest* request, NPT_SocketInfo info);

    // helper methods
    NPT_Result FindDevice(NPT_String& uuid, PLT_DeviceDataReference& device) {
        NPT_AutoLock lock(m_Devices);
        NPT_List<PLT_DeviceDataReference>::Iterator it = m_Devices.Find(PLT_DeviceDataFinder(uuid));
        if (it) {
            device = (*it);
            return NPT_SUCCESS;
        }
        return NPT_FAILURE;
    }

protected:
    virtual ~PLT_CtrlPoint();

    // methods
    NPT_Result   ProcessSsdpNotify(
        NPT_HttpRequest*   request, 
        NPT_SocketInfo     info);

    NPT_Result   ProcessSsdpMessage(
        NPT_HttpMessage*   message, 
        NPT_SocketInfo&    info, 
        NPT_String&        uuid);

    NPT_Result   ProcessGetDescriptionResponse(
        NPT_Result               res, 
        NPT_HttpResponse*        response,
        PLT_DeviceDataReference& device);

    NPT_Result   ProcessGetSCPDResponse(
        NPT_Result               res, 
        NPT_HttpRequest*         request,
        NPT_HttpResponse*        response,
        PLT_DeviceDataReference& device);

    NPT_Result   ProcessActionResponse(
        NPT_Result         res, 
        NPT_HttpResponse*  response,
        PLT_Action*        action,
        void*              userdata);

    NPT_Result   ProcessSubscribeResponse(
        NPT_Result         res, 
        NPT_HttpResponse*  response,
        PLT_Service*       service,
        void*              userdata);

    NPT_Result   DoHouseKeeping();

private:
    // methods
    NPT_Result   ParseFault(PLT_Action* action, NPT_XmlElementNode* fault);

private:
    friend class NPT_Reference<PLT_CtrlPoint>;
    friend class PLT_SsdpCtrlPointSearchTask;
	friend class PLT_CtrlPointGetDescriptionTask;
    friend class PLT_CtrlPointGetSCPDTask;
    friend class PLT_CtrlPointInvokeActionTask;
    friend class PLT_CtrlPointHouseKeepingTask;
    friend class PLT_CtrlPointSubscribeEventTask;

    NPT_AtomicVariable                      m_ReferenceCount;
    NPT_String                              m_UUIDToIgnore;
    PLT_CtrlPointHouseKeepingTask*          m_HouseKeepingTask;
    NPT_Lock<PLT_CtrlPointListenerList>     m_ListenerList;
    PLT_HttpServer*                         m_EventHttpServer;
    PLT_TaskManager*                        m_TaskManager;
    NPT_Lock<NPT_List<PLT_DeviceDataReference> >       m_Devices;
    NPT_List<PLT_EventSubscriber*>          m_Subscribers;
    NPT_String                              m_AutoSearch;
};

typedef NPT_Reference<PLT_CtrlPoint> PLT_CtrlPointReference;

/*----------------------------------------------------------------------
|   PLT_AddGetSCPDRequestIterator class
+---------------------------------------------------------------------*/
class PLT_AddGetSCPDRequestIterator
{
public:
    PLT_AddGetSCPDRequestIterator(PLT_TaskManager*         task_manager, 
                                  PLT_CtrlPoint*           ctrl_point, 
                                  PLT_DeviceDataReference& device,
                                  NPT_HttpUrl              base_url) :
        m_TaskManager(task_manager), m_CtrlPoint(ctrl_point), m_Device(device), m_BaseURL(base_url) {}

    NPT_Result operator()(PLT_Service*& service) const;
    
private:
    PLT_TaskManager*        m_TaskManager;
    PLT_CtrlPoint*          m_CtrlPoint;
    PLT_DeviceDataReference m_Device;
    NPT_HttpUrl             m_BaseURL;
};

/*----------------------------------------------------------------------
|   PLT_ServiceReadyIterator class
+---------------------------------------------------------------------*/
class PLT_ServiceReadyIterator
{
public:
    PLT_ServiceReadyIterator() {}

    NPT_Result operator()(PLT_Service*& service) const;
};

/*----------------------------------------------------------------------
|   PLT_DeviceReadyIterator class
+---------------------------------------------------------------------*/
class PLT_DeviceReadyIterator
{
public:
    PLT_DeviceReadyIterator() {}
    NPT_Result operator()(PLT_DeviceDataReference& device) const;
};

#endif /* _PLT_CONTROL_POINT_H_ */
