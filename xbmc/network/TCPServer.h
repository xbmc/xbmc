#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>
#include <sys/socket.h>
#include "interfaces/IAnnouncer.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "interfaces/json-rpc/JSONUtils.h"

namespace JSONRPC
{
  class CTCPServer : public ITransportLayer, public ANNOUNCEMENT::IAnnouncer, public CThread, protected CJSONUtils
  {
  public:
    static bool StartServer(int port, bool nonlocal);
    static void StopServer(bool bWait);

    virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol);
    virtual bool Download(const char *path, CVariant &result);
    virtual int GetCapabilities();

    virtual void Announce(ANNOUNCEMENT::EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
  protected:
    void Process();
  private:
    CTCPServer(int port, bool nonlocal);
    bool Initialize();
    bool InitializeBlue();
    bool InitializeTCP();
    void Deinitialize();

    class CTCPClient : public IClient
    {
    public:
      CTCPClient();
      //Copying a CCriticalSection is not allowed, so copy everything but that
      //when adding a member variable, make sure to copy it in CTCPClient::Copy
      CTCPClient(const CTCPClient& client);
      CTCPClient& operator=(const CTCPClient& client);
      virtual int  GetPermissionFlags();
      virtual int  GetAnnouncementFlags();
      virtual bool SetAnnouncementFlags(int flags);
      void PushBuffer(CTCPServer *host, const char *buffer, int length);
      void Disconnect();

      SOCKET           m_socket;
      sockaddr_storage m_cliaddr;
      socklen_t        m_addrlen;
      CCriticalSection m_critSection;

    private:
      void Copy(const CTCPClient& client);
      int m_announcementflags;
      int m_beginBrackets, m_endBrackets;
      char m_beginChar, m_endChar;
      std::string m_buffer;
    };

    std::vector<CTCPClient> m_connections;
    std::vector<SOCKET> m_servers;
    int m_port;
    bool m_nonlocal;
    void* m_sdpd;

    static CTCPServer *ServerInstance;
  };
}
