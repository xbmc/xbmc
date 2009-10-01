#include <stdio.h>
#include "socketutil.h"

#ifdef _XBOX
char* webs_inet_ntoa (struct in_addr in)
{
	static char _inetaddress[32];
	sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
	return _inetaddress;
}

struct hostent* webs_gethostbyname(const char* _name)
{
	HostEnt* server = new HostEnt;

	strcpy(server->name, "xbox");
	if(!strcmp(server->name, _name))
	{
		XNADDR xna;
		DWORD dwState;
		do
		{
			dwState = XNetGetTitleXnAddr(&xna);
		} while (dwState==XNET_GET_XNADDR_PENDING);

		server->addr_list[0] = server->addr;
		memcpy(server->addr, &(xna.ina.s_addr), 4);
		server->server.h_name = server->name;
		server->server.h_aliases = 0;
		server->server.h_addrtype = AF_INET;
		server->server.h_length = 4;
		server->server.h_addr_list = new char*[4];
		server->server.h_addr_list[0] = server->addr_list[0];
		server->server.h_addr_list[1] = 0;
		return (struct hostent*)server;
	}

	WSAEVENT hEvent = WSACreateEvent();
	XNDNS* pDns = NULL;
	INT err = XNetDnsLookup(_name, hEvent, &pDns);
	WaitForSingleObject(hEvent, INFINITE);
	if( pDns && pDns->iStatus == 0 )
	{
		strcpy(server->name, _name);
		server->addr_list[0] = server->addr;
		memcpy(server->addr, &(pDns->aina[0].s_addr), 4);
		server->server.h_name = server->name;
		server->server.h_aliases = 0;
		server->server.h_addrtype = AF_INET;
		server->server.h_length = 4;
		server->server.h_addr_list = new char*[4];

		server->server.h_addr_list[0] = server->addr_list[0];
		server->server.h_addr_list[1] = 0;
		XNetDnsRelease(pDns);
		return (struct hostent*)server;
	}
	if( pDns )
		XNetDnsRelease(pDns);
	delete server;
	return 0;
}
#endif