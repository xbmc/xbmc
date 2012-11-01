/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
#ifndef OSCLCONFIG_IO_CHECK_H_INCLUDED
#define OSCLCONFIG_IO_CHECK_H_INCLUDED

/**
OSCL_HAS_ANSI_FILE_IO_SUPPORT macro should be set to 1 if
the target platform supports the ANSI C file I/O functions (fopen, fread, etc).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_ANSI_FILE_IO_SUPPORT
#error "ERROR: OSCL_HAS_ANSI_FILE_IO_SUPPORT has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_ANSI_64BIT_FILE_IO_SUPPORT macro should be set to 1 if
the target platform supports the 64-bit ANSI C file I/O functions (fopen, fread, etc).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_ANSI_64BIT_FILE_IO_SUPPORT
#error "ERROR: OSCL_HAS_ANSI_64BIT_FILE_IO_SUPPORT has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_MSWIN_FILE_IO_SUPPORT macro should be set to 1 if
the target platform supports the ANSI C file I/O functions (fopen, fread, etc).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_MSWIN_FILE_IO_SUPPORT
#error "ERROR: OSCL_HAS_MSWIN_FILE_IO_SUPPORT has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_SYMBIAN_COMPATIBLE_IO_FUNCTION macro should be set to 1 if
the target platform supports the Symbian file I/O functions (RFile, RFs).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SYMBIAN_COMPATIBLE_IO_FUNCTION
#error "ERROR: OSCL_HAS_SYMBIAN_COMPATIBLE_IO_FUNCTION has to be defined to either 1 or 0"
#endif

/**
On Symbian platforms only:
OSCL_HAS_NATIVE_DUPLICATE_FILE_HANDLE  macro should be set to 1 if
the target platform supports the Symbian file I/O function RFile::Duplicate.
Otherwise it should be set to 0.
*/
#if (OSCL_HAS_SYMBIAN_COMPATIBLE_IO_FUNCTION)
#ifndef OSCL_HAS_NATIVE_DUPLICATE_FILE_HANDLE
#error "ERROR: OSCL_HAS_NATIVE_DUPLICATE_FILE_HANDLE has to be defined to either 1 or 0"
#endif
#endif


/**
OSCL_HAS_NATIVE_FILE_CACHE_ENABLE macro should be set to 1 if
the target platform includes native file cache capability.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_NATIVE_FILE_CACHE_ENABLE
#error "ERROR: OSCL_HAS_NATIVE_FILE_CACHE_ENABLE has to be defined to either 1 or 0"
#endif


/**
OSCL_HAS_PV_FILE_CACHE macro should be set to 1 if
the target platform includes PV file cache capability.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_PV_FILE_CACHE
#error "ERROR: OSCL_HAS_PV_FILE_CACHE has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_LARGE_FILE_SUPPORT macro should be set to 1 if
the target platform supports more than 32bit file I/O capability.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_LARGE_FILE_SUPPORT
#error "ERROR: OSCL_HAS_LARGE_FILE_SUPPORT has to be defined to either 1 or 0"
#endif

/**
type TOsclFileOffset should be defined as the type used for file size
and offsets on the target platform.
Example:
typedef size_t TOsclFileOffset;
*/
typedef TOsclFileOffset __verify__TOsclFileOffset__defined__;


/**
OSCL_FILE_BUFFER_MAX_SIZE macro should be set to
the desired size of the file I/O cache in bytes.
Otherwise it should be set to 0.
*/
#ifndef OSCL_FILE_BUFFER_MAX_SIZE
#error "ERROR: OSCL_FILE_BUFFER_MAX_SIZE has to be defined to a numeric value"
#endif

