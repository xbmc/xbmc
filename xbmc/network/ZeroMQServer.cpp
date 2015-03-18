/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "ZeroMQServer.h"

#include "settings/AdvancedSettings.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "Network.h"

using namespace JSONRPC;
using namespace ANNOUNCEMENT;

CZeroMQServer *CZeroMQServer::ServerInstance = NULL;

bool CZeroMQServer::StartServer(int port)
{
  StopServer(true);

  ServerInstance = new CZeroMQServer(port);
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create();
    return true;
  }
  else
    return false;
}

void CZeroMQServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

bool CZeroMQServer::IsRunning()
{
  if (ServerInstance == NULL)
    return false;

  return ((CThread*)ServerInstance)->IsRunning();
}

CZeroMQServer::CZeroMQServer(int port) : CThread("ZeroMQServer"), m_port(port), m_context(1), m_repSocket(m_context, ZMQ_REP), m_pubSocket(m_context, ZMQ_PUB)
{
}

void CZeroMQServer::Process()
{
  m_bStop = false;

  while (!m_bStop)
  {
    zmq::message_t request_packet;

    m_repSocket.recv (&request_packet);
    std::string request = std::string((const char *)request_packet.data(), request_packet.size());

    CZeroMQClient client;
    std::string response = CJSONRPC::MethodCall(request, this, &client);

    zmq::message_t response_packet (response.size());
    memcpy ((void *) response_packet.data (), response.c_str(), response.size());
    m_repSocket.send (response_packet);
  }

  Deinitialize();
}

bool CZeroMQServer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
{
  return false;
}

bool CZeroMQServer::Download(const char *path, CVariant &result)
{
  return false;
}

int CZeroMQServer::GetCapabilities()
{
  return Response | Announcing;
}

void CZeroMQServer::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  std::string out = IJSONRPCAnnouncer::AnnouncementToJSONRPC(flag, sender, message, data, g_advancedSettings.m_jsonOutputCompact);

  zmq::message_t notification_packet (out.size());
  memcpy ((void *) notification_packet.data (), out.c_str(), out.size());
  m_pubSocket.send (notification_packet);
}

bool CZeroMQServer::Initialize()
{
  Deinitialize();

  m_repSocket.bind("tcp://*:5555");
  m_pubSocket.bind("tcp://*:5556");

  CAnnouncementManager::Get().AddAnnouncer(this);
  CLog::Log(LOGINFO, "ZeroMQ JSONRPC Server: Successfully initialized");
  return true;
}

void CZeroMQServer::Deinitialize()
{
  CAnnouncementManager::Get().RemoveAnnouncer(this);
}

CZeroMQServer::CZeroMQClient::CZeroMQClient()
{
  m_new = true;
  m_announcementflags = ANNOUNCE_ALL;
}

CZeroMQServer::CZeroMQClient::CZeroMQClient(const CZeroMQClient& client)
{
  Copy(client);
}

CZeroMQServer::CZeroMQClient& CZeroMQServer::CZeroMQClient::operator=(const CZeroMQClient& client)
{
  Copy(client);
  return *this;
}

int CZeroMQServer::CZeroMQClient::GetPermissionFlags()
{
  return OPERATION_PERMISSION_ALL;
}

int CZeroMQServer::CZeroMQClient::GetAnnouncementFlags()
{
  return m_announcementflags;
}

bool CZeroMQServer::CZeroMQClient::SetAnnouncementFlags(int flags)
{
  m_announcementflags = flags;
  return true;
}

void CZeroMQServer::CZeroMQClient::Copy(const CZeroMQClient& client)
{
  m_new               = client.m_new;
  m_announcementflags = client.m_announcementflags;
}
