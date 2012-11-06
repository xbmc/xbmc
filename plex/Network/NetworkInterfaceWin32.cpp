#ifdef _WIN32

/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Jan 3, 2011
 *      Author: Elan Feingold
 */

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include "Log.h"
#include "Network/NetworkInterface.h"

#define kIPv6IfIndexBase (10000000L)

using namespace boost;

// The events.
static HANDLE cancelEvent;
static HANDLE interfaceListChangedEvent;
static SOCKET interfaceListChangedSocket;
static HANDLE tcpipChangedEvent;
static HKEY   hTcpKey;
static HANDLE addrChangeEvent;

// Static initializations.
vector<NetworkInterface::callback_function> NetworkInterface::g_observers;
vector<NetworkInterface> NetworkInterface::g_interfaces;
boost::mutex NetworkInterface::g_mutex;

struct ifaddrs
{
	struct ifaddrs* ifa_next;
	char* ifa_name;
	u_int ifa_flags;
	struct sockaddr	*	ifa_addr;
	struct sockaddr	*	ifa_netmask;
	struct sockaddr	*	ifa_broadaddr;
	struct sockaddr	*	ifa_dstaddr;
	BOOL ifa_dhcpEnabled;
	time_t ifa_dhcpLeaseExpires;
	void * ifa_data;
	
	struct
	{
		uint32_t index;
	}	ifa_extra;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkChanged()
{
  dprintf("Network change.");
  NetworkInterface::NotifyOfNetworkChange();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void SetupNotifications()
{
  // Cancel event.
  cancelEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  
  // TCP/IP change event.
  RegCreateKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), &hTcpKey);
  tcpipChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	RegNotifyChangeKeyValue(hTcpKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, tcpipChangedEvent, TRUE);

  // Socket.
  interfaceListChangedSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // Make it non-blocking.
  unsigned long param = 1;
	::ioctlsocket(interfaceListChangedSocket, FIONBIO, &param);

 	int inBuffer = 0;
	int	outBuffer = 0;
	DWORD	outSize = 0;

