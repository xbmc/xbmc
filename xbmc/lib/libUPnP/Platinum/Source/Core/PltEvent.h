/*****************************************************************
|
|   Platinum - Event
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_EVENT_H_
#define _PLT_EVENT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttpClientTask.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_StateVariable;
class PLT_DeviceData;
class PLT_Service;
class PLT_TaskManager;
class PLT_EventSubscriberTask;

/*----------------------------------------------------------------------
|   PLT_EventSubscriber class
+---------------------------------------------------------------------*/
class PLT_EventSubscriber
{
public:
    PLT_EventSubscriber(PLT_TaskManager* task_manager, PLT_Service* service);
    ~PLT_EventSubscriber();

    PLT_Service*        GetService();
    NPT_Ordinal         GetEventKey();
    NPT_Result          SetEventKey(NPT_Ordinal value);
    NPT_SocketAddress   GetLocalIf();
    NPT_Result          SetLocalIf(NPT_SocketAddress value);
    NPT_TimeStamp       GetExpirationTime();
    NPT_Result          SetExpirationTime(NPT_TimeStamp value);
    const NPT_String&   GetSID() const { return m_SID; }
    NPT_Result          SetSID(NPT_String value);
    NPT_Result          FindCallbackURL(const char* callback_url);
    NPT_Result          AddCallbackURL(const char* callback_url);
    NPT_Result          Notify(NPT_Array<PLT_StateVariable*>& vars);
    
protected:
    //members
    PLT_TaskManager*       m_TaskManager;
    PLT_EventSubscriberTask* m_SubscriberTask;
    PLT_Service*           m_Service;
    NPT_Ordinal            m_EventKey;
    NPT_String             m_SID;
    NPT_SocketAddress      m_LocalIf;
    NPT_Array<NPT_String>  m_CallbackURLs;
    NPT_TimeStamp          m_ExpirationTime;
};

/*----------------------------------------------------------------------
|   PLT_EventSubscriberFinderBySID
+---------------------------------------------------------------------*/
class PLT_EventSubscriberFinderBySID
{
public:
    // methods
    PLT_EventSubscriberFinderBySID(const char* sid) : m_SID(sid) {}

    bool operator()(const PLT_EventSubscriber* const & sub) const {
        return m_SID.Compare(sub->GetSID(), true) ? false : true;
    }

private:
    // members
    NPT_String m_SID;
};

/*----------------------------------------------------------------------
|   PLT_EventSubscriberFinderByCallbackURL
+---------------------------------------------------------------------*/
class PLT_EventSubscriberFinderByCallbackURL
{
public:
    // methods
    PLT_EventSubscriberFinderByCallbackURL(const char* callback_url) : 
      m_CallbackURL(callback_url) {}

    bool operator()(PLT_EventSubscriber* const & sub) const {
        return NPT_SUCCEEDED(sub->FindCallbackURL(m_CallbackURL));
    }

private:
    // members
    NPT_String m_CallbackURL;
};

/*----------------------------------------------------------------------
|   PLT_EventSubscriberFinderByService
+---------------------------------------------------------------------*/
class PLT_EventSubscriberFinderByService
{
public:
    // methods
    PLT_EventSubscriberFinderByService(PLT_Service* service) : m_Service(service) {}
    virtual ~PLT_EventSubscriberFinderByService() {}
    bool operator()(PLT_EventSubscriber* const & eventSub) const ;

private:
    // members
    PLT_Service* m_Service;
};

/*----------------------------------------------------------------------
|   PLT_EventSubscriberTask class
+---------------------------------------------------------------------*/
class PLT_EventSubscriberTask : public PLT_ThreadTask
{
public:
    PLT_EventSubscriberTask();
    virtual ~PLT_EventSubscriberTask();

    NPT_Result AddRequest(NPT_HttpRequest* request) { return m_Requests.Push(request, false); }
protected:
    // PLT_ThreadTask methods
    virtual void DoAbort();
    virtual void DoRun();

    virtual NPT_Result ProcessResponse(NPT_HttpRequest*  request, 
                                       NPT_SocketInfo&   info, 
                                       NPT_HttpResponse* response);

private:
    NPT_Queue<NPT_HttpRequest> m_Requests;
    NPT_Reference<NPT_Socket>  m_Socket;
};

#endif /* _PLT_EVENT_H_ */
