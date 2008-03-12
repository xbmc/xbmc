#include "include.h"
#include "EventServer.h"
#include "EventPacket.h"
#include "EventClient.h"
#include "Socket.h"
#include <map>
#include <queue>

using namespace EVENTSERVER;
using namespace EVENTPACKET;
using namespace EVENTCLIENT;
using namespace SOCKETS;
using namespace std;

CEventServer::CEventServer()
{
  m_pSocket       = NULL;
  m_pPacketBuffer = NULL;
  
  // set default port
  m_iPort = 22222;

  // default timeout in ms for receiving a single packet
  m_iListenTimeout = 1000;

  // max clients
  m_iMaxClients = 20;
}

void CEventServer::Cleanup()
{
  if (m_pSocket)
  {
    m_pSocket->Close();
    delete m_pSocket;
    m_pSocket = NULL;
  }

  if (m_pPacketBuffer)
  {
    free(m_pPacketBuffer);
    m_pPacketBuffer = NULL;
  }
}

void CEventServer::Run()
{
  CAddress any_addr;
  CSocketListener listener;
  int packetSize = 0;
  
  CLog::Log(LOGNOTICE, "ES: Starting UDP Event server");

  Cleanup();
  
  // create socket and initialize buffer
  m_pSocket = CSocketFactory::CreateUDPSocket();
  m_pPacketBuffer = (unsigned char *)malloc(PACKET_SIZE);

  if (!m_pPacketBuffer)
  {
    CLog::Log(LOGERROR, "ES: Out of memory, could not allocate packet buffer");
    return;
  }

  // bind to IP and start listening on port
  if (!m_pSocket->Bind(any_addr, m_iPort, 10))
  {
    CLog::Log(LOGERROR, "ES: Could not listen on port %d", m_iPort);
    return;
  }
  
  // add our socket to the 'select' listener
  listener.AddSocket(m_pSocket);

  while (!m_bStop)
  {
    // start listening until we timeout
    if (listener.Listen(m_iListenTimeout))
    {
      CAddress addr;
      if ((packetSize = m_pSocket->Read(addr, PACKET_SIZE, (void *)m_pPacketBuffer)) > -1)
      {
        ProcessPacket(addr, packetSize);
      }
    }
    
    // execute events for connected clients
    // ExecuteEvents();

    // refresh client list
    // RefreshClients();

    // broadcast
    // BroadcastBeacon();
  }
}

void CEventServer::ProcessPacket(CAddress& addr, int pSize)
{
  // first check if we have a client for this address
  map<unsigned long, CEventClient*>::iterator iter = m_clients.find(addr.ULong());
  
  if ( iter == m_clients.end() )
  {
    if ( m_clients.size() >= (unsigned int)m_iMaxClients)
    {
      CLog::Log(LOGWARNING, "ES: Cannot accept any more clients, maximum client count reached");
      return;
    }
    
    // new client
    CEventClient* client = new CEventClient ( addr );
    if (client==NULL)
    {
      CLog::Log(LOGERROR, "ES: Out of memory, cannot accept new client connection");
      return;
    }

    m_clients[addr.ULong()] = client;
  }
  
  // send packet to client to parse
  CEventPacket* packet = new CEventPacket(pSize, m_pPacketBuffer);
  if (packet->IsValid())
    m_clients[addr.ULong()]->AddPacket(packet);
}


