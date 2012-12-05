/* ------------------------------------------------------------------
 * Copyright (C) 1998-2010 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//     O S C L C O N F I G _ I O

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig_io.h
 *  \brief This file contains common typedefs based on the ANSI C limits.h header
 *
 *  This header file should work for any ANSI C compiler to determine the
 *  proper native C types to use for OSCL integer types.
 */


#ifndef OSCLCONFIG_IO_H_INCLUDED
#define OSCLCONFIG_IO_H_INCLUDED

#ifndef OSCLCONFIG_H_INCLUDED
#include "osclconfig.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/vfs.h>
#include <dirent.h>
//#include <glob.h>
#if (OSCL_HAS_ANSI_STDLIB_SUPPORT)
#if (OSCL_HAS_UNIX_SUPPORT)
#include <sys/stat.h>
#endif
#endif

//For File I/O
#define OSCL_HAS_GLOB 0
#define OSCL_HAS_ANSI_FILE_IO_SUPPORT 1
#define OSCL_HAS_ANSI_64BIT_FILE_IO_SUPPORT 0
#define OSCL_HAS_MSWIN_FILE_IO_SUPPORT 0
#define OSCL_HAS_SYMBIAN_COMPATIBLE_IO_FUNCTION 0
#define OSCL_HAS_NATIVE_FILE_CACHE_ENABLE 1
#define OSCL_FILE_BUFFER_MAX_SIZE   32768
#define OSCL_HAS_PV_FILE_CACHE  0
#define OSCL_HAS_LARGE_FILE_SUPPORT 1

//For Sockets
#define OSCL_HAS_SYMBIAN_SOCKET_SERVER 0
#define OSCL_HAS_SYMBIAN_DNS_SERVER 0
#define OSCL_HAS_BERKELEY_SOCKETS 1
#define OSCL_HAS_SOCKET_SUPPORT 1

//basic socket types
typedef int TOsclSocket;
typedef struct sockaddr_in TOsclSockAddr;
typedef socklen_t TOsclSockAddrLen;
typedef struct ip_mreq TIpMReq;

//Init addr macro, inet_addr returns an uint32
#define OsclValidInetAddr(addr) (inet_addr(addr)!=INADDR_NONE)

//address conversion macro-- from string to network address.
#define OsclMakeSockAddr(sockaddr,port,addrstr,ok)\
    sockaddr.sin_family=OSCL_AF_INET;\
    sockaddr.sin_port=htons(port);\
    int32 result=inet_aton((const char*)addrstr,&sockaddr.sin_addr);\
    ok=(result!=0);

//address conversion macro-- from network address to string
#define OsclUnMakeSockAddr(sockaddr,addrstr)\
    addrstr=inet_ntoa(sockaddr.sin_addr);
//address conversion macro-- from string to inaddr
#define OsclMakeInAddr(in_addr,addrstr,ok)\
    int32 result = inet_aton((const char*)addrstr, &in_addr);\
    ok=(result!=0);

//address conversion macro-- from inaddr to string
#define OsclUnMakeInAddr(in_addr,addrstr)\
    addrstr=inet_ntoa(in_addr);

//wrappers for berkeley socket calls
#define OsclSetRecvBufferSize(s,val,ok,err) \
        ok=(setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&val, sizeof(int)) !=-1);\
        if (!ok)err=errno

#define OsclBind(s,addr,ok,err)\
    TOsclSockAddr* tmpadr = &addr;\
    sockaddr* sadr = OSCL_STATIC_CAST(sockaddr*, tmpadr);\
    ok=(bind(s,sadr,sizeof(addr))!=(-1));\
    if (!ok)err=errno

#define OsclSetSockOpt(s,optLevel,optName,optVal,optLen,ok,err)\
    ok=(setsockopt(s,optLevel,optName,OSCL_STATIC_CAST(const char*,optVal),optLen) != (-1));\
    if (!ok)err=errno
#define OsclJoin(s,addr,ok,err)\
{\
        struct ip_mreq mreq; \
            void* p = &addr; \
        ok=(bind(s,(sockaddr*)p,sizeof(addr))!=(-1));\
        mreq.imr_multiaddr.s_addr = addr.sin_addr.s_addr ; \
        mreq.imr_interface.s_addr = htonl(INADDR_ANY); \
        ok=(setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq))!=(-1)); \
        if (!ok)err=errno;\
}