/**
OSCL_HAS_SOCKET_SUPPORT macro should be set to 1 if
the target platform supports sockets of any type.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SOCKET_SUPPORT
#error "ERROR: OSCL_HAS_SOCKET_SUPPORT has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_SYMBIAN_SOCKET_SERVER macro should be set to
1 if the platform supports Symbian socket API (RSocket, RSocketServ).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SYMBIAN_SOCKET_SERVER
#error "ERROR: OSCL_HAS_SYMBIAN_SOCKET_SERVER has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_SYMBIAN_DNS_SERVER macro should be set to
1 if the platform supports Symbian Host Resolver API (RHostResolver).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SYMBIAN_DNS_SERVER
#error "ERROR: OSCL_HAS_SYMBIAN_DNS_SERVER has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_BERKELEY_SOCKETS macro should be set to
1 if the platform supports Berkeley style socket API, including
non-blocking I/O and a 'select' call.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_BERKELEY_SOCKETS
#error "ERROR: OSCL_HAS_BERKELEY_SOCKETS has to be defined to either 1 or 0"
#endif

/**
For platforms with Berkeley type sockets,
TOsclSocket typedef should be set to platform native socket type.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
typedef TOsclSocket __TOsclSocketCheck___;
#endif

/**
For platforms with Berkeley type sockets,
TOsclSockAddr typedef should be set to platform native socket address type.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
typedef TOsclSockAddr __TOsclSockAddrCheck___;
#endif

/**
For platforms with Berkeley type sockets,
TOsclSockAddrLen typedef should be set to platform native socket address
length type.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
typedef TOsclSockAddrLen __TOsclSockAddrLenCheck___;
#endif

/**
For platforms with Berkeley type sockets,
OsclBind(s,addr,ok,err) must be defined to
an expression that does a bind call.
's' and 'addr' are the socket and address parameters
to the bind command.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false and 'err' must be set
to the bind error.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclBind
#error "ERROR: OsclBind(s,addr,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclListen(s,size,ok,err) must be defined to
an expression that does a listen call and sets 'ok' and 'err'
to indicate the result.
's' and 'size' are the socket and queue size args to the listen
call.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false and 'err' must be set
to the listen error.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclListen
#error "ERROR: OsclListen(s,size,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclAccept(s,accept_s,ok,err,wouldblock) must be defined to
an expression that does an accept call and sets 'ok', 'err',
and 'wouldblock' to indicate the result.
's' and 'accept_s' are the socket and accept socket args to the
accept call.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false and 'err' must be set
to the accept error.  Additionally 'wouldblock' must be set to true
if the error code indicates that the socket is non-blocking and
would block, or to false otherwise.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclAccept
#error "ERROR: OsclAccept(s,accept_s,ok,err,wouldblock) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclSetNonBlocking(s,ok,err) must be defined to
an expression that sets socket 's' to non-blocking I/O mode
and sets 'ok' and 'err' to indicate the result.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false and 'err' must be set
to the error.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclSetNonBlocking
#error "ERROR: OsclSetNonBlocking(s,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclShutdown(s,how,ok,err) must be defined to
an expression that does a shutdown call and sets 'ok' and 'err'
to indicate the result.
's' and 'how' are the socket and shutdown type args to the
shutdown call.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false and 'err' must be set
to the shutdown error.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclShutdown
#error "ERROR: OsclShutdown(s,how,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclSocket(s,fam,type,prot,ok,err) must be defined to
an expression that does a socket creation call and sets 'ok'
and 'err' to indicate the result.
's', 'fam', 'type', and 'prot' are the socket, family, type, and
protocol args to the socket call.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false and 'err' must be set
to the socket error.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclSocket
#error "ERROR: OsclSocket(s,fam,type,prot,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclSendTo(s,buf,len,addr,ok,err,nbytes,wouldblock) must be defined
to an expression that does a sendto call and sets 'ok, 'err', 'nbytes',
and 'wouldblock' to indicate the result.
's', 'buf', 'len', 'flags' and 'addr' are the arguments to the sendto
call.
On success, 'ok' must be set to true, and 'nbytes' must be set to
the number of bytes sent.
On failure, 'ok' must be set to false 'err' must be set
to the socket error.  Additionally 'wouldblock' must be set to true
if the error code indicates that the socket is non-blocking and
would block, or to false otherwise.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclSendTo
#error "ERROR: OsclSendTo(s,buf,len,flags,addr,ok,err,nbytes,wouldblock) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclSend(s,buf,len,ok,err,nbytes,wouldblock) must be defined to
an expression that does a send operation and sets 'ok, 'err', 'nbytes',
and 'wouldblock' to indicate the result.
's', 'buf', and 'len' are the args to the send call.
On success, 'ok' must be set to true, and 'nbytes' must be set to
the number of bytes sent.
On failure, 'ok' must be set to false 'err' must be set
to the socket error.  Additionally 'wouldblock' must be set to true
if the error code indicates that the socket is non-blocking and
would block, or to false otherwise.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclSend
#error "ERROR: OsclSend(s,buf,len,ok,err,nbytes,wouldblock) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclCloseSocket(s,ok,err) must be defined to
an expression that closes socket 's' and sets 'ok and 'err'
to indicate the result.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false and 'err' must be set
to the close error.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclCloseSocket
#error "ERROR: OsclCloseSocket(s,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclConnect(s,addr,ok,err,wouldblock) must be defined to
an expression that does a connect call and sets 'ok', 'err',
and 'wouldblock' to indicate the result.
's' and 'addr' are the socket and address args to the connect call.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false 'err' must be set
to the socket error.  Additionally 'wouldblock' must be set to true
if the error code indicates that the socket is non-blocking and
would block, or to false otherwise.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclConnect
#error "ERROR: OsclConnect(s,addr,ok,err,wouldblock) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclConnectComplete(s,wset,eset,success,fail,ok,err) must be set
to an expression that checks for completion of a connect operation
on a non-blocking socket and sets 'success', 'fail', 'ok', and 'err'
to indicate the result.
's' is the socket, 'wset' is the write set from the select call,
'eset' is the exception set from the select call.
If connect is not yet complete, 'success' and 'fail' must be
set false.
On connect success, 'success' must be set true.
On conneect failure, 'success' must be set false, 'fail' must be
set true. Additionally, the call attempts to retrieve the connect error.
If the connect error is obtained, 'ok' is set true and 'err' contains
the error.  If the connect error is not obtained, 'ok' is set false
and 'err' is the error code from the attempt.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclConnectComplete
#error "ERROR: OsclConnectComplete(s,wset,eset,success,fail,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclRecv(s,buf,len,ok,err,nbytes,wouldblock) must be defined to
an expression that does a recv call and sets 'ok', 'err', 'nbytes',
and 'wouldblock' to indicate the result.
's', 'buf', and 'len' are the arguments to the recv call.
On success, 'ok' must be set to true, and 'nbytes' must be set to
the number of bytes received.
On failure, 'ok' must be set to false 'err' must be set
to the socket error.  Additionally 'wouldblock' must be set to true
if the error code indicates that the socket is non-blocking and
would block, or to false otherwise.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclRecv
#error "ERROR: OsclRecv(s,buf,len,ok,err,nbytes,wouldblock) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclRecvFrom(s,buf,len,addr,addrlen,ok,err,nbytes,wouldblock) must be defined to
an expression that does a recvfrom call and sets 'ok', 'err', 'nbytes',
and 'wouldblock' to indicate the result.
's', 'buf', 'len', 'paddr', and 'paddrlen' are the arguments to the recvfrom call.
On success, 'ok' must be set to true, 'nbytes' must be set to
the number of bytes received, and 'paddr' must be set to the source address.
On failure, 'ok' must be set to false 'err' must be set
to the socket error.  Additionally 'wouldblock' must be set to true
if the error code indicates that the socket is non-blocking and
would block, or to false otherwise.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclRecvFrom
#error "ERROR: OsclRecvFrom(s,buf,len,paddr,paddrlen,ok,err,nbytes,wouldblock) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclSocketSelect(nfds,rd,wr,ex,timeout,ok,err,nhandles) must be defined to
an expression that does a select call and sets 'ok', 'err', and
'nhandles' to indicate the result.
'nfds', 'rd', 'wr', 'ex', and 'timeout' are the arguments to the
select call.
On success, 'ok' must be set to true, and 'nhandles' must be set to
the number of socket handles with activitiy detected.
On failure, 'ok' must be set to false 'err' must be set
to the select error.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclSocketSelect
#error "ERROR: OsclSocketSelect(nfds,rd,wr,ex,timeout,ok,err,nhandles) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclSocketStartup(ok) must be defined to
an expression that does any necessary startup of the socket system
and sets 'ok' to indicate the result.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclSocketStartup
#error "ERROR: OsclSocketStartup(ok) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclSocketCleanup(ok) must be defined to
an expression that does any necessary cleanup of the socket system
and sets 'ok' to indicate the result.
On success, 'ok' must be set to true.
On failure, 'ok' must be set to false.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclSocketCleanup
#error "ERROR: OsclSocketCleanup(ok) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclGetAsyncSockErr(s,ok,err) must be defined to
an expression that does a getsockopt call to retrieve a socket error
and sets 'ok' and 'err' to indicate the result.
's' is the socket argument to the getsockopt call.
On success, 'ok' must be set true and 'err' must be set to the
error retrieved.
On failure, 'ok' must be set false and 'err' must be set to the
error from the getsockopt call.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclGetAsyncSockErr
#error "ERROR: OsclGetAsyncSockErr(s,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
TOsclHostent typedef should be set to platform native hostent type.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
typedef TOsclHostent __TOsclHostentCheck___;
#endif

/**
For platforms with Berkeley type sockets,
OsclGethostbyname(name,hostent,ok,err) must be defined to
an expression that does a gethostbyname call on host 'name'
and sets 'hostent', 'ok' and 'err' to indicate the result.
'name' is the name argument to the gethostbyname call.
On success, 'ok' must be set true and 'hostent' must be set to
the TOsclHostent* retrieved.
On failure, 'ok' must be set false and 'err' must be set to the
error from the gethostbyname call.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclGethostbyname
#error "ERROR: OsclGethostbyname(name,hostent,ok,err) has to be defined"
#endif
#endif

/**
For platforms with Berkeley type sockets,
OsclGetDottedAddr(hostent,dottedaddr,ok) must be defined to
an expression that does extracts an address in dotted decimal
notation from a hostent structure.
'hostent' is the TOsclHostent*,
'dottedaddr' is a char* output containing the dotted address,
and 'ok' is a bool that should be set true on success, false on failure.
*/
#if OSCL_HAS_BERKELEY_SOCKETS
#ifndef OsclGetDottedAddr
#error "ERROR: OsclGetDottedAddr(hostent,dottedaddr,ok) has to be defined"
#endif
#endif