  // Watch for address list changes and turn it into something we can watch with an event.
  interfaceListChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  DWORD err = WSAIoctl(interfaceListChangedSocket, SIO_ADDRESS_LIST_CHANGE, &inBuffer, 0, &outBuffer, 0, &outSize, NULL, NULL);
  err = WSAEventSelect(interfaceListChangedSocket, interfaceListChangedEvent, FD_ADDRESS_LIST_CHANGE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void DestroyNotifications()
{
  CloseHandle(cancelEvent);
  cancelEvent = 0;

  CloseHandle(interfaceListChangedEvent);
  interfaceListChangedEvent = 0;

  CloseHandle(tcpipChangedEvent);
  tcpipChangedEvent = 0;

  CloseHandle(addrChangeEvent);
  addrChangeEvent = 0;

  closesocket(interfaceListChangedSocket);
  interfaceListChangedSocket = 0;

  RegCloseKey(hTcpKey);
  hTcpKey = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void RunWatchingForChanges()
{
	HANDLE waitList[3];

  for (bool done = false; done == false; )
  {
    // Set up handles.
    waitList[0] = cancelEvent;
	  waitList[1] = tcpipChangedEvent;
	  waitList[2] = interfaceListChangedEvent;
	
    // Wait for something to handle.
    DWORD result = WaitForMultipleObjects(3, waitList, FALSE, INFINITE);
    if (result == WAIT_OBJECT_0)
    {
      done = true;
    }
    else if (result == WAIT_OBJECT_0 + 1)
    {
      // Reset and notify.
      RegNotifyChangeKeyValue(hTcpKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, tcpipChangedEvent, TRUE);
      DestroyNotifications();
      SetupNotifications();
      NetworkChanged();
    }
    else if (result == WAIT_OBJECT_0 + 2)
    {
      // Sleep (GetAdaptersAddresses doesn't always stay in sync after network changed events) and notify.
      DestroyNotifications();
      SetupNotifications();
      NetworkChanged();
    }
    else
    {
      Sleep(100);
    }
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkInterface::WatchForChanges()
{
  // Get the initial list of interfaces.
  NotifyOfNetworkChange();

  // Setup the notifications.
  SetupNotifications();

  // Start the thread.
  thread t = thread(boost::bind(&RunWatchingForChanges));
  t.detach();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool AddressToIndexAndMask(struct sockaddr* addr, uint32_t* ifIndex, struct sockaddr* mask)
{
  bool ret = false;

  // For now, this is only for IPv4 addresses.
  if (addr->sa_family != AF_INET)
    return false;

	// Make an initial call to GetIpAddrTable to get the necessary size into the dwSize variable.
  PMIB_IPADDRTABLE pIPAddrTable	= 0;
  DWORD	dwSize = 0;

	for (int i = 0; i < 100; i++)
	{
    // Figure out the size.  
		DWORD err = GetIpAddrTable(pIPAddrTable, &dwSize, 0 );
		if (err != ERROR_INSUFFICIENT_BUFFER)
			break;

    // Allocate the right size.
		pIPAddrTable = (MIB_IPADDRTABLE *)realloc(pIPAddrTable, dwSize);
		if (err == WSAENOBUFS)
      return false;
	}

	for (size_t i=0; i<pIPAddrTable->dwNumEntries; i++)
	{
		if (((struct sockaddr_in* )addr)->sin_addr.s_addr == pIPAddrTable->table[i].dwAddr)
		{
      // Grab the index and the netmask.
			*ifIndex = pIPAddrTable->table[i].dwIndex;
			((struct sockaddr_in* )mask)->sin_addr.s_addr = pIPAddrTable->table[i].dwMask;
			ret = true;
			break;
		}
	}

	if (pIPAddrTable)
		free (pIPAddrTable);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void freeifaddrs(struct ifaddrs *inIFAs)
{
	struct ifaddrs* p;
	struct ifaddrs*	q;
	
	// Free each piece of the structure. Set to null after freeing to handle macro-aliased fields.
	for (p = inIFAs; p; p = q )
	{
		q = p->ifa_next;
		
		if( p->ifa_name )
		{
			free( p->ifa_name );
			p->ifa_name = NULL;
		}
		if( p->ifa_addr )
		{
			free( p->ifa_addr );
			p->ifa_addr = NULL;
		}
		if( p->ifa_netmask )
		{
			free( p->ifa_netmask );
			p->ifa_netmask = NULL;
		}
		if( p->ifa_broadaddr )
		{
			free( p->ifa_broadaddr );
			p->ifa_broadaddr = NULL;
		}
		if( p->ifa_dstaddr )
		{
			free( p->ifa_dstaddr );
			p->ifa_dstaddr = NULL;
		}
		if( p->ifa_data )
		{
			free( p->ifa_data );
			p->ifa_data = NULL;
		}
		free( p );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool SortNetworkInterface(const NetworkInterface& n1, const NetworkInterface& n2)
{
  return n1.index() < n2.index();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkInterface::GetAll(vector<NetworkInterface>& interfaces)
{
  IP_ADAPTER_ADDRESSES* iaaList = 0;
	DWORD flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

  struct ifaddrs* head = 0;
  struct ifaddrs** next = &head;

  // Get the adaptors.
	for (int i=0; i<100; i++)
	{
		ULONG iaaListSize = 0;
		DWORD err = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &iaaListSize);
		iaaList = (IP_ADAPTER_ADDRESSES* )malloc(iaaListSize);
		
		err = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, iaaList, &iaaListSize);
		if (err == ERROR_SUCCESS) 
      break;
		
		free(iaaList);
		iaaList = 0;
	}

  if (iaaList == 0)
    return;

  // Iterate through them.
  for (IP_ADAPTER_ADDRESSES* iaa = iaaList; iaa; iaa = iaa->Next)
	{
		DWORD ipv6IfIndex = 0;
		IP_ADAPTER_PREFIX* firstPrefix = 0;

		// For IPv4 interfaces, there seems to be a bug in iphlpapi.dll that causes the 
		// following code to crash when iterating through the prefix list.  This seems
		// to occur when iaa->Ipv6IfIndex != 0 when IPv6 is not installed on the host.
		// This shouldn't happen according to Microsoft docs which states:
		//
		//     "Ipv6IfIndex contains 0 if IPv6 is not available on the interface."
		//
		// So the data structure seems to be corrupted when we return from
		// GetAdaptersAddresses(). The bug seems to occur when iaa->Length <
		// sizeof(IP_ADAPTER_ADDRESSES), so when that happens, we'll manually
		// modify iaa to have the correct values.
    //
    if (iaa->Length >= sizeof(IP_ADAPTER_ADDRESSES))
		{
			ipv6IfIndex = iaa->Ipv6IfIndex;
			firstPrefix = iaa->FirstPrefix;
		}
		else
		{
			ipv6IfIndex	= 0;
			firstPrefix = NULL;
		}

		// Skip pseudo and tunnel interfaces.
		if (((ipv6IfIndex == 1) && (iaa->IfType != IF_TYPE_SOFTWARE_LOOPBACK)) || (iaa->IfType == IF_TYPE_TUNNEL))
			continue;
		
		// Add each address as a separate interface to emulate the way getifaddrs works.
    IP_ADAPTER_UNICAST_ADDRESS* addr = 0;
    int addrIndex = 0;
		for (addrIndex = 0, addr = iaa->FirstUnicastAddress; addr; ++addrIndex, addr = addr->Next)
		{			
      // Only use IPv4/6
			int family = addr->Address.lpSockaddr->sa_family;
			if ((family != AF_INET) && (family != AF_INET6)) 
        continue;
			
			// <rdar://problem/6220642> iTunes 8: Bonjour doesn't work after upgrading iTunes 8
			// Seems as if the problem here is a buggy implementation of some network interface
			// driver. It is reporting that is has a link-local address when it is actually
			// disconnected. This was causing a problem in AddressToIndexAndMask.
			// The solution is to call AddressToIndexAndMask first, and if unable to lookup
			// the address, to ignore that address.
      //
			uint32_t ipv4Index = 0;
      struct sockaddr_in ipv4Netmask;
			memset(&ipv4Netmask, 0, sizeof(ipv4Netmask));
			
			if (family == AF_INET)
			{
				if (AddressToIndexAndMask(addr->Address.lpSockaddr, &ipv4Index, (struct sockaddr* ) &ipv4Netmask) == false)
          continue;
			}

			struct ifaddrs* ifa = (struct ifaddrs *)calloc(1, sizeof(struct ifaddrs));
			*next = ifa;
			next  = &ifa->ifa_next;
			
			// Get the name.
			size_t size = strlen( iaa->AdapterName ) + 1;
			ifa->ifa_name = (char* )malloc(size);
			memcpy(ifa->ifa_name, iaa->AdapterName, size);
			
			// Get interface flags.
			ifa->ifa_flags = 0;
			if (iaa->OperStatus == IfOperStatusUp)        ifa->ifa_flags |= IFF_UP;
			if (iaa->IfType == IF_TYPE_SOFTWARE_LOOPBACK) ifa->ifa_flags |= IFF_LOOPBACK;
			if (!(iaa->Flags & IP_ADAPTER_NO_MULTICAST))  ifa->ifa_flags |= IFF_MULTICAST;
			
			// <rdar://problem/4045657> Interface index being returned is 512
			//
			// Windows does not have a uniform scheme for IPv4 and IPv6 interface indexes.
			// This code used to shift the IPv4 index up to ensure uniqueness between
			// it and IPv6 indexes.  Although this worked, it was somewhat confusing to developers, who
			// then see interface indexes passed back that don't correspond to anything
			// that is seen in Win32 APIs or command line tools like "route".  As a relatively
			// small percentage of developers are actively using IPv6, it seems to 
			// make sense to make our use of IPv4 as confusion free as possible.
			// So now, IPv6 interface indexes will be shifted up by a
			// constant value which will serve to uniquely identify them, and we will
			// leave IPv4 interface indexes unmodified.
			//
			switch (family)
			{
				case AF_INET:  ifa->ifa_extra.index = iaa->IfIndex; break;
				case AF_INET6: ifa->ifa_extra.index = ipv6IfIndex + kIPv6IfIndexBase;	 break;
				default: break;
			}

			// Get address.
			switch (family)
			{
				case AF_INET:
				case AF_INET6:
					ifa->ifa_addr = (struct sockaddr *) calloc( 1, (size_t) addr->Address.iSockaddrLength);
					memcpy(ifa->ifa_addr, addr->Address.lpSockaddr, (size_t) addr->Address.iSockaddrLength);
					break;
				
				default:
					break;
      }

      // Get netmask
      switch (family)
      {
        case AF_INET:
        {
          struct sockaddr_in* sa4 = (struct sockaddr_in*)calloc(1, sizeof(sockaddr_in));
          sa4->sin_family = AF_INET;
          sa4->sin_addr.s_addr = ipv4Netmask.sin_addr.s_addr;
          ifa->ifa_netmask = (struct sockaddr*)sa4;
          break;
        }
        case AF_INET6:
        {
          struct sockaddr_in6 *sa6 = (struct sockaddr_in6*) calloc(1, sizeof(*sa6));
          sa6->sin6_family = AF_INET6;
          memset(sa6->sin6_addr.s6_addr, 0xFF, sizeof(sa6->sin6_addr.s6_addr));
          ifa->ifa_netmask = (struct sockaddr*) sa6;

          for (IP_ADAPTER_PREFIX* prefix = firstPrefix; prefix; prefix = prefix->Next)
          {
            IN6_ADDR mask;
            IN6_ADDR maskedAddr;

            // According to MSDN:
            // "On Windows Vista and later, the linked IP_ADAPTER_PREFIX structures pointed to by the FirstPrefix member
            // include three IP adapter prefixes for each IP address assigned to the adapter. These include the host IP address prefix,
            // the subnet IP address prefix, and the subnet broadcast IP address prefix.
            // In addition, for each adapter there is a multicast address prefix and a broadcast address prefix.
            // On Windows XP with SP1 and later prior to Windows Vista, the linked IP_ADAPTER_PREFIX structures pointed to by the FirstPrefix member
            // include only a single IP adapter prefix for each IP address assigned to the adapter."
            //
            // We're only interested in the subnet IP address prefix.  We'll determine if the prefix is the
            // subnet prefix by masking our address with a mask (computed from the prefix length) and see if that is the same
            // as the prefix address.

            if (prefix->PrefixLength == 0 || prefix->PrefixLength > 128 ||
                addr->Address.iSockaddrLength != prefix->Address.iSockaddrLength ||
                memcmp(addr->Address.lpSockaddr, prefix->Address.lpSockaddr, addr->Address.iSockaddrLength) == 0)
            {
              continue;
            }

            // Compute the mask
            memset(mask.s6_addr, 0, sizeof(mask.s6_addr));

            int maskIndex;
            for (ULONG len = prefix->PrefixLength, maskIndex = 0; len > 0; len -= 8)
            {
              uint8_t maskByte = (len >= 8) ? 0xFF : (uint8_t)((0xFFU << (8 - len)) & 0xFFU);
              mask.s6_addr[maskIndex++] = maskByte;
            }

            // Apply the mask
            for (int i = 0; i < 16; i++)
            {
              maskedAddr.s6_addr[i] = ((struct sockaddr_in6*)addr->Address.lpSockaddr)->sin6_addr.s6_addr[i] & mask.s6_addr[i];
            }

            // Compare
            if (memcmp(((struct sockaddr_in6*)prefix->Address.lpSockaddr)->sin6_addr.s6_addr, maskedAddr.s6_addr, sizeof(maskedAddr.s6_addr)) == 0)
            {
              memcpy(sa6->sin6_addr.s6_addr, mask.s6_addr, sizeof(mask.s6_addr));
              break;
            }
          }
          break;
        }
      }

      // Only add IPv4 interfaces for now.
      if (family == AF_INET &&
          (ifa->ifa_flags & IFF_UP) &&
          (ifa->ifa_flags & IFF_MULTICAST))
      {
        struct sockaddr_in* ifa_addr = (struct sockaddr_in *)ifa->ifa_addr;
        string address = inet_ntoa(ifa_addr->sin_addr);

        struct sockaddr_in *ifa_msk = (struct sockaddr_in *)ifa->ifa_netmask;
        string netmask = inet_ntoa(ifa_msk->sin_addr);
      
        interfaces.push_back(NetworkInterface(ifa->ifa_extra.index, ifa->ifa_name, address, ifa->ifa_flags & IFF_LOOPBACK, netmask));
      }
		}
	}
	
  // Sort by interface index.
  std::sort(interfaces.begin(), interfaces.end(), SortNetworkInterface);

  // Free memory.	
	if (head)
		freeifaddrs(head);

  if (iaaList)
		free(iaaList);
}

#endif
