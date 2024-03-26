/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventServer.h"

#include "EventClient.h"
#include "EventPacket.h"
#include "ServiceBroker.h"
#include "Socket.h"
#include "Zeroconf.h"
#include "application/Application.h"
#include "guilib/GUIAudioManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "interfaces/builtins/Builtins.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include <cassert>
#include <map>
#include <mutex>
#include <queue>

using namespace KODI;
using namespace EVENTSERVER;
using namespace EVENTPACKET;
using namespace EVENTCLIENT;
using namespace SOCKETS;
using namespace std::chrono_literals;

/************************************************************************/
/* CEventServer                                                         */
/************************************************************************/
std::unique_ptr<CEventServer> CEventServer::m_pInstance;

CEventServer::CEventServer() : CThread("EventServer")
{
  m_bStop         = false;
  m_bRefreshSettings = false;

  // default timeout in ms for receiving a single packet
  m_iListenTimeout = 1000;
}

void CEventServer::RemoveInstance()
{
  m_pInstance.reset();
}

CEventServer* CEventServer::GetInstance()
{
  if (!m_pInstance)
    m_pInstance = std::make_unique<CEventServer>();

  return m_pInstance.get();
}

void CEventServer::StartServer()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_bRunning)
    return;

  // set default port
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_iPort = settings->GetInt(CSettings::SETTING_SERVICES_ESPORT);
  assert(m_iPort <= 65535 && m_iPort >= 1);

  // max clients
  m_iMaxClients = settings->GetInt(CSettings::SETTING_SERVICES_ESMAXCLIENTS);
  if (m_iMaxClients < 0)
  {
    CLog::Log(LOGERROR, "ES: Invalid maximum number of clients specified {}", m_iMaxClients);
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
    m_pSocket->Close();

  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_clients.clear();
}

int CEventServer::GetNumberOfClients()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_clients.size();
}

void CEventServer::Process()
{
  while(!m_bStop)
  {
    Run();
    if (!m_bStop)
      CThread::Sleep(1000ms);
  }
}

void CEventServer::Run()
{
  CSocketListener listener;
  int packetSize = 0;

  CLog::Log(LOGINFO, "ES: Starting UDP Event server on port {}", m_iPort);

  Cleanup();

  // create socket and initialize buffer
  m_pSocket = CSocketFactory::CreateUDPSocket();
  if (!m_pSocket)
  {
    CLog::Log(LOGERROR, "ES: Could not create socket, aborting!");
    return;
  }

  m_pPacketBuffer.resize(PACKET_SIZE);

  // bind to IP and start listening on port
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  int port_range = settings->GetInt(CSettings::SETTING_SERVICES_ESPORTRANGE);
  if (port_range < 1 || port_range > 100)
  {
    CLog::Log(LOGERROR, "ES: Invalid port range specified {}, defaulting to 10", port_range);
    port_range = 10;
  }
  if (!m_pSocket->Bind(!settings->GetBool(CSettings::SETTING_SERVICES_ESALLINTERFACES), m_iPort, port_range))
  {
    CLog::Log(LOGERROR, "ES: Could not listen on port {}", m_iPort);
    return;
  }

  // publish service
  std::vector<std::pair<std::string, std::string>> txt;
  CZeroconf::GetInstance()->PublishService("servers.eventserver", "_xbmc-events._udp",
                                           CSysInfo::GetDeviceName() + " eventserver", m_iPort,
                                           txt);

  // add our socket to the 'select' listener
  listener.AddSocket(m_pSocket.get());

  m_bRunning = true;

  while (!m_bStop)
  {
    try
    {
      // start listening until we timeout
      if (listener.Listen(m_iListenTimeout))
      {
        CAddress addr;
        if ((packetSize = m_pSocket->Read(addr, PACKET_SIZE, m_pPacketBuffer.data())) > -1)
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

  CLog::Log(LOGINFO, "ES: UDP Event server stopped");
  m_bRunning = false;
  Cleanup();
}

void CEventServer::ProcessPacket(CAddress& addr, int pSize)
{
  // check packet validity
  std::unique_ptr<CEventPacket> packet =
      std::make_unique<CEventPacket>(pSize, m_pPacketBuffer.data());
  if (!packet)
  {
    CLog::Log(LOGERROR, "ES: Out of memory, cannot accept packet");
    return;
  }

  unsigned int clientToken;

  if (!packet->IsValid())
  {
    CLog::Log(LOGDEBUG, "ES: Received invalid packet");
    return;
  }

  clientToken = packet->ClientToken();
  if (!clientToken)
    clientToken = addr.ULong(); // use IP if packet doesn't have a token

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // first check if we have a client for this address
  auto iter = m_clients.find(clientToken);

  if ( iter == m_clients.end() )
  {
    if ( m_clients.size() >= (unsigned int)m_iMaxClients)
    {
      CLog::Log(LOGWARNING, "ES: Cannot accept any more clients, maximum client count reached");
      return;
    }

    // new client
    auto client = std::make_unique<CEventClient>(addr);
    if (!client)
    {
      CLog::Log(LOGERROR, "ES: Out of memory, cannot accept new client connection");
      return;
    }

    m_clients[clientToken] = std::move(client);
  }
  m_clients[clientToken]->AddPacket(std::move(packet));
}

void CEventServer::RefreshClients()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto iter = m_clients.begin();

  while ( iter != m_clients.end() )
  {
    if (! (iter->second->Alive()))
    {
      CLog::Log(LOGINFO, "ES: Client {} from {} timed out", iter->second->Name(),
                iter->second->Address().Address());
      m_clients.erase(iter);
      iter = m_clients.begin();
    }
    else
    {
      if (m_bRefreshSettings)
      {
        iter->second->RefreshSettings();
      }
      ++iter;
    }
  }
  m_bRefreshSettings = false;
}

void CEventServer::ProcessEvents()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    iter->second->ProcessEvents();
    ++iter;
  }
}

bool CEventServer::ExecuteNextAction()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CEventAction actionEvent;
  auto iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    if (iter->second->GetNextAction(actionEvent))
    {
      // Leave critical section before processing action
      lock.unlock();
      switch(actionEvent.actionType)
      {
      case AT_EXEC_BUILTIN:
        CBuiltins::GetInstance().Execute(actionEvent.actionName);
        break;

      case AT_BUTTON:
        {
          unsigned int actionID;
          ACTION::CActionTranslator::TranslateString(actionEvent.actionName, actionID);
          CAction action(actionID, 1.0f, 0.0f, actionEvent.actionName);
          CGUIComponent* gui = CServiceBroker::GetGUI();
          if (gui)
            gui->GetAudioManager().PlayActionSound(action);

          g_application.OnAction(action);
        }
        break;
      }
      return true;
    }
    ++iter;
  }

  return false;
}

unsigned int CEventServer::GetButtonCode(std::string& strMapName, bool& isAxis, float& fAmount, bool &isJoystick)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto iter = m_clients.begin();
  unsigned int bcode = 0;

  while (iter != m_clients.end())
  {
    bcode = iter->second->GetButtonCode(strMapName, isAxis, fAmount, isJoystick);
    if (bcode)
      return bcode;
    ++iter;
  }
  return bcode;
}

bool CEventServer::GetMousePos(float &x, float &y)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    if (iter->second->GetMousePos(x, y))
      return true;
    ++iter;
  }
  return false;
}