/**
For platforms in which file descriptors created with a pipe() command can be
used with the select() system call the following 3 macros must be defined
*/

#if OSCL_HAS_SELECTABLE_PIPES
#ifndef OsclPipe
#error "ERROR: OsclPipe(pipearray) has to be defined"
#endif
#endif

#if OSCL_HAS_SELECTABLE_PIPES
#ifndef OsclReadFD
#error "ERROR: OsclReadFD(fd,buffer,cnt) has to be defined"
#endif
#endif

#if OSCL_HAS_SELECTABLE_PIPES
#ifndef OsclWriteFD
#error "ERROR: OsclWriteFD(fd,buffer,cnt) has to be defined"
#endif
#endif

#if OSCL_HAS_SOCKET_SUPPORT
/**
OsclValidInetAddr must be defined to a boolean expression to
evaluate whether an address is proper IP4 format.
'addr' is a char* containing the address string.
*/
#ifndef OsclValidInetAddr
#error "ERROR: OsclValidInetAddr(addr) must be defined"
#endif
#endif

#if OSCL_HAS_SOCKET_SUPPORT
/**
OSCL_SD_RECEIVE, OSCL_SD_SEND, and OSCL_SD_BOTH must be defined to
the platform-specific socket shutdown codes.
*/
#ifndef OSCL_SD_RECEIVE
#error "ERROR: OSCL_SD_RECEIVE has to be defined"
#endif
#ifndef OSCL_SD_SEND
#error "ERROR: OSCL_SD_SEND has to be defined"
#endif
#ifndef OSCL_SD_BOTH
#error "ERROR: OSCL_SD_BOTH has to be defined"
#endif
#endif

