#include "stdafx.h"
#include "dnsnamecache.h"


CDNSNameCache g_DNSCache;

CDNSNameCache::CDNSNameCache(void)
{}

CDNSNameCache::~CDNSNameCache(void)
{}

bool CDNSNameCache::Lookup(const CStdString& strHostName, CStdString& strIpAdres)
{
  // first see if this is already an ipadres
  unsigned long ulHostIp = inet_addr( strHostName.c_str() );

  if ( ulHostIp != 0xFFFFFFFF )
  {
    // yes it is, just return it
    strIpAdres.Format("%d.%d.%d.%d", (ulHostIp & 0xFF), (ulHostIp & 0xFF00) >> 8, (ulHostIp & 0xFF0000) >> 16, (ulHostIp & 0xFF000000) >> 24 );
    return true;
  }

  // nop this is an hostname
  // check if we already cached the hostname <->ip adres
  for (int i = 0; i < (int)g_DNSCache.m_vecDNSNames.size(); ++i)
  {
    CDNSName& DNSname = g_DNSCache.m_vecDNSNames[i];
    if ( DNSname.m_strHostName == strHostName )
    {
      strIpAdres = DNSname.m_strIpAdres;
      return true;
    }
  }

  // hostname not found in cache.
  // do a DNS lookup
#ifndef _XBOX
  {
	  WSADATA wsaData;		/* Used to open Windows connection */
	  SOCKET sd;				/* Socket descriptor */
	  struct sockaddr_in socket_address;
	  struct hostent *host;
	  int count = 0;

	  /* Open a windows connection */
	  if (WSAStartup(0x0101, &wsaData) != 0)
	  {
		  OutputDebugString("Could not open Windows connection\n");
      return false;
	  }

	  /* Open up a socket */
	  sd = socket(AF_INET, SOCK_STREAM, 0);

	  /* Make sure the socket was opened */
	  if (sd == INVALID_SOCKET)
	  {
		  OutputDebugString("Could not open socket.\n");
		  WSACleanup();
		  return false;
	  }

	  /* Bind with IP address */
	  memset(&socket_address, '\0', sizeof(struct sockaddr_in));
	  socket_address.sin_family = AF_INET;
	  socket_address.sin_port = htons((short)4000);

	  /* Get host by name */
	  host = gethostbyname(strHostName.c_str());
  	
	  /* Print error message if could not find host */
	  if (host == NULL || host->h_addr_list[0] == NULL)
	  {
		  OutputDebugString("Could not find host\n");
		  closesocket(sd);
		  WSACleanup();
		  return false;
	  }

	  /* Print out host name */
    CLog::Log(LOGDEBUG, "host name = %s\n", host->h_name);
  	
	  /* Loop through and print out any aliases */
	  while(1)
	  {
		  if(host->h_aliases[count] == NULL)
		  {
			  break;
		  }
      CLog::Log(LOGDEBUG, "alias %d: %s\n", count + 1, host->h_aliases[count]);
		  ++count;
	  }
  	
	  /* Print out all IP addresses of name */
	  if (host->h_addr_list[0])
	  {
      strIpAdres.Format("%d.%d.%d.%d", (unsigned char)host->h_addr_list[0][0], (unsigned char)host->h_addr_list[0][1], (unsigned char)host->h_addr_list[0][2], (unsigned char)host->h_addr_list[0][3]);
      CDNSName dnsName;
      dnsName.m_strHostName = strHostName;
      dnsName.m_strIpAdres = strIpAdres;
      g_DNSCache.m_vecDNSNames.push_back(dnsName);
	  }

	  closesocket(sd);
	  WSACleanup();

    return true;
  }
#else
  WSAEVENT hEvent = WSACreateEvent();
  XNDNS* pDns = NULL;
  INT err = XNetDnsLookup(strHostName.c_str(), hEvent, &pDns);
  WaitForSingleObject( (HANDLE)hEvent, INFINITE);
  if ( pDns && pDns->iStatus == 0 )
  {
    //DNS lookup succeeded

    unsigned long ulHostIp;
    memcpy(&ulHostIp, &(pDns->aina[0].s_addr), 4);

    strIpAdres.Format("%d.%d.%d.%d", (ulHostIp & 0xFF), (ulHostIp & 0xFF00) >> 8, (ulHostIp & 0xFF0000) >> 16, (ulHostIp & 0xFF000000) >> 24 );

    CDNSName dnsName;
    dnsName.m_strHostName = strHostName;
    dnsName.m_strIpAdres = strIpAdres;
    g_DNSCache.m_vecDNSNames.push_back(dnsName);

    XNetDnsRelease(pDns);
    WSACloseEvent(hEvent);
    return true;
  }
  if (pDns)
  {
    CLog::Log(LOGERROR, "DNS lookup for %s failed: %u", strHostName.c_str(), pDns->iStatus);
    XNetDnsRelease(pDns);
  }
  else
    CLog::Log(LOGERROR, "DNS lookup for %s failed: %u", strHostName.c_str(), err);
  WSACloseEvent(hEvent);
#endif
  return false;
}
