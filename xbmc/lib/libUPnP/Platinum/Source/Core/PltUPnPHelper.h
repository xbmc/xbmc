/*****************************************************************
|
|   Platinum - UPnP Helper
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_UPNP_HELPER_H_
#define _PLT_UPNP_HELPER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

//NPT_SET_LOCAL_LOGGER("platinum.core.upnp.helper")

/*----------------------------------------------------------------------
|   NPT_StringFinder
+---------------------------------------------------------------------*/
class NPT_StringFinder
{
public:
    // methods
    NPT_StringFinder(const char* value) : m_Value(value) {}
    virtual ~NPT_StringFinder() {}
    bool operator()(const NPT_String* const & value) const {
        return value->Compare(m_Value) ? false : true;
    }
    bool operator()(const NPT_String& value) const {
        return value.Compare(m_Value) ? false : true;
    }

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

    static const char* GenerateUUID(int charCount, NPT_String& uuid) {   
        uuid = "";
        for (int i=0;i<(charCount<100?charCount:100);i++) {
            int random = NPT_System::GetRandomInteger();
            uuid += (char)((random % 25) + 66);
        }
        return uuid;
    }

    static const char* GenerateGUID(NPT_String& guid) {   
        guid = "";
        for (int i=0;i<32;i++) {
            char nibble = (char)(NPT_System::GetRandomInteger() % 16);
            guid += (nibble < 10) ? ('0' + nibble) : ('a' + (nibble-10));
            if (i == 7 || i == 11 || i == 15 || i == 19) {
                guid += '-';
            }
        }
        return guid;
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
    
    static NPT_Result GetIPAddresses(NPT_List<NPT_String>& ips) {
        NPT_List<NPT_NetworkInterface*> if_list;
        NPT_CHECK(NPT_NetworkInterface::GetNetworkInterfaces(if_list));

        NPT_List<NPT_NetworkInterface*>::Iterator iface = if_list.GetFirstItem();
        while (iface) {
            NPT_String ip = (*(*iface)->GetAddresses().GetFirstItem()).GetPrimaryAddress().ToString();
            if (ip.Compare("0.0.0.0") && ip.Compare("127.0.0.1")) {
                ips.Add(ip);
            }
            ++iface;
        }

        if (ips.GetItemCount() == 0) {
            ips.Add("127.0.0.1");
        }

        if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
        return NPT_SUCCESS;
    }
};

#endif /* _PLT_UPNP_HELPER_H_ */