#define OsclListen(s,size,ok,err)\
    ok=(listen(iSocket,qSize)!=(-1));\
    if (!ok)err=errno

#define OsclAccept(s,accept_s,ok,err,wouldblock)\
    accept_s=accept(s,NULL,NULL);\
    ok=(accept_s!=(-1));\
    if (!ok){err=errno;wouldblock=(err==EAGAIN||err==EWOULDBLOCK);}

#define OsclSetNonBlocking(s,ok,err)\
    ok=(fcntl(s,F_SETFL,O_NONBLOCK)!=(-1));\
    if (!ok)err=errno

#define OsclShutdown(s,how,ok,err)\
    ok=(shutdown(iSocket,how)!=(-1));\
    if (!ok)err=errno

#define OsclSocket(s,fam,type,prot,ok,err)\
    s=socket(fam,type,prot);\
    ok=(s!=(-1));\
    if (!ok)err=errno

#define OsclSendTo(s,buf,len,addr,ok,err,nbytes,wouldblock)\
    TOsclSockAddr* tmpadr = &addr;\
    sockaddr* sadr = OSCL_STATIC_CAST(sockaddr*, tmpadr);\
    nbytes=sendto(s,(const void*)(buf),(size_t)(len),0,sadr,(socklen_t)sizeof(addr));\
    ok=(nbytes!=(-1));\
    if (!ok){err=errno;wouldblock=(err==EAGAIN||err==EWOULDBLOCK);}

#define OsclSend(s,buf,len,ok,err,nbytes,wouldblock)\
    nbytes=send(s,(const void*)(buf),(size_t)(len),0);\
    ok=(nbytes!=(-1));\
    if (!ok){err=errno;wouldblock=(err==EAGAIN||err==EWOULDBLOCK);}

#define OsclCloseSocket(s,ok,err)\
    ok=(close(s)!=(-1));\
    if (!ok)err=errno

#define OsclConnect(s,addr,ok,err,wouldblock)\
    TOsclSockAddr* tmpadr = &addr;\
    sockaddr* sadr = OSCL_STATIC_CAST(sockaddr*, tmpadr);\
    ok=(connect(s,sadr,sizeof(addr))!=(-1));\
    if (!ok){err=errno;wouldblock=(err==EINPROGRESS);}
#define OsclGetPeerName(s,name,namelen,ok,err)\
    ok=(getpeername(s,(sockaddr*)&name,(socklen_t*)&namelen) != (-1) );\
    if (!ok)err=errno

#define OsclGetAsyncSockErr(s,ok,err)\
    int opterr;socklen_t optlen=sizeof(opterr);\
    ok=(getsockopt(s,SOL_SOCKET,SO_ERROR,(void *)&opterr,&optlen)!=(-1));\
    if(ok)err=opterr;else err=errno;

#define OsclPipe(x)         pipe(x)
#define OsclReadFD(fd,buf,cnt)  read(fd,buf,cnt)
#define OsclWriteFD(fd,buf,cnt) write(fd,buf,cnt)

//unix reports connect completion in write set in the getsockopt
//error.
#define OsclConnectComplete(s,wset,eset,success,fail,ok,err)\
    success=fail=false;\
    if (FD_ISSET(s,&eset))\
    {fail=true;OsclGetAsyncSockErr(s,ok,err);}\
    else if (FD_ISSET(s,&wset))\
    {OsclGetAsyncSockErr(s,ok,err);if (ok && err==0)success=true;else fail=true;}

#define OsclRecv(s,buf,len,ok,err,nbytes,wouldblock)\
    nbytes=recv(s,(void *)(buf),(size_t)(len),0);\
    ok=(nbytes!=(-1));\
    if (!ok){err=errno;wouldblock=(err==EAGAIN);}

#define OsclRecvFrom(s,buf,len,paddr,paddrlen,ok,err,nbytes,wouldblock)\
{\
void* p=paddr;\
nbytes=recvfrom(s,(void*)(buf),(size_t)(len),0,(struct sockaddr*)p,paddrlen);\
    ok=(nbytes!=(-1));\
    if (!ok){err=errno;wouldblock=(err==EAGAIN);}\
}


