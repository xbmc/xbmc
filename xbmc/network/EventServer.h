#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "threads/Thread.h"
#include "Socket.h"
#include "EventClient.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"

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

