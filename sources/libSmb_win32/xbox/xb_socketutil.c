#ifdef _WIN32
#include <windows.h>
#else
#include <xtl.h>
#include <winsockx.h>
#include "xb_socketutil.h"
#endif
#include <stdio.h>
#include "config.h"
#include "xb_util.h"
#ifndef _WIN32
char* smb_inet_ntoa (struct in_addr in)
{
	static char _inetaddress[32];
	sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
	return _inetaddress;
}

struct hostent* smb_gethostbyname(const char* _name)
{
	//OutputDebugString("TODO: gethostbyname\n");
  WSAEVENT hEvent = NULL;
  XNDNS* pDns = NULL;
  INT err;

	HostEnt* server = (HostEnt*)malloc(sizeof(HostEnt));

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
		server->server.h_addr_list = (char**)malloc(sizeof(char*)*4);
		server->server.h_addr_list[0] = server->addr_list[0];
		server->server.h_addr_list[1] = 0;
		return (struct hostent*)server;
	}

	hEvent = WSACreateEvent();
	err = XNetDnsLookup(_name, hEvent, &pDns);
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
		server->server.h_addr_list = (char**)malloc(sizeof(char*)*4);

		server->server.h_addr_list[0] = server->addr_list[0];
		server->server.h_addr_list[1] = 0;
		XNetDnsRelease(pDns);
    WSACloseEvent(hEvent);
		return (struct hostent*)server;
	}
	if( pDns )
		XNetDnsRelease(pDns);

  if( hEvent )
    WSACloseEvent(hEvent);

  free(server);
	return NULL;
}

int smb_gethostname(char* buffer, int buffersize)
{
	//OutputDebugString("TODO: gethostname\n");
	if( (unsigned int)buffersize < strlen("xbox") + 1 )
		return -1;
	strcpy(buffer, "xbox");
	return 0;
}

struct hostent* FAR smb_gethostbyaddr(const char* addr, int len, int type) {
	//OutputDebugString("TODO: gethostbyaddr\n");

	HostEnt* server = (HostEnt*)malloc(sizeof(HostEnt));

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
		server->server.h_addr_list = (char**)malloc(sizeof(char*)*4);
		server->server.h_addr_list[0] = server->addr_list[0];
		server->server.h_addr_list[1] = 0;
		return (struct hostent*)server;
	}
	else
	{
		//get client hostent
		struct in_addr client = {{addr[0],addr[1],addr[2],addr[3]}};

		strcpy(server->name, smb_inet_ntoa(client));
		server->addr_list[0] = server->addr;
		memcpy(server->addr, addr, 4);
		server->server.h_name = server->name;
		server->server.h_aliases = 0;
		server->server.h_addrtype = AF_INET;
		server->server.h_length = 4;
		server->server.h_addr_list = (char**)malloc(sizeof(char*)*4);
		server->server.h_addr_list[0] = server->addr_list[0];
		server->server.h_addr_list[1] = 0;
		return (struct hostent*)server;
	}
	return 0;
}

#endif
int socketerrno(int isconnect)
{
  int error = WSAGetLastError();
  if( error == WSAEWOULDBLOCK && isconnect )
    return WSAEINPROGRESS; 
  else
    return error;
}
