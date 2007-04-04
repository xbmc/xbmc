
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"

#include "..\..\..\DNSNameCache.h"
#include "emu_dummy.h"
#include "emu_socket.h"


#define MAX_SOCKETS 100
#define SO_ERROR    0x1007

#define MSG_PEEK 0x2


extern "C"
{

#ifdef _XBOX
  int __stdcall dllgethostname(char* name, int namelen)
  {
    if ((unsigned int)namelen < strlen("xbox") + 1) return -1;
    strcpy(name, "xbox");
    return 0;
  }
#else
  int __stdcall dllgethostname(char* name, int namelen)
  {
    return gethostname(name, namelen);
  }
#endif

  static struct mphostent hbn_hostent;
  static char* hbn_cAliases[]= { NULL, NULL }; // only one NULL is needed acutally
  static char* hbn_dwlist1[] = {NULL, NULL, NULL};
  static DWORD hbn_dwList2[] = {0, 0, 0};
  static char hbn_hostname[128];

#ifdef _XBOX
  struct mphostent* __stdcall dllgethostbyname(const char* name)
  {
    CStdString strIpAdres;

    hbn_hostent.h_name = hbn_hostname;
    hbn_hostname[0] = '\0'; // clear hostname
    hbn_hostent.h_aliases = hbn_cAliases;
    hbn_hostent.h_addrtype = AF_INET;
    hbn_hostent.h_length = 4;
    hbn_hostent.h_addr_list = hbn_dwlist1;
    hbn_hostent.h_addr_list[0] = (char*)hbn_dwList2;

    dllgethostname(hbn_hostname, 128);

    if (!strcmp(hbn_hostname, name))
    {
      XNADDR xna;
      DWORD dwState;
      do dwState = XNetGetTitleXnAddr(&xna);
      while (dwState == XNET_GET_XNADDR_PENDING);

      hbn_dwList2[0] = xna.ina.S_un.S_addr;
      return &hbn_hostent;
    }
    if (CDNSNameCache::Lookup(name, strIpAdres))
    {
      strcpy(hbn_hostname, name);
      hbn_dwList2[0] = inet_addr(strIpAdres.c_str());
      return &hbn_hostent;
    }
    else
    {
      OutputDebugString("DNS:no ipadres found\n");
      return NULL;
    }
  }

  // libRV.lib depends on gethostbyname
  struct hostent;
  struct hostent* gethostbyname(const char* name)
  {
    return (hostent*)dllgethostbyname(name);
  }
#else
  struct mphostent* __stdcall dllgethostbyname(const char* name)
  {
    return (mphostent*)gethostbyname(name);
  }
#endif

  int __stdcall dllconnect(SOCKET s, const struct sockaddr FAR *name, int namelen)
  {
    struct sockaddr_in* pTmp = (struct sockaddr_in*)name;
    char szBuf[128];
    sprintf(szBuf, "connect: family:%i port:%i ip:%i.%i.%i.%i\n",
            pTmp->sin_family,
            ntohs(pTmp->sin_port),
            pTmp->sin_addr.S_un.S_addr&0xff,
            (pTmp->sin_addr.S_un.S_addr&0xff00) >> 8,
            (pTmp->sin_addr.S_un.S_addr&0xff0000) >> 16,
            (pTmp->sin_addr.S_un.S_addr) >> 24
           );
    OutputDebugString(szBuf);

    int iResult = connect( s, name, namelen);
    if(iResult == SOCKET_ERROR)
    {
      errno = WSAGetLastError();
      sprintf(szBuf, "connect returned:%i %i\n", iResult, WSAGetLastError());
      OutputDebugString(szBuf);
    }

    return iResult;
  }
  
  int __stdcall dllsend(SOCKET s, const char *buf, int len, int flags)
  {
    //flags Unsupported; must be zero.
    int res = send(s, buf, len, 0);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }

  SOCKET __stdcall dllsocket(int af, int type, int protocol)
  {
    SOCKET s = socket(af, type, protocol);
    if(s == INVALID_SOCKET)
      errno = WSAGetLastError();
    return s;
  }

  int __stdcall dllbind(SOCKET s, const struct sockaddr FAR * name, int namelen)
  {
    struct sockaddr_in* pTmp = (struct sockaddr_in*)name;
    char szBuf[128];
    sprintf(szBuf, "dllbind: family:%i port:%i ip:%i.%i.%i.%i\n",
            pTmp->sin_family,
            ntohs(pTmp->sin_port),
            pTmp->sin_addr.S_un.S_addr&0xff,
            (pTmp->sin_addr.S_un.S_addr&0xff00) >> 8,
            (pTmp->sin_addr.S_un.S_addr&0xff0000) >> 16,
            (pTmp->sin_addr.S_un.S_addr) >> 24
           );
    OutputDebugString(szBuf);

    int iResult = bind(s, name, namelen);
    if(iResult == SOCKET_ERROR)
    {
      errno = WSAGetLastError();
      sprintf(szBuf, "bind returned:%i %i\n", iResult, WSAGetLastError());
      OutputDebugString(szBuf);
    }
    return iResult;
  }

  int __stdcall dllclosesocket(SOCKET s)
  {
    int iResult = closesocket(s);
    if(iResult == SOCKET_ERROR)
      errno = WSAGetLastError();

    return iResult;
  }

  int __stdcall dllgetsockopt(SOCKET s, int level, int optname, char FAR * optval, int FAR * optlen)
  {
    if(optname == SO_ERROR)
    {
      /* unsupported option */
      /* just hope there was no error, we could probably check connetion state if we wanted */
      *((int*)optval) = 0;
      return 0;
    }
    else
      return getsockopt(s, level, optname, optval, optlen);
  }

  int __stdcall dllioctlsocket(SOCKET s, long cmd, DWORD FAR * argp)
  {
    return ioctlsocket(s, cmd, argp);
  }

  int __stdcall dllrecv(SOCKET s, char FAR * buf, int len, int flags)
  {
    if(flags & MSG_PEEK)
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - called with MSG_PEEK set, attempting workaround");
      // work around for peek, it will give garbage as data
      // and will always block, till data or error
      unsigned long size=0;
      while(1)
      {
        if(ioctlsocket(s, FIONREAD, &size))
          return -1;
        if(size)
          break;
        Sleep(1);
      }
      return min((int)size, len);
    }

    if(flags)
      CLog::Log(LOGWARNING, __FUNCTION__" - called with flags %d that will be ignored", flags);

    flags = 0;
    return recv(s, buf, len, flags);
  }

