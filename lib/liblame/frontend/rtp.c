/* $Id: rtp.c,v 1.16.8.1 2008/08/05 14:16:06 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca       /* predefined by HP cc +Olibcalls */
char   *alloca();
#   endif
#  endif
# endif
#endif

#include <stdio.h>
#include <stdarg.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "console.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

struct rtpbits {
    int     sequence:16;     /* sequence number: random */
    int     pt:7;            /* payload type: 14 for MPEG audio */
    int     m:1;             /* marker: 0 */
    int     cc:4;            /* number of CSRC identifiers: 0 */
    int     x:1;             /* number of extension headers: 0 */
    int     p:1;             /* is there padding appended: 0 */
    int     v:2;             /* version: 2 */
};

struct rtpheader {           /* in network byte order */
    struct rtpbits b;
    int     timestamp;       /* start: random */
    int     ssrc;            /* random */
    int     iAudioHeader;    /* =0?! */
};

void
initrtp(struct rtpheader *foo)
{
    foo->b.v = 2;
    foo->b.p = 0;
    foo->b.x = 0;
    foo->b.cc = 0;
    foo->b.m = 0;
    foo->b.pt = 14;     /* MPEG Audio */
#ifdef FEFE
    foo->b.sequence = 42;
    foo->timestamp = 0;
#else
    foo->b.sequence = rand() & 65535;
    foo->timestamp = rand();
#endif
    foo->ssrc = rand();
    foo->iAudioHeader = 0;
}

int
sendrtp(int fd, struct sockaddr_in *sSockAddr, struct rtpheader *foo, const void *data, int len)
{
    char   *buf = alloca(len + sizeof(struct rtpheader));
    int    *cast = (int *) foo;
    int    *outcast = (int *) buf;
    outcast[0] = htonl(cast[0]);
    outcast[1] = htonl(cast[1]);
    outcast[2] = htonl(cast[2]);
    outcast[3] = htonl(cast[3]);
    memmove(buf + sizeof(struct rtpheader), data, len);
    return sendto(fd, buf, len + sizeof(*foo), 0,
                  (struct sockaddr *) sSockAddr, sizeof(*sSockAddr));
/*  return write(fd,buf,len+sizeof(*foo))==len+sizeof(*foo); */
}

/* create a sender socket. */
int
makesocket(char *szAddr, unsigned short port, unsigned char TTL, struct sockaddr_in *sSockAddr)
{
    int     iRet, iLoop = 1;
    struct sockaddr_in sin;
    unsigned char cTtl = TTL;
    char    cLoop = 0;
    unsigned int tempaddr;

    int     iSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSocket < 0) {
        error_printf("socket() failed.\n");
        exit(1);
    }

    tempaddr = inet_addr(szAddr);
    sSockAddr->sin_family = sin.sin_family = AF_INET;
    sSockAddr->sin_port = sin.sin_port = htons(port);
    sSockAddr->sin_addr.s_addr = tempaddr;

    iRet = setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, &iLoop, sizeof(int));
    if (iRet < 0) {
        error_printf("setsockopt SO_REUSEADDR failed\n");
        exit(1);
    }

    if ((ntohl(tempaddr) >> 28) == 0xe) {
        /* only set multicast parameters for multicast destination IPs */
        iRet = setsockopt(iSocket, IPPROTO_IP, IP_MULTICAST_TTL, &cTtl, sizeof(char));
        if (iRet < 0) {
            error_printf("setsockopt IP_MULTICAST_TTL failed.  multicast in kernel?\n");
            exit(1);
        }

        cLoop = 1;      /* !? */
        iRet = setsockopt(iSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &cLoop, sizeof(char));
        if (iRet < 0) {
            error_printf("setsockopt IP_MULTICAST_LOOP failed.  multicast in kernel?\n");
            exit(1);
        }
    }

    return iSocket;
}




#if 0
/* */
/* code contributed by Anonymous source.  Supposed to be much better */
/* then original code, but only seems to run on windows with MSVC.   */
/* and I cannot test it */
/* */
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct rtpbits {
    int     sequence:16;     /* sequence number: random */
    int     pt:7;            /* payload type: 14 for MPEG audio */
    int     m:1;             /* marker: 0 */
    int     cc:4;            /* number of CSRC identifiers: 0 */
    int     x:1;             /* number of extension headers: 0 */
    int     p:1;             /* is there padding appended: 0 */
    int     v:2;             /* version: 2 */
};

struct rtpheader {           /* in network byte order */
    struct rtpbits b;
    int     timestamp;       /* start: random */
    int     ssrc;            /* random */
    int     iAudioHeader;    /* =0?! */
};

void
rtp_initialization(struct rtpheader *foo)
{
    foo->b.v = 2;
    foo->b.p = 0;
    foo->b.x = 0;
    foo->b.cc = 0;
    foo->b.m = 0;
    foo->b.pt = 14;     /* MPEG Audio */
#ifdef FEFE
    foo->b.sequence = 42;
    foo->timestamp = 0;
#else
    foo->b.sequence = rand() & 65535;
    foo->timestamp = rand();
#endif
    foo->ssrc = rand();
    foo->iAudioHeader = 0;
}

