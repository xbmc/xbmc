/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
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

#ifndef __XBNETWORK_H__
#define __XBNETWORK_H__

#pragma once

#include "Thread.h"

/*
  A big drawback of the XDK is its lack of socket event notification 
  and the WSAAsyncSelect() method.
  This file provides an implementation of socket event notification by polling the 
  sockets in a separate thread with the select() method.
  Because this happens in a seperate thread, access to the sockets must be
  synchronised. Therefore, this file contains wrappers for the native winsock 
  methods used by FileZilla.
  
  The wrappers provide synchronised access to the winsock methods, enabling the use
  of sockets by multipe threads at a time.
  
*/


char* inet_ntoa (struct in_addr in);


/*
  some helper functions
*/

unsigned long GetLocalIPAddress();

/*
 * Structures returned by network data base library, taken from the
 * BSD file netdb.h.  All addresses are supplied in host order, and
 * returned in network order (suitable for use in system calls).
 */

struct  hostent {
        char    FAR * h_name;           /* official name of host */
        char    FAR * FAR * h_aliases;  /* alias list */
        short   h_addrtype;             /* host address type */
        short   h_length;               /* length of address */
        char    FAR * FAR * h_addr_list; /* list of addresses */
#define h_addr  h_addr_list[0]          /* address, for backward compat */
};

typedef struct hostent FAR *LPHOSTENT;
 
struct hostent FAR * gethostbyname (
  const char FAR * name  
);
 

// winsock defines
#define FD_READ         0x01
#define FD_WRITE        0x02
#define FD_OOB          0x04
#define FD_ACCEPT       0x08
#define FD_CONNECT      0x10
#define FD_CLOSE        0x20


////////////////////////////////////////////////////////////////
// Socket Event Notification classes


///////////////////////////////////////////////////////////////////////////////////
// CAsyncSelectHelper

class CAsyncSelectHelper
{
public:
  CAsyncSelectHelper(SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent);
  ~CAsyncSelectHelper();

  void SetParams(HWND hWnd, unsigned int wMsg, long lEvent);

  void Reset();

  SOCKET mSocket;
  HWND mWnd;
  unsigned int mMsg;
  long mEvent;

  void Lock();
  void Unlock();

  
  CRITICAL_SECTION mCS;
  int mLockCount;

  bool mIsConnecting;
  bool mIsListening;
  bool mSendEnabled;
  bool mReceiveEnabled;
  bool mAcceptEnabled;
  bool mIsConnected;
};

typedef std::list<CAsyncSelectHelper*> CAsyncSelectHelperList;

class CAsyncSelectFindFunctor
{
public:
  CAsyncSelectFindFunctor(SOCKET s)
    :mSocket(s)
  {
  }

  bool operator()(const CAsyncSelectHelper* helper)
  {
    return mSocket == helper->mSocket;
  }

  SOCKET mSocket;
};

///////////////////////////////////////////////////////////////////////////////////
// CAsyncSelectManager 


class CAsyncSelectManager : public CThread
{
public:
  CAsyncSelectManager();
  ~CAsyncSelectManager();
  
  int WSAAsyncSelect(SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent);
  int Accept(SOCKET s, CAsyncSelectHelper* srcHelper);

  virtual DWORD Run();

  CAsyncSelectHelper* GetHelper(SOCKET s);
  
  CAsyncSelectHelperList mAsyncSelectHelperList;
  CCriticalSectionWrapper mAsyncSelectHelperListCS;
  HANDLE mEventStop;

private:
  void AddHelper(CAsyncSelectHelper* Helper);

  bool mIsSuspended;
};

class CSocketLock
{
public:
  CSocketLock(SOCKET psocket);
  ~CSocketLock();

  CAsyncSelectHelper* mHelper;
};


////////////////////////////////////////////////////////////////
// synchronized socket implementation for non-blocking sockets


/* Socket function prototypes */
namespace SyncSocket {
  SOCKET accept(IN SOCKET s, OUT struct sockaddr FAR * addr, IN OUT int FAR * addrlen);
  int bind(IN SOCKET s, IN const struct sockaddr FAR * name, IN int namelen);
  int closesocket(IN SOCKET s);
  int connect(IN SOCKET s, IN const struct sockaddr FAR * name, IN int namelen);
  int ioctlsocket(IN SOCKET s, IN long cmd, IN OUT u_long FAR * argp);
  int getpeername(IN SOCKET s, OUT struct sockaddr FAR * name, IN OUT int FAR * namelen);
  int getsockname(IN SOCKET s, OUT struct sockaddr FAR * name, IN OUT int FAR * namelen);
  int getsockopt(IN SOCKET s, IN int level, IN int optname, OUT char FAR * optval, IN OUT int FAR * optlen);
  int listen(IN SOCKET s, IN int backlog);
  int recv(IN SOCKET s, OUT char FAR * buf, IN int len, IN int flags);
  int recvfrom(IN SOCKET s, OUT char FAR * buf, IN int len, IN int flags, OUT struct sockaddr FAR * from, IN OUT int FAR * fromlen);
  int select(IN int nfds, IN OUT fd_set FAR * readfds, IN OUT fd_set FAR * writefds, IN OUT fd_set FAR *exceptfds, IN const struct timeval FAR * timeout);
  int send(IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags);
  int sendto(IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags, IN const struct sockaddr FAR * to, IN int tolen);
  int setsockopt(IN SOCKET s, IN int level, IN int optname, IN const char FAR * optval, IN int optlen);
  int shutdown(IN SOCKET s, IN int how);
};


/*
  make sure the synchronised socket functions are used
  "using namespace SyncSocket" gives ambiguous call compiler errors in AsyncSocket.cpp
  therefore create defines for the wrappers
  These defines are undefined in xbnetwork.cpp, allowing that file to access the native
  methods
*/
  
#define accept SyncSocket::accept
#define bind   SyncSocket::bind
#define closesocket SyncSocket::closesocket
#define connect SyncSocket::connect
#define ioctlsocket SyncSocket::ioctlsocket
#define getpeername SyncSocket::getpeername
#define getsockname SyncSocket::getsockname
#define getsockopt SyncSocket::getsockopt
#define listen SyncSocket::listen
#define recv SyncSocket::recv
#define recvfrom SyncSocket::recvfrom
#define select SyncSocket::select 
#define send SyncSocket::send
#define sendto SyncSocket::sendto
#define setsockopt SyncSocket::setsockopt
#define shutdown SyncSocket::shutdown


#endif
