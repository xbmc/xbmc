/*****************************************************************
|
|   Platinum - UPnP
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_UPNP_H_
#define _PLT_UPNP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptHttp.h"
#include "NptList.h"
#include "NptSystem.h"
#include "PltTaskManager.h"
#include "PltCtrlPoint.h"
#include "PltDeviceHost.h"

/*----------------------------------------------------------------------
|   forward definitions
+---------------------------------------------------------------------*/
class PLT_SsdpListenTask;
class NPT_String;

/*----------------------------------------------------------------------
|   NPT_StringFinder
+---------------------------------------------------------------------*/
class NPT_StringFinder
{
public:
    // methods
    NPT_StringFinder(const char* value);
    virtual ~NPT_StringFinder() {}
    bool operator()(const NPT_String* const & value) const;

private:
    // members
    NPT_String   m_Value;
};

/*----------------------------------------------------------------------
|   PLT_UPnPMessageHelper class
+---------------------------------------------------------------------*/
class PLT_UPnPMessageHelper
{
public:
    // methods
    static NPT_Result GetST(NPT_HttpMessage* message, NPT_String& value)           { return message->GetHeaders().GetHeaderValue("ST", value); }
    static void       SetST(NPT_HttpMessage* message, const char* st)              { message->GetHeaders().SetHeader("ST", st); }
    static NPT_Result GetNT(NPT_HttpMessage* message, NPT_String& value)           { return message->GetHeaders().GetHeaderValue("NT", value); }
    static void       SetNT(NPT_HttpMessage* message, const char* nt)              { message->GetHeaders().SetHeader("NT", nt); }
    static NPT_Result GetNTS(NPT_HttpMessage* message, NPT_String& value)          { return message->GetHeaders().GetHeaderValue("NTS", value); }
    static void       SetNTS(NPT_HttpMessage* message, const char* nts)            { message->GetHeaders().SetHeader("NTS", nts); }
    static NPT_Result GetMAN(NPT_HttpMessage* message, NPT_String& value)          { return message->GetHeaders().GetHeaderValue("MAN", value); }
    static void       SetMAN(NPT_HttpMessage* message, const char* man)            { message->GetHeaders().SetHeader("MAN", man); }
    static NPT_Result GetLocation(NPT_HttpMessage* message, NPT_String& value)     { return message->GetHeaders().GetHeaderValue("LOCATION", value); }
    static void       SetLocation(NPT_HttpMessage* message, const char* location)  { message->GetHeaders().SetHeader("LOCATION", location); }
    static NPT_Result GetServer(NPT_HttpMessage* message, NPT_String& value)       { return message->GetHeaders().GetHeaderValue("SERVER", value); }
    static void       SetServer(NPT_HttpMessage* message, const char* server)      { message->GetHeaders().SetHeader("SERVER", server); }
    static NPT_Result GetUSN(NPT_HttpMessage* message, NPT_String& value)          { return message->GetHeaders().GetHeaderValue("USN", value); }
    static void       SetUSN(NPT_HttpMessage* message, const char* usn)            { message->GetHeaders().SetHeader("USN", usn); }
    static NPT_Result GetCallbacks(NPT_HttpMessage* message, NPT_String& value)    { return message->GetHeaders().GetHeaderValue("CALLBACK", value); }
    static void       SetCallbacks(NPT_HttpMessage* message, const char* callbacks){ message->GetHeaders().SetHeader("CALLBACK", callbacks); }
    static NPT_Result GetSID(NPT_HttpMessage* message, NPT_String& value)          { return message->GetHeaders().GetHeaderValue("SID", value); }
    static void       SetSID(NPT_HttpMessage* message, const char* sid)            { message->GetHeaders().SetHeader("SID", sid); }
    static NPT_Result GetLeaseTime(NPT_HttpMessage* message, NPT_Timeout& value) { 
        NPT_String cc; 
        value = 0;
        NPT_Result res = message->GetHeaders().GetHeaderValue("CACHE-CONTROL", cc); 
        if (NPT_FAILED(res)) return res;
        return ExtractLeaseTime(cc, value); 
    }
    static void       SetLeaseTime(NPT_HttpMessage* message, const NPT_Timeout lease) { 
        char age[20]; 
        sprintf(age, "max-age=%d", (int)lease); 
        message->GetHeaders().SetHeader("CACHE-CONTROL", age); 
    }
    static NPT_Result GetTimeOut(NPT_HttpMessage* message, NPT_Timeout& value) { 
        NPT_String cc; 
        value = 0;
        NPT_Result res = message->GetHeaders().GetHeaderValue("TIMEOUT", cc); 
        if (NPT_FAILED(res)) return res;
        return ExtractTimeOut(cc, value); 
    }
    static void       SetTimeOut(NPT_HttpMessage* message, const NPT_Timeout timeout) { 
        char duration[20]; 
        if (timeout >=0) {
            sprintf(duration, "Second-%d", (int)timeout); 
            message->GetHeaders().SetHeader("TIMEOUT", duration); 
        } else {
            message->GetHeaders().SetHeader("TIMEOUT", "Second-infinite"); 
        }
    }
    static NPT_Result GetMX(NPT_HttpMessage* message, long& value) { 
        NPT_String cc; 
        value = 0;
        NPT_Result res = message->GetHeaders().GetHeaderValue("MX", cc);
        if (NPT_FAILED(res)) return res;

        return NPT_ParseInteger(cc, value);
    }
    static void       SetMX(NPT_HttpMessage* message, const long mx) { 
        NPT_String buf = NPT_String::FromInteger(mx);
        message->GetHeaders().SetHeader("MX", buf); 
    }
    static NPT_Result GetSeq(NPT_HttpMessage* message, long& value) { 
        NPT_String cc; 
        value = 0;
        NPT_Result res = message->GetHeaders().GetHeaderValue("SEQ", cc);
        if (NPT_FAILED(res)) return res;

        return NPT_ParseInteger(cc, value);
    }
    static void        SetSeq(NPT_HttpMessage* message, const long seq) { 
        NPT_String buf = NPT_String::FromInteger(seq);
        message->GetHeaders().SetHeader("SEQ", buf); 
    }

