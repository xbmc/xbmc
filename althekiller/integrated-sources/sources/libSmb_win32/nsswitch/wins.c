/* 
   Unix SMB/CIFS implementation.
   a WINS nsswitch module 
   Copyright (C) Andrew Tridgell 1999
   
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

#ifdef _XBOX
#define close(s) closesocket(s)
#endif


#ifdef HAVE_NS_API_H
#undef VOLATILE

#include <ns_daemon.h>
#endif

#ifndef INADDRSZ
#define INADDRSZ 4
#endif

static int initialised;

extern BOOL AllowDebugChange;

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


static void nss_wins_init(void)
{
	initialised = 1;
	DEBUGLEVEL = 0;
	AllowDebugChange = False;

	TimeInit();
	setup_logging("nss_wins",False);
	load_case_tables();
	lp_load(dyn_CONFIGFILE,True,False,False,True);
	load_interfaces();
}

static struct in_addr *lookup_byname_backend(const char *name, int *count)
{
	int fd = -1;
	struct ip_service *address = NULL;
	struct in_addr *ret = NULL;
	int j, flags = 0;

	if (!initialised) {
		nss_wins_init();
	}

	*count = 0;

	/* always try with wins first */
	if (resolve_wins(name,0x00,&address,count)) {
		if ( (ret = SMB_MALLOC_P(struct in_addr)) == NULL ) {
			free( address );
			return NULL;
		}
		*ret = address[0].ip;
		free( address );
		return ret;
	}

	fd = wins_lookup_open_socket_in();
	if (fd == -1) {
		return NULL;
	}

	/* uggh, we have to broadcast to each interface in turn */
	for (j=iface_count() - 1;j >= 0;j--) {
		struct in_addr *bcast = iface_n_bcast(j);
		ret = name_query(fd,name,0x00,True,True,*bcast,count, &flags, NULL);
		if (ret) break;
	}

	close(fd);
	return ret;
}

#ifdef HAVE_NS_API_H

static NODE_STATUS_STRUCT *lookup_byaddr_backend(char *addr, int *count)
{
	int fd;
	struct in_addr  ip;
	struct nmb_name nname;
	NODE_STATUS_STRUCT *status;

	if (!initialised) {
		nss_wins_init();
	}

	fd = wins_lookup_open_socket_in();
	if (fd == -1)
		return NULL;

	make_nmb_name(&nname, "*", 0);
	ip = *interpret_addr2(addr);
	status = node_status_query(fd,&nname,ip, count, NULL);

	close(fd);
	return status;
}

/* IRIX version */

int init(void)
{
	nsd_logprintf(NSD_LOG_MIN, "entering init (wins)\n");
	nss_wins_init();
	return NSD_OK;
}

int lookup(nsd_file_t *rq)
{
	char *map;
	char *key;
	char *addr;
	struct in_addr *ip_list;
	NODE_STATUS_STRUCT *status;
	int i, count, len, size;
	char response[1024];
	BOOL found = False;

	nsd_logprintf(NSD_LOG_MIN, "entering lookup (wins)\n");
	if (! rq) 
		return NSD_ERROR;

	map = nsd_attr_fetch_string(rq->f_attrs, "table", (char*)0);
	if (! map) {
		rq->f_status = NS_FATAL;
		return NSD_ERROR;
	}

	key = nsd_attr_fetch_string(rq->f_attrs, "key", (char*)0);
	if (! key || ! *key) {
		rq->f_status = NS_FATAL;
		return NSD_ERROR;
	}

	response[0] = '\0';
	len = sizeof(response) - 2;

	/* 
	 * response needs to be a string of the following format
	 * ip_address[ ip_address]*\tname[ alias]*
	 */
	if (StrCaseCmp(map,"hosts.byaddr") == 0) {
		if ( status = lookup_byaddr_backend(key, &count)) {
		    size = strlen(key) + 1;
		    if (size > len) {
			free(status);
			return NSD_ERROR;
		    }
		    len -= size;
		    strncat(response,key,size);
		    strncat(response,"\t",1);
		    for (i = 0; i < count; i++) {
			/* ignore group names */
			if (status[i].flags & 0x80) continue;
			if (status[i].type == 0x20) {
				size = sizeof(status[i].name) + 1;
				if (size > len) {
				    free(status);
				    return NSD_ERROR;
				}
				len -= size;
				strncat(response, status[i].name, size);
				strncat(response, " ", 1);
				found = True;
			}
		    }
		    response[strlen(response)-1] = '\n';
		    free(status);
		}
	} else if (StrCaseCmp(map,"hosts.byname") == 0) {
	    if (ip_list = lookup_byname_backend(key, &count)) {
		for (i = count; i ; i--) {
		    addr = inet_ntoa(ip_list[i-1]);
		    size = strlen(addr) + 1;
		    if (size > len) {
			free(ip_list);
			return NSD_ERROR;
		    }
		    len -= size;
		    if (i != 0)
			response[strlen(response)-1] = ' ';
		    strncat(response,addr,size);
		    strncat(response,"\t",1);
		}
		size = strlen(key) + 1;
		if (size > len) {
		    free(ip_list);
		    return NSD_ERROR;
		}   
		strncat(response,key,size);
		strncat(response,"\n",1);
		found = True;
		free(ip_list);
	    }
	}

	if (found) {
	    nsd_logprintf(NSD_LOG_LOW, "lookup (wins %s) %s\n",map,response);
	    nsd_set_result(rq,NS_SUCCESS,response,strlen(response),VOLATILE);
	    return NSD_OK;
	}
	nsd_logprintf(NSD_LOG_LOW, "lookup (wins) not found\n");
	rq->f_status = NS_NOTFOUND;
	return NSD_NEXT;
}

