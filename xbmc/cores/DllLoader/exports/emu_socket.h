#ifndef _EMU_SOCKET_H
#define _EMU_SOCKET_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "emu_socket\emu_socket.h"

#ifdef __cplusplus
extern "C"
{
#endif

  struct mphostent* __stdcall dllgethostbyname(const char* name);
#ifdef _XBOX
  extern "C" char* inet_ntoa(in_addr in);
#endif
  int __stdcall dllconnect(int s, const struct sockaddr FAR *name, int namelen);
  int __stdcall dllsend(int s, const char FAR *buf, int len, int flags);
  int __stdcall dllsocket(int af, int type, int protocol);
  int __stdcall dllbind(int s, const struct sockaddr FAR * name, int namelen);
  int __stdcall dllclosesocket(int s);
  int __stdcall dllgetsockopt(int s, int level, int optname, char FAR * optval, int FAR * optlen);
  int __stdcall dllioctlsocket(int s, long cmd, DWORD FAR * argp);
  int __stdcall dllrecv(int s, char FAR * buf, int len, int flags);
  int __stdcall dllselect(int nfds, fd_set FAR * readfds, fd_set FAR * writefds, fd_set FAR *exceptfds, const struct timeval FAR * timeout);
  int __stdcall dllsendto(int s, const char FAR * buf, int len, int flags, const struct sockaddr FAR * to, int tolen);
  int __stdcall dllsetsockopt(int s, int level, int optname, const char FAR * optval, int optlen);
  int __stdcall dll__WSAFDIsSet(int fd, fd_set* set);

  int __stdcall dllaccept(int s, struct sockaddr FAR * addr, OUT int FAR * addrlen);
  int __stdcall dllgethostname(char* name, int namelen);
  int __stdcall dllgetsockname(int s, struct sockaddr* name, int* namelen);
  int __stdcall dlllisten(int s, int backlog);
  u_short __stdcall dllntohs(u_short netshort);
  int __stdcall dllrecvfrom(int s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);
  int __stdcall dllshutdown(int s, int how);
  char* __stdcall dllinet_ntoa (struct in_addr in);

  struct servent* __stdcall dllgetservbyname(const char* name,const char* proto);
  struct protoent* __stdcall dllgetprotobyname(const char* name);
  int __stdcall dllgetpeername(int s, struct sockaddr FAR *name, int FAR *namelen);
  struct servent* __stdcall dllgetservbyport(int port, const char* proto);
#ifdef _XBOX
  struct mphostent* __stdcall dllgethostbyaddr(const char* addr, int len, int type);
#endif

  int __stdcall dllgetaddrinfo(const char* nodename, const char* servname, const struct addrinfo* hints, struct addrinfo** res);
  int __stdcall dllgetnameinfo(const struct sockaddr *sa, size_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags);
  void __stdcall dllfreeaddrinfo(struct addrinfo *ai);
  
#ifdef __cplusplus
}
#endif

#endif // _EMU_SOCKET_H
