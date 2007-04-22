/*
 * LinksBoks
 * Copyright (c) 2003-2005 ysbox
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "cfg.h"

#ifdef __XBOX__

#include "xbox-dns.h"
#include <xtl.h>
#undef errno

typedef struct {
struct hostent server;
char name[128];
char addr[16];
char* addr_list[4];
} HostEnt;

struct hostent* _cdecl gethostbyname(const char*);
struct hostent* _cdecl gethostbyaddr(const char* addr, int len, int type);

#define XBOXNAME "myxbox"

int _cdecl gethostname(char* buffer, int buffersize)
{
	if( (unsigned int)buffersize < strlen(XBOXNAME) + 1 )
		return 0;
	strcpy(buffer, XBOXNAME);
	return 1;
}


struct hostent* _cdecl gethostbyname(const char* _name)
{
	HostEnt *server = (HostEnt *)malloc( sizeof( HostEnt ) );
	struct hostent *host = (struct hostent *)malloc( sizeof( struct hostent ) );
	unsigned long addr = inet_addr( _name );

	WSAEVENT hEvent = WSACreateEvent();
	XNDNS* pDns = NULL;
	if( addr != INADDR_NONE )
	{
		host->h_addr_list[0] = (char *)malloc( 4 );
		memcpy( host->h_addr_list[0], &addr, 4 );
	}
	else
	{
		INT err = XNetDnsLookup(_name, hEvent, &pDns);
		WaitForSingleObject(hEvent, INFINITE);
		if( !pDns || pDns->iStatus )
		{
			if( pDns )
				XNetDnsRelease(pDns);
			free( host );
			return NULL;
		}

		host->h_addr_list[0] = (char *)malloc( 4 );
		memcpy( host->h_addr_list[0], &(pDns->aina[0].s_addr), 4 );

		XNetDnsRelease(pDns);
	}

	host->h_name = (char *)malloc( strlen( _name ) );
	strcpy( host->h_name, _name );
	host->h_aliases = 0;
	host->h_addrtype = AF_INET;
	host->h_length = 4;
	host->h_addr_list[1] = NULL;

	return host;
}

#endif