#else

/* Allocate some space from the nss static buffer.  The buffer and buflen
   are the pointers passed in by the C library to the _nss_*_*
   functions. */

static char *get_static(char **buffer, size_t *buflen, int len)
{
	char *result;

	/* Error check.  We return false if things aren't set up right, or
	   there isn't enough buffer space left. */
	
	if ((buffer == NULL) || (buflen == NULL) || (*buflen < len)) {
		return NULL;
	}

	/* Return an index into the static buffer */

	result = *buffer;
	*buffer += len;
	*buflen -= len;

	return result;
}

/****************************************************************************
gethostbyname() - we ignore any domain portion of the name and only
handle names that are at most 15 characters long
  **************************************************************************/
NSS_STATUS
_nss_wins_gethostbyname_r(const char *hostname, struct hostent *he,
			  char *buffer, size_t buflen, int *h_errnop)
{
	struct in_addr *ip_list;
	int i, count;
	fstring name;
	size_t namelen;
		
	memset(he, '\0', sizeof(*he));
	fstrcpy(name, hostname);

	/* Do lookup */

	ip_list = lookup_byname_backend(name, &count);

	if (!ip_list)
		return NSS_STATUS_NOTFOUND;

	/* Copy h_name */

	namelen = strlen(name) + 1;

	if ((he->h_name = get_static(&buffer, &buflen, namelen)) == NULL)
		return NSS_STATUS_TRYAGAIN;

	memcpy(he->h_name, name, namelen);

	/* Copy h_addr_list, align to pointer boundary first */

	if ((i = (unsigned long)(buffer) % sizeof(char*)) != 0)
		i = sizeof(char*) - i;

	if (get_static(&buffer, &buflen, i) == NULL)
		return NSS_STATUS_TRYAGAIN;

	if ((he->h_addr_list = (char **)get_static(
		     &buffer, &buflen, (count + 1) * sizeof(char *))) == NULL)
		return NSS_STATUS_TRYAGAIN;

	for (i = 0; i < count; i++) {
		if ((he->h_addr_list[i] = get_static(&buffer, &buflen,
						     INADDRSZ)) == NULL)
			return NSS_STATUS_TRYAGAIN;
		memcpy(he->h_addr_list[i], &ip_list[i], INADDRSZ);
	}

	he->h_addr_list[count] = NULL;

	if (ip_list)
		free(ip_list);

	/* Set h_addr_type and h_length */

	he->h_addrtype = AF_INET;
	he->h_length = INADDRSZ;

	/* Set h_aliases */

	if ((i = (unsigned long)(buffer) % sizeof(char*)) != 0)
		i = sizeof(char*) - i;

	if (get_static(&buffer, &buflen, i) == NULL)
		return NSS_STATUS_TRYAGAIN;

	if ((he->h_aliases = (char **)get_static(
		     &buffer, &buflen, sizeof(char *))) == NULL)
		return NSS_STATUS_TRYAGAIN;

	he->h_aliases[0] = NULL;

	return NSS_STATUS_SUCCESS;
}


NSS_STATUS
_nss_wins_gethostbyname2_r(const char *name, int af, struct hostent *he,
			   char *buffer, size_t buflen, int *h_errnop)
{
	if(af!=AF_INET) {
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	return _nss_wins_gethostbyname_r(
		name, he, buffer, buflen, h_errnop);
}
#endif
