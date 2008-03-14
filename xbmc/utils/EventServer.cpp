#include "stdafx.h"
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

CEventServer* CEventServer::m_pInstance = NULL;

CEventServer::CEventServer()
{
  m_pSocket       = NULL;
  m_pPacketBuffer = NULL;
  m_bStop         = false;
  m_pThread       = NULL;
  m_bRunning      = false;
  
  // set default port
  m_iPort = 22222;

  // default timeout in ms for receiving a single packet
  m_iListenTimeout = 1000;

  // max clients
  m_iMaxClients = 20;
}

CEventServer* CEventServer::GetInstance()
{
  if (!m_pInstance)
  {
    m_pInstance = new CEventServer();
  }
  return m_pInstance;
}

void CEventServer::StartServer()
{
  if (m_pThread)
    return;

  m_bStop = false;
  m_pThread = new CThread(this);
  m_pThread->Create();
  m_pThread->SetName("EventServer");
}

void CEventServer::StopServer()
{
  m_bStop = true;
  if (m_pThread)
  {
    m_pThread->WaitForThreadExit(2000);
    delete m_pThread;
  }
  m_pThread = NULL;    
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

  m_bRunning = true;

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

    // execute events
    ExecuteEvents();

    // refresh client list
    // RefreshClients();

    // broadcast
    // BroadcastBeacon();
  }

  m_bRunning = false;
  Cleanup();
}

void CEventServer::ProcessPacket(CAddress& addr, int pSize)
{
  // check packet validity
  CEventPacket* packet = new CEventPacket(pSize, m_pPacketBuffer);

  if (!packet->IsValid())
  {
    CLog::Log(LOGDEBUG, "ES: Received invalid packet");
    delete packet;
    return;
  }

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
  
  m_clients[addr.ULong()]->AddPacket(packet);

}

void CEventServer::ExecuteEvents()
{
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    iter->second->ExecuteEvents();
    iter++;
  }
}
