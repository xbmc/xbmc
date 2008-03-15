#include "stdafx.h"

#ifdef HAS_EVENT_SERVER

#include "Socket.h"
#include <vector>

using namespace SOCKETS;
using namespace std;

/**********************************************************************/
/* CPosixUDPSocket                                                    */
/**********************************************************************/

bool CPosixUDPSocket::Bind(CAddress& addr, int port, int range)
{
  // close any existing sockets
  Close();

  // create the socket
  m_iSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (m_iSock < 0)
  {
    CLog::Log(LOGERROR, "UDP: Could not create socket");
    CLog::Log(LOGERROR, "UDP: %s", strerror(errno));
    return false;
  }

  // make sure we can reuse the address
  int yes = 1;
  if (setsockopt(m_iSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1)
  {
    CLog::Log(LOGWARNING, "UDP: Could not enable the address reuse options");
    CLog::Log(LOGWARNING, "UDP: %s", strerror(errno));
  }
  
  // set the port
  m_addr = addr;
  m_iPort = port;
  m_addr.saddr.sin_port = htons(port);

  // bind the socket ( try from port to port+range )
  while (m_iPort <= port + range)
  {
    if (bind(m_iSock, (struct sockaddr*)&m_addr.saddr, sizeof(m_addr.saddr)) < 0)
    {
      CLog::Log(LOGWARNING, "UDP: Error binding socket on port %d", m_iPort);
      CLog::Log(LOGWARNING, "UDP: %s", strerror(errno));
      m_iPort++;
    }
    else
    {
      CLog::Log(LOGNOTICE, "UDP: Listening on port %d", port);
      SetBound();
      SetReady();
      break;
    }
  }

  // check for errors
  if (!Bound())
  {
    CLog::Log(LOGERROR, "UDP: No suitable port found");
    Close();
    return false;
  }

  return true;
}

void CPosixUDPSocket::Close()
{
  if (m_iSock>=0)
  {
    close(m_iSock);
    m_iSock = -1;
  }
  SetBound(false);
  SetReady(false);
}

int CPosixUDPSocket::Read(CAddress& addr, const int buffersize, void *buffer)
{
  return (int)recvfrom(m_iSock, buffer, (size_t)buffersize, 0, 
                       (struct sockaddr*)&addr.saddr, &addr.size);  
}

int CPosixUDPSocket::SendTo(const CAddress& addr, const int buffersize, 
                          const void *buffer)
{
  return (int)sendto(m_iSock, buffer, (size_t)buffersize, 0, 
                     (const struct sockaddr*)&addr.saddr, 
                     sizeof(addr.saddr));
}

/**********************************************************************/
/* CSocketFactory                                                     */
/**********************************************************************/

CUDPSocket* CSocketFactory::CreateUDPSocket()
{
#ifdef _LINUX
  return new CPosixUDPSocket();
#endif
  return NULL;
}

/**********************************************************************/
/* CSocketListener                                                    */
/**********************************************************************/

CSocketListener::CSocketListener()
{
  Clear();
}

void CSocketListener::AddSocket(CBaseSocket *sock)
{
  // WARNING: not threadsafe (which is ok for now)

  if (sock && sock->Ready())
  {
    m_sockets.push_back(sock);
    FD_SET(sock->Socket(), &m_fdset);
    if (sock->Socket() > m_iMaxSockets)
      m_iMaxSockets = sock->Socket();
  }
}

bool CSocketListener::Listen(int timeout)
{
  if (m_sockets.size()==0)
    return false;

  m_iReadyCount = 0;
  m_iCurrentSocket = 0;
  
  FD_ZERO(&m_fdset);
  for (unsigned int i = 0 ; i<m_sockets.size() ; i++)
  {
    FD_SET(m_sockets[i]->Socket(), &m_fdset);
  }

  // set our timeout
  struct timeval tv;
  int rem = timeout % 1000;
  tv.tv_usec = rem * 1000;
  tv.tv_sec = timeout / 1000;

  if (timeout<0)
  {
    m_iReadyCount = select(m_iMaxSockets+1, &m_fdset, NULL, NULL, NULL);
  }
  else
  {
    m_iReadyCount = select(m_iMaxSockets+1, &m_fdset, NULL, NULL, &tv);
  }

  if (m_iReadyCount<0)
  {
    CLog::Log(LOGERROR, "SOCK: Error selecting socket(s)");
    Clear();
    return false;
  }
  else
  {
    m_iCurrentSocket = 0;
    return (m_iReadyCount>0);
  }
}

void CSocketListener::Clear()
{
  m_sockets.clear();
  FD_ZERO(&m_fdset);
  m_iReadyCount = 0;
  m_iMaxSockets = 0;
  m_iCurrentSocket = 0;
}

CBaseSocket* CSocketListener::GetFirstReadySocket()
{
  if (m_iReadyCount<=0)
    return NULL;
  
  for (int i = 0 ; i < (int)m_sockets.size() ; i++)
  {
    if (FD_ISSET((m_sockets[i])->Socket(), &m_fdset)) 
    {
      m_iCurrentSocket = i;
      return m_sockets[i];
    }
  }
  return NULL;
}

CBaseSocket* CSocketListener::GetNextReadySocket()
{
  if (m_iReadyCount<=0)
    return NULL;
  
  for (int i = m_iCurrentSocket+1 ; i<(int)m_sockets.size() ; i++)
  {
    if (FD_ISSET(m_sockets[i]->Socket(), &m_fdset)) 
    {
      m_iCurrentSocket = i;
      return m_sockets[i];
    }
  }
  return NULL;
}

#endif // HAS_EVENT_SERVER