#define OsclSocketSelect(nfds,rd,wr,ex,timeout,ok,err,nhandles)\
    nhandles=select(nfds,&rd,&wr,&ex,&timeout);\
    ok=(nhandles!=(-1));\
    if (!ok)err=errno

//there's not really any socket startup needed on unix, but
//you need to define a signal handler for SIGPIPE to avoid
//broken pipe crashes.
#define OsclSocketStartup(ok)\
    signal(SIGPIPE,SIG_IGN);\
    ok=true

#define OsclSocketCleanup(ok)\
    signal(SIGPIPE,SIG_DFL);\
    ok=true

//hostent type
typedef struct hostent TOsclHostent;

//wrapper for gethostbyname
#define OsclGethostbyname(name,hostent,ok,err)\
    hostent=gethostbyname((const char*)name);\
    ok=(hostent!=NULL);\
    if (!ok)err=errno;

//extract dotted address from a hostent
#define OsclGetDottedAddr(hostent,dottedaddr,ok)\
    long *_hostaddr=(long*)hostent->h_addr_list[0];\
    struct in_addr _inaddr;\
    _inaddr.s_addr=*_hostaddr;\
    dottedaddr=inet_ntoa(_inaddr);\
    ok=(dottedaddr!=NULL);

//extract dotted address from a hostent into the vector of OsclNetworkAddress
#define OsclGetDottedAddrVector(hostent,dottedaddr,dottedaddrvect,ok)\
    if(dottedaddrvect)\
    {\
    long **_addrlist=(long**)hostent->h_addr_list;\
    for(int i = 0; _addrlist[i] != NULL; i++){\
        struct in_addr _inaddr;\
        _inaddr.s_addr=*_addrlist[i];\
        OsclNetworkAddress addr(inet_ntoa(_inaddr), 0);\
        dottedaddrvect->push_back(addr);\
    }\
    if (!dottedaddrvect->empty())\
        {dottedaddr->port = dottedaddrvect->front().port; dottedaddr->ipAddr.Set(dottedaddrvect->front().ipAddr.Str());}\
    ok=(!dottedaddrvect->empty() && (((*dottedaddrvect)[0]).ipAddr.Str() != NULL));\
    }\
    else\
    {\
        char *add;\
        OsclGetDottedAddr(hostent,add,ok);\
        if(ok) dottedaddr->ipAddr.Set(add);\
    }

//socket shutdown codes
#define OSCL_SD_RECEIVE SHUT_RD
#define OSCL_SD_SEND SHUT_WR
#define OSCL_SD_BOTH SHUT_RDWR

//address family codes
#define OSCL_AF_INET AF_INET

//socket type codes
#define OSCL_SOCK_STREAM SOCK_STREAM
#define OSCL_SOCK_DATAGRAM SOCK_DGRAM

//IP protocol codes
#define OSCL_IPPROTO_IP  IPPROTO_IP
#define OSCL_IPPROTO_TCP IPPROTO_TCP
#define OSCL_IPPROTO_UDP IPPROTO_UDP

//Socket option Levels
#define OSCL_SOL_SOCKET SOL_SOCKET
#define OSCL_SOL_IP     IPPROTO_IP
#define OSCL_SOL_TCP    IPPROTO_TCP
#define OSCL_SOL_UDP    IPPROTO_UDP

//Socket Option Values (level = IP)
#define OSCL_SOCKOPT_IP_MULTICAST_TTL   IP_MULTICAST_TTL
#define OSCL_SOCKOPT_IP_ADDMEMBERSHIP   IP_ADD_MEMBERSHIP
#define OSCL_SOCKOPT_IP_TOS             IP_TOS

//Socket Option Values (level = Socket)
#define OSCL_SOCKOPT_SOL_REUSEADDR      SO_REUSEADDR
//End sockets

// file IO support
#if (OSCL_HAS_LARGE_FILE_SUPPORT)
typedef off64_t TOsclFileOffset;
#else
typedef int32 TOsclFileOffset;
#endif

#include "osclconfig_io_check.h"

#endif