#define FD_CLR_INVALID(set) \
  do { \
    int v, l = sizeof(int); \
    unsigned int i; \
    for(i = 0;i<set->fd_count;i++) { \
      if( getsockopt(set->fd_array[i], SOL_SOCKET, SO_TYPE, (char*)&v, &l) == SOCKET_ERROR ) { \
          FD_CLR(set->fd_array[i], set); \
          i--; \
      } \
    } \
  } while(0)

  int __stdcall dllselect(int nfds, fd_set FAR * readfds, fd_set FAR * writefds, fd_set FAR *exceptfds, const struct timeval FAR * timeout)
  {
#if 0
    // hack for some dll that do selects on multiple sockets, where it has 
    // closed atleast one of the sockets before, but still tries to check for data
    if(readfds)
      FD_CLR_INVALID(readfds);
    if(writefds)
      FD_CLR_INVALID(writefds);
    if(exceptfds)
      FD_CLR_INVALID(exceptfds);
#endif
    int res = select(nfds, readfds, writefds, exceptfds, timeout);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();

    return res;
  }

  int __stdcall dllsendto(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR * to, int tolen)
  {
    int res = sendto(s, buf, len, flags, to, tolen);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }
  
  int __stdcall dllsetsockopt(SOCKET s, int level, int optname, const char FAR * optval, int optlen)
  {
    return setsockopt(s, level, optname, optval, optlen);
  }

  SOCKET __stdcall dllaccept(SOCKET s, struct sockaddr FAR * addr, OUT int FAR * addrlen)
  {
    int res = accept(s, addr, addrlen);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }

  int __stdcall dllgetsockname(SOCKET s, struct sockaddr* name, int* namelen)
  {
    int res = getsockname(s, name, namelen);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }

  int __stdcall dlllisten(SOCKET s, int backlog)
  {
    int res = listen(s, backlog);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }

  u_short __stdcall dllntohs(u_short netshort)
  {
    return ntohs(netshort);
  }

  int __stdcall dllrecvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
  {
    return recvfrom(s, buf, len, flags, from, fromlen);
  }

  int __stdcall dll__WSAFDIsSet(SOCKET s, fd_set* set)
  {
    return __WSAFDIsSet(s, set);
  }

  int __stdcall dllshutdown(SOCKET s, int how)
  {
    return shutdown(s, how);
  }

  char* __stdcall dllinet_ntoa (struct in_addr in)
  {
    static char _inetaddress[32];
    sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
    return _inetaddress;
  }

  static struct mphostent hba_hostent;
  static char* hba_cAliases[]= { NULL, NULL }; // only one NULL is needed acutally
  static char* hba_dwlist1[] = {NULL, NULL, NULL};
  static DWORD hba_dwList2[] = {0, 0, 0};
  static char hba_hostname[128];

