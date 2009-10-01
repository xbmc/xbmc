#ifndef __GNUC__
#pragma code_seg( "RTV_TEXT" )
#pragma data_seg( "RTV_DATA" )
#pragma bss_seg( "RTV_BSS" )
#pragma const_seg( "RTV_RD" )
#endif

/*
 * Copyright (C) 2002 John Todd Larason <jtl@molehill.org>
 *
 * Parts based on ReplayPC 0.3 by Matthew T. Linehan and others
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */

#include "netclient.h"
#include "rtv.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
//#include <netdb.h>

#ifdef _XBOX
#	include <Xtl.h>

	typedef SOCKET socket_fd;

	struct  hostent {
		char    * h_name;			/* official name of host */
		char    * * h_aliases;		/* alias list */
		short   h_addrtype;			/* host address type */
		short   h_length;			/* length of address */
		char    * * h_addr_list;	/* list of addresses */
	#define h_addr  h_addr_list[0]	/* address, for backward compat */
	};

	struct hostent * gethostbyname(const char * name);
#elif defined _WIN32
#  include <winsock2.h>
#  pragma comment(lib, "ws2_32.lib")
   typedef SOCKET socket_fd;
#else
#  include <unistd.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  define closesocket(fd) close(fd)
   typedef int socket_fd;
#  define INVALID_SOCKET (-1)
#include <netdb.h>
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#endif

static int nc_do_dump = 0;
static int nc_initted = 0;

struct nc
{
    u8        rcv_buf[32768];
    size_t    low, high;
    socket_fd fd;
};

static void nc_fini(void)
{
    nc_initted = 0;

#ifdef _WIN32
    WSACleanup();
#endif
}

static void nc_dump(char * tag, unsigned char * buf, size_t sz)
{
#if 0
    unsigned int rows, row, col, i, c;
    if (!nc_do_dump)
        return;
    //fprintf(stderr, "NC DUMP: %s %lu\n", tag, (unsigned long)sz);
    if (buf == NULL)
        return;
    rows = (sz + 15)/16;
    for (row = 0; row < rows; row++) {
        //fprintf(stderr, "| ");
        for (col = 0; col < 16; col++) {
            i = row * 16 + col;
            //if (i < sz)
              //  fprintf(stderr, "%02x", buf[i]);
            //else
              //  fprintf(stderr, "  ");
            //if ((i & 3) == 3)
              //  fprintf(stderr, " ");
        }
        fprintf(stderr, "  |  ");
        for (col = 0; col < 16; col++) {
            i = row * 16 + col;
            if (i < sz) {
                c = buf[i];
                fprintf(stderr, "%c", (c >= ' ' && c <= '~') ? c : '.');
            } else {
                fprintf(stderr, " ");
            }
            if ((i & 3) == 3)
                fprintf(stderr, " ");
        }
        fprintf(stderr, " |\n");
    }
#endif
}

void nc_error(char * where)
{
#ifdef _WIN32
    //fprintf(stderr, "NC error:%s:%d/%d\n", where, errno, WSAGetLastError());
#else
    //fprintf(stderr, "NC error:%s:%d:%s\n", where, errno, strerror(errno));
#endif
}

static int nc_init(void)
{
#ifdef _WIN32
    WSADATA wd;
    
    nc_dump("initting", NULL, 0);
    
    if (WSAStartup(MAKEWORD(2,2), &wd) == -1) {
        nc_error("WSAStartup");
        return -1;
    }
#endif
    nc_initted = 1;

    //if (getenv("NC_DUMP"))
    //    nc_do_dump = 1;

    atexit(nc_fini);
    return 0;
}

static unsigned long parse_addr(const char *address_str)
{
    unsigned long addr;
    struct hostent *hent;

    addr = inet_addr(address_str);
    if (addr != INADDR_NONE)
        return addr;

    nc_dump("looking up address", NULL, 0);

    hent = gethostbyname(address_str);
    if (!hent) {
        //herror(address_str);
        return INADDR_NONE;
    }

    if (hent->h_addrtype == AF_INET && hent->h_addr_list[0] != NULL) {
        memcpy(&addr, hent->h_addr_list[0], sizeof(addr));
        return addr;
    }

    //fprintf(stderr, "No IPv4 address for %s\n", address_str);
    return INADDR_NONE;
}