int
rtp_send(SOCKET s, struct rtpheader *foo, void *data, int len)
{
    char   *buffer = malloc(len + sizeof(struct rtpheader));
    int    *cast = (int *) foo;
    int    *outcast = (int *) buffer;
    int     count, size;

    outcast[0] = htonl(cast[0]);
    outcast[1] = htonl(cast[1]);
    outcast[2] = htonl(cast[2]);
    outcast[3] = htonl(cast[3]);
    memmove(buffer + sizeof(struct rtpheader), data, len);
/*    return sendto (fd,buf,len+sizeof(*foo),0,(struct sockaddr *)sSockAddr,sizeof(*sSockAddr)); */
/*  return write(fd,buf,len+sizeof(*foo))==len+sizeof(*foo); */
    size = len + sizeof(*foo);
    count = send(s, buffer, size, 0);
    free(buffer);

    return count != size;
}

/* create a sender socket. */
int
rtp_socket(SOCKET * ps, char *address, unsigned short port, int TTL)
{
/*    int iRet ; */
    int     iLoop = 1;
/*    struct  sockaddr_in sin ; */
    char    cTTL = (char) TTL;
    char    cLoop = 0;
/*    unsigned int tempaddr ; */
    BOOL    True = TRUE;
    INT     error;
    char   *c = "";
    UINT    ip;
    PHOSTENT host;
    SOCKET  s;
    SOCKADDR_IN source, dest;
#if 0
    int     s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        error_printf("socket() failed.\n");
        exit(1);
    }

    tempaddr = inet_addr(address);
    sSockAddr->sin_family = sin.sin_family = AF_INET;
    sSockAddr->sin_port = sin.sin_port = htons(port);
    sSockAddr->sin_addr.s_addr = tempaddr;

    iRet = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *) &iLoop, sizeof(int));
    if (iRet < 0) {
        error_printf("setsockopt SO_REUSEADDR failed\n");
        exit(1);
    }

    if ((ntohl(tempaddr) >> 28) == 0xe) {
        /* only set multicast parameters for multicast destination IPs */
        iRet = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &cTTL, sizeof(char));
        if (iRet < 0) {
            error_printf("setsockopt IP_MULTICAST_TTL failed.    multicast in kernel?\n");
            exit(1);
        }

        cLoop = 1;      /* !? */
        iRet = setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &cLoop, sizeof(char));
        if (iRet < 0) {
            error_printf("setsockopt IP_MULTICAST_LOOP failed.    multicast in kernel?\n");
            exit(1);
        }
    }
#endif
    source.sin_family = AF_INET;
    source.sin_addr.s_addr = htonl(INADDR_ANY);
    source.sin_port = htons(0);

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(address);

    if (!strcmp(address, "255.255.255.255")) {
    }
    else if (dest.sin_addr.s_addr == INADDR_NONE) {
        host = gethostbyname(address);

        if (host) {
            dest.sin_addr = *(PIN_ADDR) host->h_addr;
        }
        else {
            printf("Unknown host %s\r\n", address);
            return 1;
        }
    }

    dest.sin_port = htons((u_short) port);

    ip = ntohl(dest.sin_addr.s_addr);

    if (IN_CLASSA(ip))
        c = "class A";
    if (IN_CLASSB(ip))
        c = "class B";
    if (IN_CLASSC(ip))
        c = "class C";
    if (IN_CLASSD(ip))
        c = "class D";
    if (ip == INADDR_LOOPBACK)
        c = "loopback";
    if (ip == INADDR_BROADCAST)
        c = "broadcast";

    s = socket(AF_INET, SOCK_DGRAM, PF_UNSPEC);

    if (s == INVALID_SOCKET) {
        error = WSAGetLastError();
        printf("socket () error %d\r\n", error);
        return error;
    }

    error = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *) &True, sizeof(BOOL));

    error = bind(s, (struct sockaddr *) &source, sizeof(source));

    if (error == SOCKET_ERROR) {
        error = WSAGetLastError();
        printf("bind () error %d\r\n", error);
        closesocket(s);
        return error;
    }

    if (ip == INADDR_BROADCAST) {
        printf("broadcast %s:%u %s\r\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), c);

        error = setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)
                           &True, sizeof(BOOL));

        if (error == SOCKET_ERROR) {
            error = WSAGetLastError();
            printf("setsockopt (%u, SOL_SOCKET, SO_BROADCAST, ...) error %d\r\n", s, error);
            closesocket(s);
            return error;
        }
    }

    if (IN_CLASSD(ip)) {
        printf("multicast %s:%u %s\r\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), c);

/*        error = setsockopt (s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) &TTL, sizeof (int)) ; */
        error = setsockopt(s, IPPROTO_IP, 3, (const char *) &TTL, sizeof(int));

        if (error == SOCKET_ERROR) {
            error = WSAGetLastError();
            printf("setsockopt (%u, IPPROTO_IP, IP_MULTICAST_TTL, ...) error %d\r\n", s, error);
            closesocket(s);
            return error;
        }
    }

    error = connect(s, (PSOCKADDR) & dest, sizeof(SOCKADDR_IN));

    if (error == SOCKET_ERROR) {
        printf("connect: error %d\n", WSAGetLastError());
        closesocket(s);
        return error;
    }

    *ps = s;

    return 0;
}



#endif