#ifdef _XBOX
  struct mphostent* __stdcall dllgethostbyaddr(const char* addr, int len, int type)
  {
    CLog::Log(LOGWARNING, "Untested function dllgethostbyaddr called!");
    XNADDR xna;
    DWORD dwState;

    hba_hostent.h_name = hba_hostname;
    hba_hostname[0] = '\0'; // clear hostname
    hba_hostent.h_aliases = hba_cAliases;
    hba_hostent.h_addrtype = AF_INET;
    hba_hostent.h_length = 4;
    hba_hostent.h_addr_list = hba_dwlist1;
    hba_hostent.h_addr_list[0] = (char*)hba_dwList2;

    do dwState = XNetGetTitleXnAddr(&xna);
    while (dwState == XNET_GET_XNADDR_PENDING);

    if (!memcmp(addr, &(xna.ina.s_addr), 4))
    {
      //get title hostent
      dllgethostname(hba_hostname, 128);
      hba_dwList2[0] = xna.ina.S_un.S_addr;
      return &hba_hostent;
    }
    else
    {
      //get client hostent
      in_addr client = {{addr[0], addr[1], addr[2], addr[3]}};
      strcpy(hba_hostname, dllinet_ntoa(client));
      hba_dwList2[0] = client.S_un.S_addr;
      return &hba_hostent;
    }
    return 0;
  }
#endif

  struct servent* __stdcall dllgetservbyname(const char* name, const char* proto)
  {
    OutputDebugString("TODO: getservbyname\n");
    return NULL;
  }

  struct servent* __stdcall dllgetservbyport(int port, const char* proto)
  {
    OutputDebugString("TODO: getservbyport\n");
    return NULL;
  }

  struct protoent* __stdcall dllgetprotobyname(const char* name)
  {
    OutputDebugString("TODO: getprotobyname\n");
    return NULL;
  }
  
  int __stdcall dllgetpeername(SOCKET s, struct sockaddr FAR *name, int FAR *namelen)
  {
    return getpeername(s, name, namelen);
  }

  int getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
  int __stdcall dllgetaddrinfo(const char* nodename, const char* servname, const struct addrinfo* hints, struct addrinfo** res)
  {
    return getaddrinfo(nodename, servname, hints, res);
  }

  int getnameinfo(const struct sockaddr *sa, size_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags);
  int __stdcall dllgetnameinfo(const struct sockaddr *sa, size_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags)
  {
    return getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
  }
  
  void freeaddrinfo(struct addrinfo *ai);
	void __stdcall dllfreeaddrinfo(struct addrinfo *ai)
	{
	  return freeaddrinfo(ai);
	}	
}