#if OSCL_HAS_SOCKET_SUPPORT
/**
OSCL_AF_INET must be defined to the platform-specific
network address family codes for INET.
*/
#ifndef OSCL_AF_INET
#error "ERROR: OSCL_AF_INET has to be defined"
#endif
#endif

#if OSCL_HAS_SOCKET_SUPPORT
/**
OSCL_SOCK_STREAM and OSCL_SOCK_DATAGRAM must be defined to
the platform-specific socket type codes.
*/
#ifndef OSCL_SOCK_STREAM
#error "ERROR: OSCL_SOCK_STREAM has to be defined"
#endif
#ifndef OSCL_SOCK_DATAGRAM
#error "ERROR: OSCL_SOCK_DATAGRAM has to be defined"
#endif
#endif

#if OSCL_HAS_SOCKET_SUPPORT
/**
OSCL_IPPROTO_TCP and OSCL_IPPROTO_UDP must be defined to
the platform-specific IP protocol codes.
*/
#ifndef OSCL_IPPROTO_TCP
#error "ERROR: OSCL_IPPROTO_TCP has to be defined"
#endif
#ifndef OSCL_IPPROTO_UDP
#error "ERROR: OSCL_IPPROTO_UDP has to be defined"
#endif
#endif


#endif // OSCLCONFIG_IO_CHECK_H_INCLUDED


