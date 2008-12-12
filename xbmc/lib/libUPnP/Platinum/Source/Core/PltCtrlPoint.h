/*****************************************************************
|
|   Platinum - Control Point
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

#ifndef _PLT_CONTROL_POINT_H_
#define _PLT_CONTROL_POINT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltService.h"
#include "PltHttpServerListener.h"
#include "PltSsdpListener.h"
#include "PltDeviceData.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServer;
class PLT_CtrlPointHouseKeepingTask;
class PLT_SsdpSearchTask;
class PLT_SsdpListenTask;

/*----------------------------------------------------------------------
|   PLT_CtrlPointListener class
+---------------------------------------------------------------------*/
class PLT_CtrlPointListener
{
public:
    virtual ~PLT_CtrlPointListener() {}

    virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device) = 0;
    virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device) = 0;
    virtual NPT_Result OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata) = 0;
    virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars) = 0;
};

typedef NPT_List<PLT_CtrlPointListener*> PLT_CtrlPointListenerList;

/*----------------------------------------------------------------------
|   PLT_CtrlPoint class
+---------------------------------------------------------------------*/
class PLT_CtrlPoint : public PLT_SsdpPacketListener,
                      public PLT_SsdpSearchResponseListener
{
public:
    PLT_CtrlPoint(const char* autosearch = "upnp:rootdevice"); // pass NULL to bypass the multicast search

    NPT_Result   AddListener(PLT_CtrlPointListener* listener);
    NPT_Result   RemoveListener(PLT_CtrlPointListener* listener);
    void         IgnoreUUID(const char* uuid);
    NPT_Result   InspectDevice(const char* location, 
                               const char* uuid, 
                               NPT_Timeout leasetime = 1800);
    NPT_Result   Search(const NPT_HttpUrl& url = NPT_HttpUrl("239.255.255.250", 1900, "*"), 
                        const char*        target = "upnp:rootdevice", 
                        NPT_Cardinal       mx = 5);
    NPT_Result   Discover(const NPT_HttpUrl& url = NPT_HttpUrl("239.255.255.250", 1900, "*"), 
                          const char*        target = "ssdp:all", 
                          NPT_Cardinal       mx = 5,
                          NPT_Timeout        repeat = 50000);
    NPT_Result   InvokeAction(PLT_ActionReference& action, void* userdata = NULL);
    NPT_Result   Subscribe(PLT_Service* service, bool cancel = false, void* userdata = NULL);

    // NPT_HttpRequestHandler methods
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request,
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);

    // PLT_SsdpSearchResponseListener methods
    virtual NPT_Result ProcessSsdpSearchResponse(NPT_Result                    res, 
                                                 const NPT_HttpRequestContext& context,
                                                 NPT_HttpResponse*             response);
    // PLT_SsdpPacketListener method
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest&              request, 
                                    const NPT_HttpRequestContext& context);
    
    // helper methods
    NPT_Result  FindDevice(const char* uuid, PLT_DeviceDataReference& device);

protected:
    virtual ~PLT_CtrlPoint();

    NPT_Result   Start(PLT_SsdpListenTask* task);
    NPT_Result   Stop(PLT_SsdpListenTask* task);

    NPT_Result   ProcessSsdpNotify(NPT_HttpRequest&              request, 
                                   const NPT_HttpRequestContext& context);
    NPT_Result   ProcessSsdpMessage(NPT_HttpMessage*              message, 
                                    const NPT_HttpRequestContext& context,  
                                    NPT_String&                   uuid);
    NPT_Result   ProcessGetDescriptionResponse(NPT_Result                    res, 
                                               const NPT_HttpRequestContext& context,
                                               NPT_HttpResponse*             response,
                                               PLT_DeviceDataReference&      device);
    NPT_Result   ProcessGetSCPDResponse(NPT_Result               res, 
                                        NPT_HttpRequest*         request,
                                        NPT_HttpResponse*        response,
                                        PLT_DeviceDataReference& device);
    NPT_Result   ProcessActionResponse(NPT_Result               res, 
                                       NPT_HttpResponse*        response,
                                       PLT_ActionReference&     action,
                                       void*                    userdata);
    NPT_Result   ProcessSubscribeResponse(NPT_Result         res, 
                                          NPT_HttpResponse*  response,
                                          PLT_Service*       service,
                                          void*              userdata);
    NPT_Result   ProcessHttpNotify(NPT_HttpRequest&              request,
                                   const NPT_HttpRequestContext& context,
                                   NPT_HttpResponse&             response);
private:
    // methods
    NPT_Result DoHouseKeeping();
    NPT_Result RemoveDevice(PLT_DeviceDataReference& data);
    NPT_Result ParseFault(PLT_ActionReference& action, NPT_XmlElementNode* fault);
    PLT_SsdpSearchTask* CreateSearchTask(const NPT_HttpUrl&   url, 
                                         const char*          target, 
                                         NPT_Cardinal         mx, 
                                         const NPT_IpAddress& address);
    
private:
    friend class NPT_Reference<PLT_CtrlPoint>;
    friend class PLT_UPnP;
    friend class PLT_UPnP_CtrlPointStartIterator;
    friend class PLT_UPnP_CtrlPointStopIterator;
    friend class PLT_EventSubscriberRemoverIterator;
    friend class PLT_CtrlPointGetDescriptionTask;
    friend class PLT_CtrlPointGetSCPDTask;
    friend class PLT_CtrlPointInvokeActionTask;
    friend class PLT_CtrlPointHouseKeepingTask;
    friend class PLT_CtrlPointSubscribeEventTask;

    NPT_List<NPT_String>                         m_UUIDsToIgnore;
    NPT_Lock<PLT_CtrlPointListenerList>          m_ListenerList;
    PLT_HttpServer*                              m_EventHttpServer;
    NPT_HttpRequestHandler*                      m_EventHttpServerHandler;
    PLT_TaskManager                              m_TaskManager;
    NPT_Lock<NPT_List<PLT_DeviceDataReference> > m_Devices;
    NPT_Lock<NPT_List<PLT_EventSubscriber*> >    m_Subscribers;
    NPT_String                                   m_AutoSearch;
};

typedef NPT_Reference<PLT_CtrlPoint> PLT_CtrlPointReference;

#endif /* _PLT_CONTROL_POINT_H_ */
