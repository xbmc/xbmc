/*****************************************************************
|
|      Neptune - Network :: BSD Implementation
|
|      (c) 2001-2005 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#if (defined(_WIN32) || defined(_WIN32_WCE) || defined(_XBOX)) && !defined(__SYMBIAN32__)
#if !defined(__WINSOCK__) 
#define __WINSOCK__ 
#endif
#endif

#if defined(__WINSOCK__) && !defined(_XBOX)
#define STRICT
#define NPT_WIN32_USE_WINSOCK2
#ifdef NPT_WIN32_USE_WINSOCK2
/* it is important to include this in this order, because winsock.h and ws2tcpip.h */
/* have different definitions for the same preprocessor symbols, such as IP_ADD_MEMBERSHIP */
#include <winsock2.h>
#include <ws2tcpip.h> 
#else
#include <winsock.h>
#endif
#include <windows.h>

// force a reference to the initializer so that the linker does not optimize it out
#include "NptWin32Network.h" // we need this for the static initializer
static NPT_WinsockSystem& WinsockInitializer = NPT_WinsockSystem::Initializer; 

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#endif

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptNetwork.h"
#include "NptUtils.h"
#include "NptConstants.h"
#include "NptResults.h"
#include "NptSockets.h"

#if defined(NPT_CONFIG_HAVE_GETADDRINFO) 
/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_BSD_NETWORK_MAX_ADDR_LIST_LENGTH = 1024;

/*----------------------------------------------------------------------
|   IPv6 support
+---------------------------------------------------------------------*/
#if defined(NPT_CONFIG_ENABLE_IPV6)
#include <arpa/inet.h>
#endif

/*----------------------------------------------------------------------
|   MapGetAddrInfoErrorCode
+---------------------------------------------------------------------*/
static NPT_Result
MapGetAddrInfoErrorCode(int /*error_code*/)
{
    return NPT_ERROR_HOST_UNKNOWN;
}

/*----------------------------------------------------------------------
|   NPT_NetworkNameResolver::Resolve
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkNameResolver::Resolve(const char*              name, 
                                 NPT_List<NPT_IpAddress>& addresses,
                                 NPT_Timeout              /*timeout*/)
{
    // empty the list first
    addresses.Clear();
    
    
    // get the addr list

    //struct addrinfo hints;
    //NPT_SetMemory(&hints, 0, sizeof(hints));
    //hints.ai_family   = PF_UNSPEC;
    //hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags    = AI_DEFAULT;
    struct addrinfo *infos = NULL;
    int result = getaddrinfo(name,  /* hostname */
                             NULL,  /* servname */
                             NULL,  /* hints    */
                             &infos /* res      */);
    if (result != 0) {
        return MapGetAddrInfoErrorCode(result);
    }
    
    for (struct addrinfo* info = infos; 
                          info && addresses.GetItemCount() < NPT_BSD_NETWORK_MAX_ADDR_LIST_LENGTH;
                          info = info->ai_next) {
        unsigned int expected_length;
        if (info->ai_family == AF_INET) {
            expected_length = sizeof(struct sockaddr_in);
#if defined(NPT_CONFIG_ENABLE_IPV6)
        } else if (info->ai_family == AF_INET6) {
            expected_length = sizeof(struct sockaddr_in6);
#endif
        } else {
            continue;
        }
        if ((unsigned int)info->ai_addrlen < expected_length) continue;
        if (info->ai_protocol != 0 && info->ai_protocol != IPPROTO_TCP) continue;
    
        if (info->ai_family == AF_INET) {
            struct sockaddr_in* inet_addr = (struct sockaddr_in*)info->ai_addr;
            NPT_IpAddress address(ntohl(inet_addr->sin_addr.s_addr));
            addresses.Add(address);
        }
#if defined(NPT_CONFIG_ENABLE_IPV6)
        else if (info->ai_family == AF_INET6) {
            struct sockaddr_in6* inet_addr = (struct sockaddr_in6*)info->ai_addr;
            NPT_IpAddress address(NPT_IpAddress::IPV6, inet_addr->sin6_addr.s6_addr, 16, inet_addr->sin6_scope_id);
            addresses.Add(address);
        }
#endif
    }
    freeaddrinfo(infos);
    
    return NPT_SUCCESS;
}
#endif
