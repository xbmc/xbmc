
#include "stdafx.h"
#include "..\DllLoaderContainer.h"

#include "..\..\..\DNSNameCache.h"
#include "..\..\..\xbox\Network.h"
#include "emu_dummy.h"
#include "emu_socket.h"


#define MAX_SOCKETS 100
#define SO_ERROR    0x1007

#define MSG_PEEK 0x2

static bool m_bSocketsInit = false;
typedef struct
{
  SOCKET sock;
  char*  data;
  char*  start;
  char*  end;
} SSocketData;
static SSocketData m_sockets[MAX_SOCKETS + 1];

void InitSockets()
{
  m_bSocketsInit = true;
  memset(m_sockets, 0, sizeof(SSocketData) * (MAX_SOCKETS + 1));
  for(int i = 0; i < MAX_SOCKETS; i++)
    m_sockets[i].sock = INVALID_SOCKET;
}

SOCKET GetSocketForIndex(int iIndex)
{
  if (iIndex < 3 || iIndex >= MAX_SOCKETS)
  {
    CLog::Log(LOGERROR, "GetSocketForIndex() invalid index:%i", iIndex);
    return INVALID_SOCKET;
  }

  if (InterlockedCompareExchangePointer((PVOID*)&m_sockets[iIndex].sock, (PVOID)INVALID_SOCKET, (PVOID)INVALID_SOCKET) == (PVOID)INVALID_SOCKET)
    CLog::Log(LOGERROR, "GetSocketForIndex() invalid socket for index:%i", iIndex);

  return m_sockets[iIndex].sock;
}

int GetIndexForSocket(SOCKET iSocket)
{
  for (int i = 0; i < MAX_SOCKETS; i++)
  {
    if (InterlockedCompareExchangePointer((PVOID*)&m_sockets[i].sock, (PVOID)iSocket, (PVOID)iSocket) == (PVOID)iSocket)
      return i;
  }
  return -1;
}

int AddSocket(SOCKET iSocket)
{
  if(!m_bSocketsInit)
    InitSockets();

  if(iSocket == INVALID_SOCKET)
    return -1;

  for (int i = 3; i < MAX_SOCKETS; i++)
  {
    if (InterlockedCompareExchangePointer((PVOID*)&m_sockets[i].sock, (PVOID)iSocket, (PVOID)INVALID_SOCKET) == (PVOID)INVALID_SOCKET)
    {
      if(m_sockets[i].data) delete m_sockets[i].data;
      m_sockets[i].data = NULL;
      m_sockets[i].start = NULL;
      m_sockets[i].end = NULL;
      return i;
    }
  }
  CLog::Log(LOGERROR, __FUNCTION__" - Unable to add socket to internal list, no space left");
  return -1;
}

