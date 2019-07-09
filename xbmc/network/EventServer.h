/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "EventClient.h"
#include "Socket.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "threads/Thread.h"

#include <atomic>
#include <map>
#include <queue>
#include <vector>

namespace EVENTSERVER
{

  /**********************************************************************/
  /* UDP Event Server Class                                             */
  /**********************************************************************/
  class CEventServer : private CThread
  {
  public:
    static void RemoveInstance();
    static CEventServer* GetInstance();
    ~CEventServer() override = default;

    // IRunnable entry point for thread
    void  Process() override;

    bool Running()
    {
      return m_bRunning;
    }

    void RefreshSettings()
    {
      CSingleLock lock(m_critSection);
      m_bRefreshSettings = true;
    }

    // start / stop server
    void StartServer();
    void StopServer(bool bWait);

    // get events
    unsigned int GetButtonCode(std::string& strMapName, bool& isAxis, float& amount, bool &isJoystick);
    bool ExecuteNextAction();
    bool GetMousePos(float &x, float &y);
    int GetNumberOfClients();

  protected:
    CEventServer();
    void Cleanup();
    void Run();
    void ProcessPacket(SOCKETS::CAddress& addr, int packetSize);
    void ProcessEvents();
    void RefreshClients();

    std::map<unsigned long, EVENTCLIENT::CEventClient*>  m_clients;
    static CEventServer* m_pInstance;
    SOCKETS::CUDPSocket* m_pSocket;
    int              m_iPort;
    int              m_iListenTimeout;
    int              m_iMaxClients;
    unsigned char*   m_pPacketBuffer;
    std::atomic<bool>  m_bRunning;
    CCriticalSection m_critSection;
    bool             m_bRefreshSettings;
  };

}

