/*****************************************************************
|
|   Neptune - Network :: Winsock Implementation
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#define STRICT
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NptNetwork.h"
#include "NptWin32Network.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   static initializer
+---------------------------------------------------------------------*/
NPT_WinsockSystem::NPT_WinsockSystem() {
    WORD    wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    /*wVersionRequested = MAKEWORD(1, 1);*/
    WSAStartup( wVersionRequested, &wsaData );
}
NPT_WinsockSystem::~NPT_WinsockSystem() {
    WSACleanup();
}
NPT_WinsockSystem NPT_WinsockSystem::Initializer;

#if defined(_WIN32_WCE)
// don't use the SIO_GET_INTERFACE_LIST on Windows CE, it is
// hopelessly broken, and will crash your application.
#define NPT_NETWORK_USE_IP_HELPER_API
#else
#define NPT_NETWORK_USE_SIO_GET_INTERFACE_LIST
#endif

/*----------------------------------------------------------------------
|   NPT_IpAddress::Any and NPT_IpAddress::Loopback
+---------------------------------------------------------------------*/
const NPT_IpAddress NPT_IpAddress::Any;
const NPT_IpAddress NPT_IpAddress::Loopback(127,0,0,1);

/*----------------------------------------------------------------------
|   NPT_IpAddress::ToString
+---------------------------------------------------------------------*/
NPT_String
NPT_IpAddress::ToString() const
{
    NPT_String address;
    address.Reserve(16);
    address += NPT_String::FromInteger(m_Address[0]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[1]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[2]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[3]);

    return address;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::Parse
+---------------------------------------------------------------------*/
NPT_Result
NPT_IpAddress::Parse(const char* name)
{
    // check the name
    if (name == NULL) return NPT_ERROR_INVALID_PARAMETERS;

    // clear the address
    NPT_SetMemory(&m_Address[0], 0, sizeof(m_Address));

    // parse
    unsigned int  fragment;
    bool          fragment_empty = true;
    unsigned char address[4];
    unsigned int  accumulator;
    for (fragment = 0, accumulator = 0; fragment < 4; ++name) {
        if (*name == '\0' || *name == '.') {
            // fragment terminator
            if (fragment_empty) return NPT_ERROR_INVALID_SYNTAX;
            address[fragment++] = accumulator;
            if (*name == '\0') break;
            accumulator = 0;
            fragment_empty = true;
        } else if (*name >= '0' && *name <= '9') {
            // numerical character
            accumulator = accumulator*10 + (*name - '0');
            if (accumulator > 255) return NPT_ERROR_INVALID_SYNTAX;
            fragment_empty = false; 
        } else {
            // invalid character
            return NPT_ERROR_INVALID_SYNTAX;
        }
    }

    if (fragment == 4 && *name == '\0' && !fragment_empty) {
        m_Address[0] = address[0];
        m_Address[1] = address[1];
        m_Address[2] = address[2];
        m_Address[3] = address[3];
        return NPT_SUCCESS;
    } else {
        return NPT_ERROR_INVALID_SYNTAX;
    }
}

#if defined(NPT_NETWORK_USE_SIO_GET_INTERFACE_LIST)
/*----------------------------------------------------------------------
|   NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{
    // create a socket to talk to the TCP/IP stack
    SOCKET net;
    if((net = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0)) == INVALID_SOCKET) {
        return NPT_FAILURE;
    }

    // get a list of interfaces
    INTERFACE_INFO query[32];  // get up to 32 interfaces 
    DWORD bytes_returned;
    int io_result = WSAIoctl(net, 
                             SIO_GET_INTERFACE_LIST, 
                             NULL, 0, 
                             &query, sizeof(query), 
                             &bytes_returned, 
                             NULL, NULL);
    if (io_result == SOCKET_ERROR) {
        closesocket(net);
        return NPT_FAILURE;
    }

    // we don't need the socket anymore
    closesocket(net);

    // Display interface information
    int interface_count = (bytes_returned/sizeof(INTERFACE_INFO));
    unsigned int iface_index = 0;
    for (int i=0; i<interface_count; i++) {
        SOCKADDR_IN* address;
        NPT_Flags    flags = 0;

        // primary address
        address = (SOCKADDR_IN*)&query[i].iiAddress;
        NPT_IpAddress primary_address(ntohl(address->sin_addr.s_addr));

        // netmask
        address = (SOCKADDR_IN*)&query[i].iiNetmask;
        NPT_IpAddress netmask(ntohl(address->sin_addr.s_addr));

        // broadcast address
        address = (SOCKADDR_IN*)&query[i].iiBroadcastAddress;
        NPT_IpAddress broadcast_address(ntohl(address->sin_addr.s_addr));

        {
            // broadcast address is incorrect
            unsigned char addr[4];
            for(int i=0; i<4; i++) {
                addr[i] = (primary_address.AsBytes()[i] & netmask.AsBytes()[i]) | 
                    ~netmask.AsBytes()[i];
            }
            broadcast_address.Set(addr);
        }

        // ignore interfaces that are not up
        if (!(query[i].iiFlags & IFF_UP)) {
            continue;
        }
        if (query[i].iiFlags & IFF_BROADCAST) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_BROADCAST;
        }
        if (query[i].iiFlags & IFF_MULTICAST) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_MULTICAST;
        }
        if (query[i].iiFlags & IFF_LOOPBACK) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_LOOPBACK;
        }
        if (query[i].iiFlags & IFF_POINTTOPOINT) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_POINT_TO_POINT;
        }

        // mac address (no support for this for now)
        NPT_MacAddress mac;

        // create an interface object
        char iface_name[5];
        iface_name[0] = 'i';
        iface_name[1] = 'f';
        iface_name[2] = '0'+(iface_index/10);
        iface_name[3] = '0'+(iface_index%10);
        iface_name[4] = '\0';
        NPT_NetworkInterface* iface = new NPT_NetworkInterface(iface_name, mac, flags);

        // set the interface address
        NPT_NetworkInterfaceAddress iface_address(
            primary_address,
            broadcast_address,
            NPT_IpAddress::Any,
            netmask);
        iface->AddAddress(iface_address);  
         
        // add the interface to the list
        interfaces.Add(iface);   

        // increment the index (used for generating the name
        iface_index++;
    }

    return NPT_SUCCESS;
}
#elif defined(NPT_NETWORK_USE_IP_HELPER_API)
// Use the IP Helper API
#include <iphlpapi.h>

/*----------------------------------------------------------------------
|   NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{
    IP_ADAPTER_ADDRESSES* iface_list = NULL;
    ULONG                 size = sizeof(IP_ADAPTER_INFO);

    // get the interface table
    for(;;) {
        iface_list = (IP_ADAPTER_ADDRESSES*)malloc(size);
        DWORD result = GetAdaptersAddresses(AF_INET,
                                            0,
                                            NULL,
                                            iface_list, &size);
        if (result == NO_ERROR) {
            break;
        } else {
            // free and try again
            free(iface_list);
            if (result != ERROR_BUFFER_OVERFLOW) {
                return NPT_FAILURE;
            }
        }
    }

    // iterate over the interfaces
    for (IP_ADAPTER_ADDRESSES* iface = iface_list; iface; iface = iface->Next) {
        // skip this interface if it is not up
        if (iface->OperStatus != IfOperStatusUp) continue;

        // get the interface type and mac address
        NPT_MacAddress::Type mac_type;
        switch (iface->IfType) {
            case IF_TYPE_ETHERNET_CSMACD:   mac_type = NPT_MacAddress::TYPE_ETHERNET; break;
            case IF_TYPE_SOFTWARE_LOOPBACK: mac_type = NPT_MacAddress::TYPE_LOOPBACK; break;
            case IF_TYPE_PPP:               mac_type = NPT_MacAddress::TYPE_PPP;      break;
            default:                        mac_type = NPT_MacAddress::TYPE_UNKNOWN;  break;
        }
        NPT_MacAddress mac(mac_type, iface->PhysicalAddress, iface->PhysicalAddressLength);

        // compute interface flags
        NPT_Flags flags = 0;
        if (!(iface->Flags & IP_ADAPTER_NO_MULTICAST)) flags |= NPT_NETWORK_INTERFACE_FLAG_MULTICAST;
        if (iface->IfType == IF_TYPE_SOFTWARE_LOOPBACK) flags |= NPT_NETWORK_INTERFACE_FLAG_LOOPBACK;
        if (iface->IfType == IF_TYPE_PPP) flags |= NPT_NETWORK_INTERFACE_FLAG_POINT_TO_POINT;

        // compute the unicast address (only the first one is supported for now)
        NPT_IpAddress primary_address;
        if (iface->FirstUnicastAddress) {
            if (iface->FirstUnicastAddress->Address.lpSockaddr == NULL) continue;
            if (iface->FirstUnicastAddress->Address.iSockaddrLength != sizeof(SOCKADDR_IN)) continue;
            SOCKADDR_IN* address = (SOCKADDR_IN*)iface->FirstUnicastAddress->Address.lpSockaddr;
            if (address->sin_family != AF_INET) continue;
            primary_address.Set(ntohl(address->sin_addr.s_addr));
        }
        NPT_IpAddress broadcast_address; // not supported yet
        NPT_IpAddress netmask;           // not supported yet

        // convert the interface name to UTF-8
        // BUG in Wine: FriendlyName is NULL
        unsigned int iface_name_length = (unsigned int)iface->FriendlyName?wcslen(iface->FriendlyName):0;
        char* iface_name = new char[4*iface_name_length+1];
        int result = WideCharToMultiByte(
            CP_UTF8, 0, iface->FriendlyName, iface_name_length,
            iface_name, 4*iface_name_length+1,
            NULL, NULL);
        if (result > 0) {
            iface_name[result] = '\0';
        } else {
            iface_name[0] = '\0';
        }

        // create an interface descriptor
        NPT_NetworkInterface* iface_object = new NPT_NetworkInterface(iface_name, mac, flags);
        NPT_NetworkInterfaceAddress iface_address(
            primary_address,
            broadcast_address,
            NPT_IpAddress::Any,
            netmask);
        iface_object->AddAddress(iface_address);  
    
        // cleanup 
        delete[] iface_name;
         
        // add the interface to the list
        interfaces.Add(iface_object);   
    }

    free(iface_list);
    return NPT_SUCCESS;
}
#else
/*----------------------------------------------------------------------
|   NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>&)
{
    return NPT_SUCCESS;
}
#endif
