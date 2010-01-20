/*****************************************************************
|
|   Platinum - UPnP Helper
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

#ifndef _PLT_UPNP_HELPER_H_
#define _PLT_UPNP_HELPER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

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
    static const NPT_String* GetST(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("ST"); 
    }
    static NPT_Result SetST(NPT_HttpMessage& message, 
                            const char*      st) { 
        return message.GetHeaders().SetHeader("ST", st); 
    }
    static const NPT_String* GetNT(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("NT"); 
    }
    static NPT_Result SetNT(NPT_HttpMessage& message, 
                            const char*      nt) { 
        return message.GetHeaders().SetHeader("NT", nt); 
    }
    static const NPT_String* GetNTS(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("NTS"); 
    }
    static NPT_Result SetNTS(NPT_HttpMessage& message, 
                             const char*      nts) { 
        return message.GetHeaders().SetHeader("NTS", nts); 
    }
    static const NPT_String* GetMAN(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("MAN"); 
    }
    static NPT_Result SetMAN(NPT_HttpMessage& message, 
                             const char*      man) { 
        return message.GetHeaders().SetHeader("MAN", man); 
    }
    static const NPT_String* GetLocation(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("LOCATION"); 
    }
    static NPT_Result SetLocation(NPT_HttpMessage& message, 
                                  const char*      location) { 
        return message.GetHeaders().SetHeader("LOCATION", location); 
    }
    static const NPT_String* GetServer(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_SERVER); 
    }
    static NPT_Result SetServer(NPT_HttpMessage& message, 
                                const char*      server, 
                                bool             replace = true) { 
        return message.GetHeaders().SetHeader(
            NPT_HTTP_HEADER_SERVER, 
            server, 
            replace); 
    }
    static const NPT_String* GetUSN(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("USN"); 
    }
    static NPT_Result SetUSN(NPT_HttpMessage& message, 
                             const char*      usn) { 
        return message.GetHeaders().SetHeader("USN", usn); 
    }
    static const NPT_String* GetCallbacks(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("CALLBACK"); 
    }
    static NPT_Result SetCallbacks(NPT_HttpMessage& message, 
                                   const char*      callbacks) { 
        return message.GetHeaders().SetHeader("CALLBACK", callbacks); 
    }
    static const NPT_String* GetSID(NPT_HttpMessage& message) { 
        return message.GetHeaders().GetHeaderValue("SID"); 
    }
    static NPT_Result SetSID(NPT_HttpMessage& message, 
                             const char*      sid) { 
        return message.GetHeaders().SetHeader("SID", sid); 
    }
    static NPT_Result GetLeaseTime(NPT_HttpMessage& message, 
                                   NPT_Timeout&     value) { 
        value = 0;
        const NPT_String* cc = 
            message.GetHeaders().GetHeaderValue("CACHE-CONTROL");
        NPT_CHECK_POINTER(cc);
        return ExtractLeaseTime(*cc, value); 
    }
    static NPT_Result SetLeaseTime(NPT_HttpMessage&  message, 
                                   const NPT_Timeout lease) { 
        return message.GetHeaders().SetHeader(
            "CACHE-CONTROL", 
            "max-age="+NPT_String::FromInteger(lease)); 
    }
    static NPT_Result GetTimeOut(NPT_HttpMessage& message, 
                                 NPT_Int32&       value) { 
        value = 0;
        const NPT_String* timeout = 
            message.GetHeaders().GetHeaderValue("TIMEOUT"); 
        NPT_CHECK_POINTER(timeout);
        return ExtractTimeOut(*timeout, value); 
    }
    static NPT_Result SetTimeOut(NPT_HttpMessage& message, 
                                 const NPT_Int32  timeout) { 
        if (timeout >= 0) {
            return message.GetHeaders().SetHeader(
                "TIMEOUT", 
                "Second-"+NPT_String::FromInteger(timeout)); 
        } else {
            return message.GetHeaders().SetHeader(
                "TIMEOUT", "Second-infinite"); 
        }
    }
    static NPT_Result GetMX(NPT_HttpMessage& message, 
                            NPT_UInt32&      value) { 
        value = 0;
        const NPT_String* mx = 
            message.GetHeaders().GetHeaderValue("MX");
        NPT_CHECK_POINTER(mx);
        return NPT_ParseInteger32(*mx, value);
    }
    static NPT_Result SetMX(NPT_HttpMessage& message, 
                            const NPT_UInt32 mx) {
        return message.GetHeaders().SetHeader(
            "MX", 
            NPT_String::FromInteger(mx)); 
    }
    static NPT_Result GetSeq(NPT_HttpMessage& message,  
                             NPT_UInt32&      value) { 
        value = 0;
        const NPT_String* seq = 
            message.GetHeaders().GetHeaderValue("SEQ");
        NPT_CHECK_POINTER(seq);
        return NPT_ParseInteger32(*seq, value);
    }
    static NPT_Result SetSeq(NPT_HttpMessage& message, 
                             const NPT_UInt32 seq) {
        return message.GetHeaders().SetHeader(
            "SEQ", 
            NPT_String::FromInteger(seq)); 
    }
    static const char* GenerateUUID(int         count, 
                                    NPT_String& uuid) {   
        uuid = "";
        for (int i=0;i<(count<100?count:100);i++) {
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
    static NPT_Result ExtractLeaseTime(const char*  cache_control, 
                                       NPT_Timeout& lease) {
        int value;
        if (cache_control && 
            sscanf(cache_control, "max-age=%d", &value) == 1) {
            lease = value;
            return NPT_SUCCESS;
        }
        return NPT_FAILURE;
    }
    static NPT_Result ExtractTimeOut(const char* timeout, 
                                     NPT_Int32&  len) {
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
    static NPT_Result GetIPAddresses(NPT_List<NPT_IpAddress>& ips, 
                                     bool with_localhost = false) {
        NPT_List<NPT_NetworkInterface*> if_list;
        NPT_CHECK(NPT_NetworkInterface::GetNetworkInterfaces(if_list));

        NPT_List<NPT_NetworkInterface*>::Iterator iface = 
            if_list.GetFirstItem();
        while (iface) {
            NPT_IpAddress ip = 
                (*(*iface)->GetAddresses().GetFirstItem()).GetPrimaryAddress();
            if (ip.ToString().Compare("0.0.0.0") && ip.ToString().Compare("127.0.0.1")) {
                ips.Add(ip);
            }
            ++iface;
        }

        if (ips.GetItemCount() == 0 || with_localhost) {
            NPT_IpAddress localhost;
            localhost.Parse("127.0.0.1");
            ips.Add(localhost);
        }

        if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
        return NPT_SUCCESS;
    }

    static NPT_Result GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& if_list, 
                                           bool with_localhost = false) {
        NPT_CHECK(_GetNetworkInterfaces(if_list, false));

        // if no valid interfaces or if requested, add localhost capable interface
        if (if_list.GetItemCount() == 0 || with_localhost) {
            NPT_CHECK(_GetNetworkInterfaces(if_list, true));
        }
        return NPT_SUCCESS;
    }

	static NPT_Result GetMACAddresses(NPT_List<NPT_String>& addresses) {
        NPT_List<NPT_NetworkInterface*> if_list;
        NPT_CHECK(NPT_NetworkInterface::GetNetworkInterfaces(if_list));

        NPT_List<NPT_NetworkInterface*>::Iterator iface = if_list.GetFirstItem();
        while (iface) {
            NPT_String ip = (*(*iface)->GetAddresses().GetFirstItem()).GetPrimaryAddress().ToString();
            if (ip.Compare("0.0.0.0") && ip.Compare("127.0.0.1")) {
				addresses.Add((*iface)->GetMacAddress().ToString());
            }
            ++iface;
        }

        if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
        return NPT_SUCCESS;
    }


	static bool IsLocalNetworkAddress(const NPT_IpAddress& address) {
		if(address.ToString() == "127.0.0.1") return true;

		NPT_List<NPT_NetworkInterface*> if_list;
        NPT_NetworkInterface::GetNetworkInterfaces(if_list);

		NPT_List<NPT_NetworkInterface*>::Iterator iface = if_list.GetFirstItem();
        while (iface) {
			if((*iface)->IsAddressInNetwork(address)) return true;
            ++iface;
        }

		if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
		return false;
	}

private:

    static NPT_Result _GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& if_list, 
                                            bool only_localhost = false) {
        NPT_List<NPT_NetworkInterface*> _if_list;
        NPT_CHECK(NPT_NetworkInterface::GetNetworkInterfaces(_if_list));

        NPT_NetworkInterface* iface;
        while (NPT_SUCCEEDED(_if_list.PopHead(iface))) {
            NPT_String ip = 
                iface->GetAddresses().GetFirstItem()->GetPrimaryAddress().ToString();
            if (ip.Compare("0.0.0.0") && 
                ((!only_localhost && ip.Compare("127.0.0.1")) || 
                 (only_localhost && !ip.Compare("127.0.0.1")))) {
                if_list.Add(iface);

                // add localhost only once
                if (only_localhost) break;
            } else {
                delete iface;
            }
        }

        // cleanup any remaining items in list if we breaked early
        _if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
        return NPT_SUCCESS;
    }
};

#endif /* _PLT_UPNP_HELPER_H_ */
