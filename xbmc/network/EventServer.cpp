/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#ifdef HAS_EVENT_SERVER

#include "EventServer.h"
#include "EventPacket.h"
#include "EventClient.h"
#include "Socket.h"
#include "threads/CriticalSection.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "interfaces/Builtins.h"
#include "input/ButtonTranslator.h"
#include "threads/SingleLock.h"
#include "Zeroconf.h"
#include "guilib/GUIAudioManager.h"
#include <map>
#include <queue>

using namespace EVENTSERVER;
using namespace EVENTPACKET;
using namespace EVENTCLIENT;
using namespace SOCKETS;
using namespace std;

/************************************************************************/
/* CEventServer                                                         */
/************************************************************************/
CEventServer* CEventServer::m_pInstance = NULL;
CEventServer::CEventServer() : CThread("CEventServer")
{
  m_pSocket       = NULL;
  m_pPacketBuffer = NULL;
  m_bStop         = false;
  m_bRunning      = false;
  m_bRefreshSettings = false;

  // default timeout in ms for receiving a single packet
  m_iListenTimeout = 1000;
}

void CEventServer::RemoveInstance()
{
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance=NULL;
  }
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
  CSingleLock lock(m_critSection);
  if(m_bRunning)
    return;

  // set default port
  string port = (const char*)g_guiSettings.GetString("services.esport");
  assert(port.length());
  m_iPort = atoi(port.c_str());
  assert(m_iPort <= 65535 && m_iPort >= 1);

  // max clients
  m_iMaxClients = g_guiSettings.GetInt("services.esmaxclients");
  if (m_iMaxClients < 0)
  {
    CLog::Log(LOGERROR, "ES: Invalid maximum number of clients specified %d", m_iMaxClients);
    m_iMaxClients = 20;
  }

  CThread::Create();
}

void CEventServer::StopServer(bool bWait)
{
  CZeroconf::GetInstance()->RemoveService("services.eventserver");
  StopThread(bWait);
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
  CSingleLock lock(m_critSection);

  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();
  while (iter != m_clients.end())
  {
    if (iter->second)
    {
      delete iter->second;
    }
    m_clients.erase(iter);
    iter =  m_clients.begin();
  }
}

int CEventServer::GetNumberOfClients()
{
  CSingleLock lock(m_critSection);
  return m_clients.size();
}

void CEventServer::Process()
{
  while(!m_bStop)
  {
    Run();
    if (!m_bStop)
      Sleep(1000);
  }
}

void CEventServer::Run()
{
  CAddress any_addr;
  CSocketListener listener;
  int packetSize = 0;
  std::map<std::string, std::string> txt;  

  CLog::Log(LOGNOTICE, "ES: Starting UDP Event server on %s:%d", any_addr.Address(), m_iPort);

  Cleanup();

  // create socket and initialize buffer
  m_pSocket = CSocketFactory::CreateUDPSocket();
  if (!m_pSocket)
  {
    CLog::Log(LOGERROR, "ES: Could not create socket, aborting!");
    return;
  }
  m_pPacketBuffer = (unsigned char *)malloc(PACKET_SIZE);

  if (!m_pPacketBuffer)
  {
    CLog::Log(LOGERROR, "ES: Out of memory, could not allocate packet buffer");
    return;
  }

  // bind to IP and start listening on port
  int port_range = g_guiSettings.GetInt("services.esportrange");
  if (port_range < 1 || port_range > 100)
  {
    CLog::Log(LOGERROR, "ES: Invalid port range specified %d, defaulting to 10", port_range);
    port_range = 10;
  }
  if (!m_pSocket->Bind(any_addr, m_iPort, port_range))
  {
    CLog::Log(LOGERROR, "ES: Could not listen on port %d", m_iPort);
    return;
  }

  // publish service
  CZeroconf::GetInstance()->PublishService("servers.eventserver",
                               "_xbmc-events._udp",
                               g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME),
                               m_iPort,
                               txt);

  // add our socket to the 'select' listener
  listener.AddSocket(m_pSocket);

  m_bRunning = true;

  while (!m_bStop)
  {
    try
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
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "ES: Exception caught while listening for socket");
      break;
    }

    // process events and queue the necessary actions and button codes
    ProcessEvents();

    // refresh client list
    RefreshClients();

    // broadcast
    // BroadcastBeacon();
  }

  CLog::Log(LOGNOTICE, "ES: UDP Event server stopped");
  m_bRunning = false;
  Cleanup();
}

