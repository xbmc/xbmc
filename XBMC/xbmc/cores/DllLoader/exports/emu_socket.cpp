
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"

#include "..\..\..\DNSNameCache.h"
#include "emu_dummy.h"
#include "emu_socket.h"


#define MAX_SOCKETS 50

static bool m_bSocketsInit = false;
static int m_sockets[MAX_SOCKETS + 1];

void InitSockets()
{
  m_bSocketsInit = true;
  memset(m_sockets , 0 , sizeof(m_sockets));
}

int GetSocketForIndex(int iIndex)
{
  if (iIndex < 3 || iIndex >= MAX_SOCKETS)
  {
    CLog::Log(LOGERROR, "GetSocketForIndex() invalid index:%i", iIndex);
    return -1;
  }
  if (m_sockets[iIndex] == 0)
  {
    CLog::Log(LOGERROR, "GetSocketForIndex() invalid socket for index:%i", iIndex);
    return -1;
  }
  return m_sockets[iIndex];
}

int GetIndexForSocket(SOCKET iSocket)
{
  for (int i = 0; i < MAX_SOCKETS; i++)
  {
    if (m_sockets[i] == iSocket) return i;
  }
  return 0;
}

int AddSocket(int iSocket)
{
  if (!m_bSocketsInit)
  {
    InitSockets();
  }
  for (int i = 3; i < MAX_SOCKETS; i++)
  {
    if (m_sockets[i] == 0)
    {
      m_sockets[i] = iSocket;
      return i;
    }
  }
  return 0;
}

void ReleaseSocket(int iIndex)
{
  if (iIndex < 3 || iIndex >= MAX_SOCKETS)
  {
    CLog::Log(LOGERROR, "ReleaseSocket() invalid index:%i", iIndex);
    return ;
  }
  if (m_sockets[iIndex] == 0)
  {
    CLog::Log(LOGERROR, "ReleaseSocket() invalid socket for index:%i", iIndex);
    return ;
  }
  m_sockets[iIndex] = 0;
}

#ifdef __cplusplus
extern "C"
{
#endif

  int __stdcall dllgethostname(char* name, int namelen)
  {
    if ((unsigned int)namelen < strlen("xbox") + 1) return -1;
    strcpy(name, "xbox");
    return 0;
  }

  static struct mphostent hbn_hostent;
  static char* hbn_cAliases[]= { NULL, NULL }; // only one NULL is needed acutally
  static char* hbn_dwlist1[] = {NULL, NULL, NULL};
  static DWORD hbn_dwList2[] = {0, 0, 0};
  static char hbn_hostname[128];

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

  int __stdcall dllconnect( SOCKET s, const struct sockaddr FAR *name, int namelen)
  {
    int socket = GetSocketForIndex(s);
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
    sprintf(szBuf, "connect returned:%i %i\n", iResult, WSAGetLastError());
    return iResult;
  }
  
  int __stdcall dllsend(SOCKET s, const char *buf, int len, int flags)
  {
    int socket = GetSocketForIndex(s);
    //flags Unsupported; must be zero. [GeminiServer]
    //int iResult = send(socket, buf, len, flags);
    int iResult = send(socket, buf, len, 0);

    int iErr = WSAGetLastError();
    return iResult;
  }

  int __stdcall dllsocket(int af, int type, int protocol)
  {
    int iSocket = socket(af, type, protocol);

    return AddSocket(iSocket);
  }

  int __stdcall dllbind(int s, const struct sockaddr FAR * name, int namelen)
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

    int socket = GetSocketForIndex(s);

    int iResult = bind(socket, name, namelen);
    sprintf(szBuf, "bind returned:%i %i\n", iResult, WSAGetLastError());
    OutputDebugString(szBuf);
    return iResult;
  }

  int __stdcall dllclosesocket(int s)
  {
    int socket = GetSocketForIndex(s);

    int iret = closesocket(socket);
    ReleaseSocket(s);
    return iret;
  }

  int __stdcall dllgetsockopt(int s, int level, int optname, char FAR * optval, int FAR * optlen)
  {
    int socket = GetSocketForIndex(s);

    return getsockopt(socket, level, optname, optval, optlen);
  }

  int __stdcall dllioctlsocket(int s, long cmd, DWORD FAR * argp)
  {
    int socket = GetSocketForIndex(s);

    return ioctlsocket(socket, cmd, argp);
  }

  int __stdcall dllrecv(int s, char FAR * buf, int len, int flags)
  {
    int socket = GetSocketForIndex(s);

    return recv(socket, buf, len, flags);
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
    
    if (readfds)
    {
      for (i = 0; i < readfds->fd_count; ++i)
       FD_SET(GetSocketForIndex(readfds->fd_array[i]), &readset);
    }
    if (writefds)
    {
      for (i = 0; i < writefds->fd_count; ++i)
       FD_SET(GetSocketForIndex(writefds->fd_array[i]), &writeset);
    }
    if (exceptfds)
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
    int socket = GetSocketForIndex(s);

    return sendto(socket, buf, len, flags, to, tolen);
  }
  
  int __stdcall dllsetsockopt(int s, int level, int optname, const char FAR * optval, int optlen)
  {
    int socket = GetSocketForIndex(s);

    return setsockopt(socket, level, optname, optval, optlen);
  }

  int __stdcall dllaccept(SOCKET s, struct sockaddr FAR * addr, OUT int FAR * addrlen)
  {
    int socket = GetSocketForIndex(s);

    int newsocket = accept(socket, addr, addrlen);
    return AddSocket(newsocket);
  }

  int __stdcall dllgetsockname(SOCKET s, struct sockaddr* name, int* namelen)
  {
    int socket = GetSocketForIndex(s);

    return getsockname(socket, name, namelen);
  }

  int __stdcall dlllisten(SOCKET s, int backlog)
  {
    int socket = GetSocketForIndex(s);

    return listen(socket, backlog);
  }

  u_short __stdcall dllntohs(u_short netshort)
  {
    return ntohs(netshort);
  }

  int __stdcall dllrecvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
  {
    int socket = GetSocketForIndex(s);

    return recvfrom(socket, buf, len, flags, from, fromlen);
  }

  int __stdcall dll__WSAFDIsSet(SOCKET s, fd_set* set)
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

  int __stdcall dllshutdown(SOCKET s, int how)
  {
    int socket = GetSocketForIndex(s);

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
    int socket = GetSocketForIndex(s);
    
    return getpeername(socket, name, namelen);
  }

#ifdef __cplusplus
}
#endif