    static const char* GenerateUID(int charCount, NPT_String& uuid) {   
        uuid = "";
        for (int i=0;i<(charCount<100?charCount:100);i++) {
            int random = NPT_System::GetRandomInteger();
            uuid += (char)((random % 25) + 66);
        }
        return uuid;
    }

    static NPT_Result ExtractLeaseTime(const char* cache_control, NPT_Timeout& lease) {
        int value;
        if (cache_control && sscanf(cache_control, "max-age=%d", &value) == 1) {
            lease = value;
            return NPT_SUCCESS;
        }
        return NPT_FAILURE;
    }

    static NPT_Result ExtractTimeOut(const char* timeout, NPT_Timeout& len) {
        NPT_String temp = timeout;
        if (temp.CompareN("Second-", 7, true)) {
            return NPT_ERROR_INVALID_FORMAT;
        }

        if (temp.Compare("Second-infinite", true) == 0) {
            len = NPT_TIMEOUT_INFINITE;
            return NPT_SUCCESS;
        }
        return temp.SubString(7).ToInteger(len);
    }
};

/*----------------------------------------------------------------------
|   PLT_UPnP class
+---------------------------------------------------------------------*/
class PLT_UPnP : public PLT_TaskManager
{
public:
    PLT_UPnP(NPT_UInt32 ssdp_port = 1900, bool multicast = true);
    ~PLT_UPnP();

    NPT_Result AddDevice(PLT_DeviceHostReference& device);
    NPT_Result AddCtrlPoint(PLT_CtrlPointReference& ctrlpoint);

    // PLT_TaskManager methods
    NPT_Result Stop();

private:
    NPT_Mutex                           m_Lock;

    NPT_List<PLT_DeviceHostReference>   m_Devices;
    NPT_List<PLT_CtrlPointReference>    m_CtrlPoints;

    // since we can only have one socket listening on port 1900, 
    // we create it in here and we will attach every ctrl points
    // and devices to it when they're added
    PLT_SsdpListenTask*                 m_SsdpListenTask; 
};

#endif // _PLT_UPNP_H 