void ReleaseSocket(int iIndex)
{
  if (iIndex < 3 || iIndex >= MAX_SOCKETS)
  {
    CLog::Log(LOGERROR, "ReleaseSocket() invalid index:%i", iIndex);
    return ;
  }

  if (InterlockedExchangePointer((PVOID*)&m_sockets[iIndex].sock, (PVOID)INVALID_SOCKET) == (PVOID)INVALID_SOCKET)
    CLog::Log(LOGERROR, "ReleaseSocket() invalid socket for index:%i", iIndex);

  if(m_sockets[iIndex].data) delete m_sockets[iIndex].data;
  m_sockets[iIndex].data = NULL;
  m_sockets[iIndex].start = NULL;
  m_sockets[iIndex].end = NULL;
}

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
      if(g_network.IsAvailable())
        hbn_dwList2[0] = inet_addr(g_network.m_networkinfo.ip);

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

  int __stdcall dllconnect(int s, const struct sockaddr FAR *name, int namelen)
  {
    SOCKET socket = GetSocketForIndex(s);
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

    int iResult = connect( socket, name, namelen);
    if(iResult == SOCKET_ERROR)
    {
      errno = WSAGetLastError();
      sprintf(szBuf, "connect returned:%i %i\n", iResult, WSAGetLastError());
      OutputDebugString(szBuf);
    }

    return iResult;
  }
  
  int __stdcall dllsend(int s, const char *buf, int len, int flags)
  {
    SOCKET socket = GetSocketForIndex(s);
    //flags Unsupported; must be zero.
    int res = send(socket, buf, len, 0);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }

  int __stdcall dllsocket(int af, int type, int protocol)
  {
    SOCKET sock = socket(af, type, protocol);
    if(sock == INVALID_SOCKET)
      errno = WSAGetLastError();
    return AddSocket(sock);;
  }

  int __stdcall dllbind(int s, const struct sockaddr FAR * name, int namelen)
  {
    SOCKET socket = GetSocketForIndex(s);
    struct sockaddr_in address2;

    if( name->sa_family == AF_INET 
    &&  namelen >= sizeof(sockaddr_in) )
    {
      struct sockaddr_in* address = (struct sockaddr_in*)name;

      CLog::Log(LOGDEBUG, "dllbind: family:%i port:%i ip:%i.%i.%i.%i\n",
            address->sin_family,
            ntohs(address->sin_port),
            address->sin_addr.S_un.S_un_b.s_b1,
            address->sin_addr.S_un.S_un_b.s_b2,
            address->sin_addr.S_un.S_un_b.s_b3,
            address->sin_addr.S_un.S_un_b.s_b4);

      
      if( address->sin_addr.S_un.S_addr == inet_addr(g_network.m_networkinfo.ip)
      ||  address->sin_addr.S_un.S_addr == inet_addr("127.0.0.1") )
      {
        // local xbox, correct for xbox stack
        address2 = *address;
        address2.sin_addr.S_un.S_addr = 0;
        name = (sockaddr*)&address2;
        namelen = sizeof(address2);
      }
    }    

    int iResult = bind(socket, name, namelen);
    if(iResult == SOCKET_ERROR)
    {
      errno = WSAGetLastError();
      CLog::Log(LOGERROR, "bind returned:%i %i\n", iResult, WSAGetLastError());
    }
    return iResult;
  }

  int __stdcall dllclosesocket(int s)
  {
    SOCKET socket = GetSocketForIndex(s);

    int iResult = closesocket(socket);
    if(iResult == SOCKET_ERROR)
      errno = WSAGetLastError();

    ReleaseSocket(s);
    return iResult;
  }

  int __stdcall dllgetsockopt(int s, int level, int optname, char FAR * optval, int FAR * optlen)
  {
    SOCKET socket = GetSocketForIndex(s);

    if(optname == SO_ERROR)
    {
      /* unsupported option */
      /* just hope there was no error, we could probably check connetion state if we wanted */
      *((int*)optval) = 0;
      return 0;
    }
    else
      return getsockopt(socket, level, optname, optval, optlen);
  }

  int __stdcall dllioctlsocket(int s, long cmd, DWORD FAR * argp)
  {
    if (s < 3 || s >= MAX_SOCKETS)
    {
      CLog::Log(LOGERROR, "dllioctlsocket invalid index:%i", s);
      return SOCKET_ERROR;
    }
    SSocketData& socket = m_sockets[s];

    int res = ioctlsocket(socket.sock, cmd, argp);
    if( res == 0 && cmd == FIONREAD && socket.data )
      *argp += socket.end - socket.start;

    return res;
  }

  int __stdcall dllrecv(int s, char FAR * buf, int len, int flags)
  {
    if (s < 3 || s >= MAX_SOCKETS)
    {
      CLog::Log(LOGERROR, "dllrecv invalid index:%i", s);
      return SOCKET_ERROR;
    }
    SSocketData& socket = m_sockets[s];
    int len2, len3;

    if(flags & MSG_PEEK)
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - called with MSG_PEEK set, attempting workaround");
      // work around for peek, it will give garbage as data
      
      if(socket.data == NULL)
      {
        socket.data = new char[len];
        len2 = recv(socket.sock, socket.data, len, 0);
        if(len2 == SOCKET_ERROR)
        {
          SAFE_DELETE(socket.data);
          return SOCKET_ERROR;
        }
        socket.start = socket.data;
        socket.end = socket.start + len2;
      }
      len2 = min(len, socket.end - socket.start);
      memcpy(buf, socket.start, len2);
      return len2;
    }

    if(flags)
      CLog::Log(LOGWARNING, __FUNCTION__" - called with flags %d that will be ignored", flags);
    flags = 0;

    len2 = 0;
    if(socket.start < socket.end)
    {
      len2 = min(len, socket.end - socket.start);
      memcpy(buf, socket.start, len2);
      socket.start += len2;
      buf++;
      len--;
    }      

    if(socket.start >= socket.end)
    {
      delete[] socket.data;
      socket.data = NULL;
      socket.start = NULL;
      socket.end = NULL;
    }

    if(len == 0)
      return 0;

    len3 = recv(socket.sock, buf, len, flags);
    if(len3 == SOCKET_ERROR)
      return SOCKET_ERROR;

    return len2 + len3;
  }

  int __stdcall dllselect(int nfds, fd_set FAR * readfds, fd_set FAR * writefds, fd_set FAR *exceptfds, const struct timeval FAR * timeout)
  {
    fd_set readset, writeset, exceptset;
    unsigned int i;

    fd_set* preadset = &readset;
    fd_set* pwriteset = &writeset;
    fd_set* pexceptset = &exceptset;
    
    if (!readfds) preadset = NULL;
    if (!writefds) pwriteset = NULL;
    if (!exceptfds) pexceptset = NULL;
    
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_ZERO(&exceptset);

    if(readfds)
    {
      for (i = 0; i < readfds->fd_count; ++i)
       FD_SET(GetSocketForIndex(readfds->fd_array[i]), &readset);
    }
    if(writefds)
    {
      for (i = 0; i < writefds->fd_count; ++i)
       FD_SET(GetSocketForIndex(writefds->fd_array[i]), &writeset);
    }
    if(exceptfds)
    {
      for (i = 0; i < exceptfds->fd_count; ++i)
       FD_SET(GetSocketForIndex(exceptfds->fd_array[i]), &exceptset);
    }
    
    int ifd = select(nfds, preadset, pwriteset, pexceptset, timeout);

    // convert real socket identifiers back to our custom ones and clean results
    if (readfds)
    {
      FD_ZERO(readfds);
      for (i = 0; i < readset.fd_count; i++) FD_SET(GetIndexForSocket(readset.fd_array[i]), readfds);
    }
    if (writefds)
    {
      FD_ZERO(writefds);
      for (i = 0; i < writeset.fd_count; i++) FD_SET(GetIndexForSocket(writeset.fd_array[i]), writefds);
    }
    if (exceptfds)
    {
      FD_ZERO(exceptfds);
      for (i = 0; i < exceptset.fd_count; i++) FD_SET(GetIndexForSocket(exceptset.fd_array[i]), exceptfds);
    }

    return ifd;
  }

  int __stdcall dllsendto(int s, const char FAR * buf, int len, int flags, const struct sockaddr FAR * to, int tolen)
  {
    SOCKET socket = GetSocketForIndex(s);
    int res = sendto(socket, buf, len, flags, to, tolen);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }
  
  int __stdcall dllsetsockopt(int s, int level, int optname, const char FAR * optval, int optlen)
  {
    SOCKET socket = GetSocketForIndex(s);
    int res = setsockopt(socket, level, optname, optval, optlen);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }

  int __stdcall dllaccept(int s, struct sockaddr FAR * addr, OUT int FAR * addrlen)
  {
    SOCKET socket = GetSocketForIndex(s);
    SOCKET res = accept(socket, addr, addrlen);
    if(res == INVALID_SOCKET)
      errno = WSAGetLastError();
    return AddSocket(res);
  }

  int __stdcall dllgetsockname(int s, struct sockaddr* name, int* namelen)
  {
    SOCKET socket = GetSocketForIndex(s);
    int res = getsockname(socket, name, namelen);
    if(res == SOCKET_ERROR)
    {
      errno = WSAGetLastError();
      return res;
    }

    if( name->sa_family == AF_INET 
    && *namelen >= sizeof(sockaddr_in) )
    {
      sockaddr_in *addr = (sockaddr_in*)name;
      if( addr->sin_port > 0 && addr->sin_addr.S_un.S_addr == 0 )
      {
        // unspecifed address will always be on local xbox ip
        // some dll's assume this will return a proper address
        // even if windows standard doesn't gurantee it
        if( g_network.IsAvailable() )
          addr->sin_addr.S_un.S_addr = inet_addr(g_network.m_networkinfo.ip);
      }
    }
    return res;
  }

  int __stdcall dlllisten(int s, int backlog)
  {
    SOCKET socket = GetSocketForIndex(s);
    int res = listen(socket, backlog);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();
    return res;
  }

  u_short __stdcall dllntohs(u_short netshort)
  {
    return ntohs(netshort);
  }

  int __stdcall dllrecvfrom(int s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
  {
    SOCKET socket = GetSocketForIndex(s);
    int res = recvfrom(socket, buf, len, flags, from, fromlen);
    if(res == SOCKET_ERROR)
      errno = WSAGetLastError();    
    return res;
  }

  int __stdcall dll__WSAFDIsSet(int s, fd_set* set)
  {
    fd_set real_set; // the set with real socket id's
    int real_socket = GetSocketForIndex(s);
    
    FD_ZERO(&real_set);
    if (set)
    {
      for (unsigned int i = 0; i < set->fd_count; ++i)
        FD_SET(GetSocketForIndex(set->fd_array[i]), &real_set);
    }      

    return __WSAFDIsSet(real_socket, &real_set);
  }

  int __stdcall dllshutdown(int s, int how)
  {
    SOCKET socket = GetSocketForIndex(s);
    return shutdown(socket, how);
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
  
  int __stdcall dllgetpeername(int s, struct sockaddr FAR *name, int FAR *namelen)
  {
    int socket = GetSocketForIndex(s);
    return getpeername(socket, name, namelen);
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