struct nc * nc_open(char * address_str, short port)
{
    struct nc * nc;
    struct sockaddr_in address;

    if (!nc_initted) {
        if (nc_init()) {
            //fprintf(stderr, "Couldn't initialize netclient library\n");
            return NULL;
        }
    }
    
    nc = malloc(sizeof *nc);
    if (!nc) {
        //fprintf(stderr, "Couldn't allocate memory for struct nc\n");
        return NULL;
    }
    memset(nc, 0, sizeof *nc);
    nc->fd = -1;

    memset(&address, 0, sizeof address);
    address.sin_family      = AF_INET;
    address.sin_port        = htons(port);
    address.sin_addr.s_addr = parse_addr(address_str);

    if (address.sin_addr.s_addr == INADDR_NONE) {
        //fprintf(stderr, "Couldn't determine address for \"%s\"\n", address_str);
        return NULL;
    }

    nc_dump("creating socket", NULL, 0);
    nc->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (nc->fd == INVALID_SOCKET) {
        nc_error("open_nc socket");
        return NULL;
    }
    nc_dump("created", NULL, 0);

    nc_dump("connecting", NULL, 0);
    if (connect(nc->fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        nc_error("open_nc connect");
        return NULL;
    }
    nc_dump("connected", NULL, 0);

    return nc;
}

int nc_close(struct nc * nc)
{
    int r = 0;
    
    if (nc->fd >= 0) {
        nc_dump("closing", NULL, 0);
        r = closesocket(nc->fd);
        nc_dump("closed", NULL, 0);
    }
    free(nc);

    return r;
}

static void fill_rcv_buf(struct nc * nc)
{
    int r;
    
    nc_dump("receiving...", NULL, 0);
    r = recv(nc->fd, nc->rcv_buf, sizeof(nc->rcv_buf), 0);
    nc->low  = 0;
    if (r <= 0) {
        if (r < 0)
            nc_error("fill_rcv_buf recv");
        /* XXX -- error or closed, return whatever we got*/
        nc->high = 0;
    } else {
        nc->high = r;
    }
    nc_dump("received", nc->rcv_buf, nc->high);
}

int nc_read(struct nc * nc, unsigned char * buf, size_t len)
{
    size_t l;
    int r = 0;

    nc_dump("Reading", NULL, len);
    while (len) {
        if (nc->high == nc->low) {
            fill_rcv_buf(nc);
        }
        if (nc->high == 0) {
            nc_dump("Read", buf, r);
            return r;
        }
        l = MIN(len, nc->high - nc->low);
        memcpy(buf + r, nc->rcv_buf + nc->low, l);
        len     -= l;
        r       += l;
        nc->low += l;
    }
    nc_dump("Read", buf, r);
    return r;
}

int nc_read_line(struct nc * nc, unsigned char * buf, size_t maxlen)
{
    size_t r = 0;

    nc_dump("Reading line", NULL, maxlen);
    while (r < maxlen) {
        if (nc_read(nc, buf + r, 1) <= 0) {
            return r;
        }
        if (buf[r] == '\012') {
            buf[r] = '\0';
            if (r > 0 && buf[r-1] == '\015') {
                r--;
                buf[r] = '\0';
            }
            nc_dump("Read line", buf, r);
            return r;
        }
        r++;
    }
    nc_dump("Read line", buf, r);
    return r;
}

int nc_write(struct nc * nc, unsigned char * buf, size_t len)
{
    int r;
    nc_dump("sending...", buf, len);
    r = send(nc->fd, buf, len, 0);
    if (r < 0)
        nc_error("nc_write");
    nc_dump("sent", NULL, 0);
    return r;
}
