#pragma once
/*
 * Many concepts and protocol specification in this code are taken from
 * the Boxee project. http://www.boxee.tv
 *
 *      Copyright (C) 2011-2012 Team XBMC
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
#ifdef HAS_AIRPLAY

#include <map>
#include <sys/socket.h>
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "utils/HttpParser.h"
#include "utils/StdString.h"

class DllLibPlist;

#define AIRPLAY_SERVER_VERSION_STR "101.28"

class CAirPlayServer : public CThread
{
public:
  static bool StartServer(int port, bool nonlocal);
  static void StopServer(bool bWait);
  static bool SetCredentials(bool usePassword, const CStdString& password);
  static bool IsPlaying(){ return m_isPlaying > 0;}
  static int m_isPlaying;

protected:
  void Process();

private:
  CAirPlayServer(int port, bool nonlocal);
  bool SetInternalCredentials(bool usePassword, const CStdString& password);
  bool Initialize();
  void Deinitialize();

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
                    int length, CStdString &sessionId,
                    std::map<CStdString, int> &reverseSockets);

    void Disconnect();

    int m_socket;
    struct sockaddr m_cliaddr;
    socklen_t m_addrlen;
    CCriticalSection m_critSection;

  private:
    int ProcessRequest( CStdString& responseHeader,
                        CStdString& response,
                        CStdString& reverseHeader,
                        CStdString& reverseBody,
                        CStdString& sessionId);

    void ComposeReverseEvent(CStdString& reverseHeader, CStdString& reverseBody, CStdString sessionId, int state);
    void ComposeAuthRequestAnswer(CStdString& responseHeader, CStdString& responseBody);
    bool checkAuthorization(const CStdString& authStr, const CStdString& method, const CStdString& uri);
    void Copy(const CTCPClient& client);

    HttpParser* m_httpParser;
    DllLibPlist *m_pLibPlist;//the lib
    bool m_bAuthenticated;
    int  m_lastEvent;
    CStdString m_authNonce;
  };

  std::vector<CTCPClient> m_connections;
  std::map<CStdString, int> m_reverseSockets;
  int m_ServerSocket;
  int m_port;
  bool m_nonlocal;
  bool m_usePassword;
  CStdString m_password;

  static CAirPlayServer *ServerInstance;
};

#endif