void CEventServer::ProcessPacket(CAddress& addr, int pSize)
{
  // check packet validity
  CEventPacket* packet = new CEventPacket(pSize, m_pPacketBuffer);
  if(packet == NULL)
  {
    CLog::Log(LOGERROR, "ES: Out of memory, cannot accept packet");
    return;
  }

  unsigned int clientToken;

  if (!packet->IsValid())
  {
    CLog::Log(LOGDEBUG, "ES: Received invalid packet");
    delete packet;
    return;
  }

  clientToken = packet->ClientToken();
  if (!clientToken)
    clientToken = addr.ULong(); // use IP if packet doesn't have a token

  CSingleLock lock(m_critSection);

  // first check if we have a client for this address
  map<unsigned long, CEventClient*>::iterator iter = m_clients.find(clientToken);

  if ( iter == m_clients.end() )
  {
    if ( m_clients.size() >= (unsigned int)m_iMaxClients)
    {
      CLog::Log(LOGWARNING, "ES: Cannot accept any more clients, maximum client count reached");
      delete packet;
      return;
    }

    // new client
    CEventClient* client = new CEventClient ( addr );
    if (client==NULL)
    {
      CLog::Log(LOGERROR, "ES: Out of memory, cannot accept new client connection");
      delete packet;
      return;
    }

    m_clients[clientToken] = client;
  }
  m_clients[clientToken]->AddPacket(packet);
}

void CEventServer::RefreshClients()
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while ( iter != m_clients.end() )
  {
    if (! (iter->second->Alive()))
    {
      CLog::Log(LOGNOTICE, "ES: Client %s from %s timed out", iter->second->Name().c_str(),
                iter->second->Address().Address());
      delete iter->second;
      m_clients.erase(iter);
      iter = m_clients.begin();
    }
    else
    {
      if (m_bRefreshSettings)
      {
        iter->second->RefreshSettings();
      }
      iter++;
    }
  }
  m_bRefreshSettings = false;
}

void CEventServer::ProcessEvents()
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    iter->second->ProcessEvents();
    iter++;
  }
}

bool CEventServer::ExecuteNextAction()
{
  CSingleLock lock(m_critSection);

  CEventAction actionEvent;
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    if (iter->second->GetNextAction(actionEvent))
    {
      // Leave critical section before processing action
      lock.Leave();
      switch(actionEvent.actionType)
      {
      case AT_EXEC_BUILTIN:
        CBuiltins::Execute(actionEvent.actionName);
        break;

      case AT_BUTTON:
        {
          int actionID;
          CButtonTranslator::TranslateActionString(actionEvent.actionName.c_str(), actionID);
          CAction action(actionID, 1.0f, 0.0f, actionEvent.actionName);
          g_audioManager.PlayActionSound(action);
          g_application.OnAction(action);
        }
        break;
      }
      return true;
    }
    iter++;
  }

  return false;
}

unsigned int CEventServer::GetButtonCode(std::string& strMapName, bool& isAxis, float& fAmount)
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();
  unsigned int bcode = 0;

  while (iter != m_clients.end())
  {
    bcode = iter->second->GetButtonCode(strMapName, isAxis, fAmount);
    if (bcode)
      return bcode;
    iter++;
  }
  return bcode;
}

bool CEventServer::GetMousePos(float &x, float &y)
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    if (iter->second->GetMousePos(x, y))
      return true;
    iter++;
  }
  return false;
}

#endif // HAS_EVENT_SERVER
