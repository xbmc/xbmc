#pragma once
/*
 * Many concepts and protocol specification in this code are taken from
 * the Boxee project. http://www.boxee.tv
 *
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your option)
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
#ifdef HAS_AIRPLAY

#include <map>
#include <sys/socket.h>
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "utils/HttpParser.h"
#include "interfaces/IAnnouncer.h"

class DllLibPlist;

#define AIRPLAY_SERVER_VERSION_STR "101.28"

class CAirPlayServer : public CThread, public ANNOUNCEMENT::IAnnouncer
{
public:
  // IAnnouncer IF
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  //AirPlayServer impl.
  static bool StartServer(int port, bool nonlocal);
  static void StopServer(bool bWait);
  static bool IsRunning();
  static bool SetCredentials(bool usePassword, const std::string& password);
  static bool IsPlaying(){ return m_isPlaying > 0;}
  static void backupVolume();
  static void restoreVolume();
  static int m_isPlaying;

protected:
  void Process();

private:
  CAirPlayServer(int port, bool nonlocal);
  ~CAirPlayServer();
  bool SetInternalCredentials(bool usePassword, const std::string& password);
  bool Initialize();
  void Deinitialize();
  void AnnounceToClients(int state);

  class CTCPClient
  {
  public:
    CTCPClient();
    ~CTCPClient();
    //Copying a CCriticalSection is not allowed, so copy everything but that
    //when adding a member variable, make sure to copy it in CTCPClient::Copy
    CTCPClient(const CTCPClient& client);
    CTCPClient& operator=(const CTCPClient& client);
    void PushBuffer(CAirPlayServer *host, const char *buffer,
                    int length, std::string &sessionId,
                    std::map<std::string, int> &reverseSockets);
    void ComposeReverseEvent(std::string& reverseHeader, std::string& reverseBody, int state);

    void Disconnect();

    int m_socket;
    struct sockaddr_storage m_cliaddr;
    socklen_t m_addrlen;
    CCriticalSection m_critSection;
    int  m_sessionCounter;
    std::string m_sessionId;

  private:
    int ProcessRequest( std::string& responseHeader,
                        std::string& response);

    void ComposeAuthRequestAnswer(std::string& responseHeader, std::string& responseBody);
    bool checkAuthorization(const std::string& authStr, const std::string& method, const std::string& uri);
    void Copy(const CTCPClient& client);

    HttpParser* m_httpParser;
    DllLibPlist *m_pLibPlist;//the lib
    bool m_bAuthenticated;
    int  m_lastEvent;
    std::string m_authNonce;
  };

  CCriticalSection m_connectionLock;
  std::vector<CTCPClient> m_connections;
  std::map<std::string, int> m_reverseSockets;
  int m_ServerSocket;
  int m_port;
  bool m_nonlocal;
  bool m_usePassword;
  std::string m_password;
  int m_origVolume;

  static CCriticalSection ServerInstanceLock;
  static CAirPlayServer *ServerInstance;
};

#endif
