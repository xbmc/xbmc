#include "stdafx.h"
#include "dnsnamecache.h"

CDNSNameCache g_DNSCache;

CDNSNameCache::CDNSNameCache(void)
{
}

CDNSNameCache::~CDNSNameCache(void)
{
}

bool CDNSNameCache::Lookup(const CStdString& strHostName, CStdString& strIpAdres)
{
	// first see if this is already an ipadres
	unsigned long ulHostIp = inet_addr( strHostName.c_str() );
	
	if( ulHostIp != 0xFFFFFFFF )
	{
		// yes it is, just return it
		strIpAdres.Format("%d.%d.%d.%d",(ulHostIp & 0xFF),(ulHostIp & 0xFF00) >> 8,(ulHostIp & 0xFF0000) >> 16,(ulHostIp & 0xFF000000) >> 24 );
		return true;
	}

	// nop this is an hostname
	// check if we already cached the hostname <->ip adres
	for (int i=0; i < (int)g_DNSCache.m_vecDNSNames.size(); ++i)
	{
			CDNSName& DNSname = g_DNSCache.m_vecDNSNames[i];
			if ( DNSname.m_strHostName==strHostName ) 
			{
				strIpAdres=DNSname.m_strIpAdres;
				return true;
			}
	}

	// hostname not found in cache.
	// do a DNS lookup
	WSAEVENT hEvent = WSACreateEvent();
	XNDNS* pDns		  = NULL;
	INT err = XNetDnsLookup(strHostName.c_str(), hEvent, &pDns);
	WaitForSingleObject( (HANDLE)hEvent, INFINITE);
	if( pDns && pDns->iStatus == 0 )
	{
		//DNS lookup succeeded
		
		unsigned long ulHostIp;
		memcpy(&ulHostIp, &(pDns->aina[0].s_addr), 4);

		strIpAdres.Format("%d.%d.%d.%d",(ulHostIp & 0xFF),(ulHostIp & 0xFF00) >> 8,(ulHostIp & 0xFF0000) >> 16,(ulHostIp & 0xFF000000) >> 24 );

		CDNSName dnsName;
		dnsName.m_strHostName=strHostName;
		dnsName.m_strIpAdres =strIpAdres;
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
	return false;
}