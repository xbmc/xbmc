#include "..\..\..\stdafx.h"
#include <stdio.h>
#include "socketutil.h"
#ifdef _XBOX
#include "util.h"
#endif

#pragma code_seg( "PY_UTIL" )
#pragma data_seg( "PY_UTIL_RW" )
#pragma bss_seg( "PY_UTIL_RW" )
#pragma const_seg( "PY_UTIL_RD" )
#pragma comment(linker, "/merge:PY_UTIL_RW=PY_UTIL")
#pragma comment(linker, "/merge:PY_UTIL_RD=PY_UTIL")
#pragma comment(linker, "/section:PY_UTIL,RWE")

#ifdef _XBOX
char* inet_ntoa (struct in_addr in)
{
	static char _inetaddress[32];
	sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
	return _inetaddress;
}

struct hostent* gethostbyname(const char* _name)
{
	//OutputDebugString("TODO: gethostbyname\n");

	HostEnt* server = new HostEnt;

	gethostname(server->name, 128);
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

int gethostname(char* buffer, int buffersize)
{
	//OutputDebugString("TODO: gethostname\n");
	if( (unsigned int)buffersize < strlen("xbox") + 1 )
		return -1;
	strcpy(buffer, "xbox");
	return 0;
}

struct hostent* FAR gethostbyaddr(const char* addr, int len, int type) {
	OutputDebugString("TODO: gethostbyaddr\n");

	HostEnt* server = new HostEnt;

	XNADDR xna;
	DWORD dwState;
	do
	{
		dwState = XNetGetTitleXnAddr(&xna);
	} while (dwState==XNET_GET_XNADDR_PENDING);

	if (!memcmp(addr, &(xna.ina.s_addr), 4))
	{
		//get title hostent
		gethostname(server->name, 128);
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
	else
	{
		//get client hostent
		//strcpy(server->name, _name);
		server->addr_list[0] = server->addr;
		memcpy(server->addr, addr, 4);
		//server->server.h_name = server->name;
		server->server.h_aliases = 0;
		server->server.h_addrtype = AF_INET;
		server->server.h_length = 4;
		server->server.h_addr_list = new char*[4];
		server->server.h_addr_list[0] = server->addr_list[0];
		server->server.h_addr_list[1] = 0;
		return (struct hostent*)server;
	}
	return 0;
}

struct servent* FAR getservbyname(const char* name, const char* proto) {
	OutputDebugString("TODO: getservbyname\n");
	return NULL;
}

struct servent* FAR getservbyport(int port, const char* proto) {
	OutputDebugString("TODO: getservbyport\n");
	return NULL;
}

struct protoent* FAR getprotobyname(const char* name) {
	OutputDebugString("TODO: getprotobyname\n");
	return NULL;
}
#endif
