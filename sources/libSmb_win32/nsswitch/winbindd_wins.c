/* 
   Unix SMB/CIFS implementation.

   Winbind daemon - WINS related functions

   Copyright (C) Andrew Tridgell 1999
   Copyright (C) Herb Lewis 2002
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"
#include "winbindd.h"

#ifdef _XBOX
#define close(s) closesocket(s)
#endif

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

/* Use our own create socket code so we don't recurse.... */

static int wins_lookup_open_socket_in(void)
{
	struct sockaddr_in sock;
	int val=1;
	int res;

	memset((char *)&sock,'\0',sizeof(sock));

#ifdef HAVE_SOCK_SIN_LEN
	sock.sin_len = sizeof(sock);
#endif
	sock.sin_port = 0;
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = interpret_addr("0.0.0.0");
	res = socket(AF_INET, SOCK_DGRAM, 0);
	if (res == -1)
		return -1;

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val));
#ifdef SO_REUSEPORT
	setsockopt(res,SOL_SOCKET,SO_REUSEPORT,(char *)&val,sizeof(val));
#endif /* SO_REUSEPORT */

	/* now we've got a socket - we need to bind it */

	if (bind(res, (struct sockaddr * ) &sock,sizeof(sock)) < 0) {
		close(res);
		return(-1);
	}

	set_socket_options(res,"SO_BROADCAST");

	return res;
}


static NODE_STATUS_STRUCT *lookup_byaddr_backend(char *addr, int *count)
{
	int fd;
	struct in_addr  ip;
	struct nmb_name nname;
	NODE_STATUS_STRUCT *status;

	fd = wins_lookup_open_socket_in();
	if (fd == -1)
		return NULL;

	make_nmb_name(&nname, "*", 0);
	ip = *interpret_addr2(addr);
	status = node_status_query(fd,&nname,ip, count, NULL);

	close(fd);
	return status;
}

static struct in_addr *lookup_byname_backend(const char *name, int *count)
{
	int fd;
	struct ip_service *ret = NULL;
	struct in_addr *return_ip = NULL;
	int j, i, flags = 0;

	*count = 0;

	/* always try with wins first */
	if (resolve_wins(name,0x20,&ret,count)) {
		if ( count == 0 )
			return NULL;
		if ( (return_ip = SMB_MALLOC_ARRAY(struct in_addr, *count)) == NULL ) {
			free( ret );
			return NULL;
		}

		/* copy the IP addresses */
		for ( i=0; i<(*count); i++ ) 
			return_ip[i] = ret[i].ip;
		
		free( ret );
		return return_ip;
	}

	fd = wins_lookup_open_socket_in();
	if (fd == -1) {
		return NULL;
	}

	/* uggh, we have to broadcast to each interface in turn */
	for (j=iface_count() - 1;
	     j >= 0;
	     j--) {
		struct in_addr *bcast = iface_n_bcast(j);
		return_ip = name_query(fd,name,0x20,True,True,*bcast,count, &flags, NULL);
		if (return_ip) {
			break;
		}
	}

	close(fd);
	return return_ip;
}

/* Get hostname from IP  */

void winbindd_wins_byip(struct winbindd_cli_state *state)
{
	fstring response;
	int i, count, maxlen, size;
	NODE_STATUS_STRUCT *status;

	/* Ensure null termination */
	state->request.data.winsreq[sizeof(state->request.data.winsreq)-1]='\0';

	DEBUG(3, ("[%5lu]: wins_byip %s\n", (unsigned long)state->pid,
		state->request.data.winsreq));

	*response = '\0';
	maxlen = sizeof(response) - 1;

	if ((status = lookup_byaddr_backend(state->request.data.winsreq, &count))){
	    size = strlen(state->request.data.winsreq);
	    if (size > maxlen) {
		SAFE_FREE(status);
		request_error(state);
		return;
	    }
	    fstrcat(response,state->request.data.winsreq);
	    fstrcat(response,"\t");
	    for (i = 0; i < count; i++) {
		/* ignore group names */
		if (status[i].flags & 0x80) continue;
		if (status[i].type == 0x20) {
			size = sizeof(status[i].name) + strlen(response);
			if (size > maxlen) {
			    SAFE_FREE(status);
			    request_error(state);
			    return;
			}
			fstrcat(response, status[i].name);
			fstrcat(response, " ");
		}
	    }
	    /* make last character a newline */
	    response[strlen(response)-1] = '\n';
	    SAFE_FREE(status);
	}
	fstrcpy(state->response.data.winsresp,response);
	request_ok(state);
}

/* Get IP from hostname */

void winbindd_wins_byname(struct winbindd_cli_state *state)
{
	struct in_addr *ip_list;
	int i, count, maxlen, size;
	fstring response;
	char * addr;

	/* Ensure null termination */
	state->request.data.winsreq[sizeof(state->request.data.winsreq)-1]='\0';

	DEBUG(3, ("[%5lu]: wins_byname %s\n", (unsigned long)state->pid,
		state->request.data.winsreq));

	*response = '\0';
	maxlen = sizeof(response) - 1;

	if ((ip_list = lookup_byname_backend(state->request.data.winsreq,&count))){
		for (i = count; i ; i--) {
		    addr = inet_ntoa(ip_list[i-1]);
		    size = strlen(addr);
		    if (size > maxlen) {
			SAFE_FREE(ip_list);
			request_error(state);
			return;
		    }
		    if (i != 0) {
			/* Clear out the newline character */
		        /* But only if there is something in there, 
			   otherwise we clobber something in the stack */
			if (strlen(response))
				response[strlen(response)-1] = ' '; 
		    }
		    fstrcat(response,addr);
		    fstrcat(response,"\t");
		}
		size = strlen(state->request.data.winsreq) + strlen(response);
		if (size > maxlen) {
		    SAFE_FREE(ip_list);
		    request_error(state);
		    return;
		}   
		fstrcat(response,state->request.data.winsreq);
		fstrcat(response,"\n");
		SAFE_FREE(ip_list);
	} else {
		request_error(state);
		return;
	}

	fstrcpy(state->response.data.winsresp,response);

	request_ok(state);
}
