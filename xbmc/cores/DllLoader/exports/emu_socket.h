
#ifndef _EMU_SOCKET_H
#define _EMU_SOCKET_H

#include "emu_socket\emu_socket.h"

#ifdef __cplusplus
extern "C"
{
#endif

  struct mphostent* __stdcall dllgethostbyname(const char* name);
#ifdef _XBOX
  extern "C" char* inet_ntoa(in_addr in);
#endif
  int __stdcall dllconnect(SOCKET s, const struct sockaddr FAR *name, int namelen);
  int __stdcall dllsend(SOCKET s, const char FAR *buf, int len, int flags);
  SOCKET __stdcall dllsocket(int af, int type, int protocol);
  int __stdcall dllbind(SOCKET s, const struct sockaddr FAR * name, int namelen);
  int __stdcall dllclosesocket(SOCKET s);
  int __stdcall dllgetsockopt(SOCKET s, int level, int optname, char FAR * optval, int FAR * optlen);
  int __stdcall dllioctlsocket(SOCKET s, long cmd, DWORD FAR * argp);
  int __stdcall dllrecv(SOCKET s, char FAR * buf, int len, int flags);
  int __stdcall dllselect(int nfds, fd_set FAR * readfds, fd_set FAR * writefds, fd_set FAR *exceptfds, const struct timeval FAR * timeout);
  int __stdcall dllsendto(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR * to, int tolen);
  int __stdcall dllsetsockopt(SOCKET s, int level, int optname, const char FAR * optval, int optlen);
  int __stdcall dll__WSAFDIsSet(SOCKET fd, fd_set* set);

  SOCKET __stdcall dllaccept(SOCKET s, struct sockaddr FAR * addr, OUT int FAR * addrlen);
  int __stdcall dllgethostname(char* name, int namelen);
  int __stdcall dllgetsockname(SOCKET s, struct sockaddr* name, int* namelen);
  int __stdcall dlllisten(SOCKET s, int backlog);
  u_short __stdcall dllntohs(u_short netshort);
  int __stdcall dllrecvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);
  int __stdcall dllshutdown(SOCKET s, int how);
  char* __stdcall dllinet_ntoa (struct in_addr in);

  struct servent* __stdcall dllgetservbyname(const char* name,const char* proto);
  struct protoent* __stdcall dllgetprotobyname(const char* name);
  int __stdcall dllgetpeername(SOCKET s, struct sockaddr FAR *name, int FAR *namelen);
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