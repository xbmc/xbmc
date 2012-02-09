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

#include "HTSPConnection.h"
#include "../../../lib/platform/threads/mutex.h"
#include "../../../lib/platform/util/timeutils.h"
#include "../../../lib/platform/sockets/tcp.h"
#include "client.h"

extern "C" {
#include "cmyth/include/refmem/atomic.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

CHTSPConnection::CHTSPConnection() :
    m_socket(NULL),
    m_challenge(NULL),
    m_iChallengeLength(0),
    m_iProtocol(0),
    m_iPortnumber(g_iPortHTSP),
    m_iConnectTimeout(g_iConnectTimeout * 1000),
    m_strUsername(g_strUsername),
    m_strPassword(g_strPassword),
    m_strHostname(g_strHostname),
    m_bIsConnected(false),
    m_iQueueSize(1000)
{
}

CHTSPConnection::~CHTSPConnection()
{
  Close();
}

bool CHTSPConnection::Connect()
{
  if (m_bIsConnected)
    return true;

  uint64_t iNow = GetTimeMs();
  uint64_t iTarget = iNow + m_iConnectTimeout;

  XBMC->Log(LOG_DEBUG, "%s - connecting to '%s', port '%d'", __FUNCTION__, m_strHostname.c_str(), m_iPortnumber);

  if (!m_socket)
    m_socket = new CTcpConnection(m_strHostname, m_iPortnumber);
  while (!m_socket->IsOpen() && iNow < iTarget)
  {
    if (!m_socket->Open(iTarget - iNow))
      CCondition::Sleep(100);
    iNow = GetTimeMs();
  }

  if (!m_socket->IsOpen())
  {
    XBMC->Log(LOG_ERROR, "%s - failed to connect to the backend (%s)", __FUNCTION__, m_socket->GetError().c_str());
    return false;
  }

  m_bIsConnected = true;
  XBMC->Log(LOG_DEBUG, "%s - connected to '%s', port '%d'", __FUNCTION__, m_strHostname.c_str(), m_iPortnumber);

  if (!SendGreeting())
  {
    XBMC->Log(LOG_ERROR, "%s - failed to read greeting from the backend", __FUNCTION__);
    Close();
    return false;
  }

  if(m_iProtocol < 2)
  {
    XBMC->Log(LOG_ERROR, "%s - incompatible protocol version %d", __FUNCTION__, m_iProtocol);
    Close();
    return false;
  }

  if (!Auth())
  {
    XBMC->Log(LOG_ERROR, "%s - failed to authenticate", __FUNCTION__);
    Close();
    return false;
  }

  return true;
}

void CHTSPConnection::Close()
{
  if (!m_bIsConnected)
    return;
  m_bIsConnected = false;

  if(m_socket->IsOpen())
  {
    m_socket->Close();
    delete m_socket;
    m_socket = NULL;
  }

  if(m_challenge)
  {
    free(m_challenge);
    m_challenge     = NULL;
    m_iChallengeLength = 0;
  }
}

void CHTSPConnection::Abort(void)
{
  if (!m_bIsConnected)
    return;
  m_bIsConnected = false;

  m_socket->Shutdown();
}

htsmsg_t* CHTSPConnection::ReadMessage(int iInitialTimeout /* = 10000 */, int iDatapacketTimeout /* = 10000 */)
{
  void*    buf;
  uint32_t l;

  if(m_queue.size())
  {
    htsmsg_t* m = m_queue.front();
    m_queue.pop_front();
    return m;
  }

  {
    CLockObject lock(m_mutex);
    if (!IsConnected() || !m_socket->IsOpen())
    {
      XBMC->Log(LOG_ERROR, "%s - not connected", __FUNCTION__);
      return NULL;
    }

    if (m_socket->Read(&l, 4, iInitialTimeout) != 4)
    {
      if(m_socket->GetErrorNumber() != ETIMEDOUT && m_socket->GetErrorNumber() != 0)
        XBMC->Log(LOG_ERROR, "%s - Failed to read packet size (%s)", __FUNCTION__, m_socket->GetError().c_str());
      return NULL;
    }

    l = ntohl(l);
    if(l == 0)
      return NULL;

    buf = malloc(l);

    if(m_socket->Read(buf, l, iDatapacketTimeout) != (ssize_t)l)
    {
      XBMC->Log(LOG_ERROR, "%s - Failed to read packet (%s)", __FUNCTION__, m_socket->GetError().c_str());
      free(buf);
      Close();
      return NULL;
    }
  }

  return htsmsg_binary_deserialize(buf, l, buf); /* consumes 'buf' */
}

bool CHTSPConnection::TransmitMessage(htsmsg_t* m)
{
  void*  buf;
  size_t len;

  if(htsmsg_binary_serialize(m, &buf, &len, -1) < 0)
  {
    htsmsg_destroy(m);
    return false;
  }
  htsmsg_destroy(m);

  CLockObject lock(m_mutex);
  ssize_t iWriteResult = m_socket->Write(buf, len);
  if (iWriteResult != (ssize_t)len)
  {
    XBMC->Log(LOG_ERROR, "%s - Failed to write packet (%s)", __FUNCTION__, m_socket->GetError().c_str());
    free(buf);
    Close();
    return false;
  }
  free(buf);
  return true;
}

htsmsg_t* CHTSPConnection::ReadResult(htsmsg_t* m, bool sequence)
{
  uint32_t iSequence = 0;
  if(sequence)
  {
    iSequence = mvp_atomic_inc(&g_iPacketSequence);
    htsmsg_add_u32(m, "seq", iSequence);
  }

  if(!TransmitMessage(m))
    return NULL;

  std::deque<htsmsg_t*> queue;
  m_queue.swap(queue);

  while((m = ReadMessage()))
  {
    uint32_t seq;
    if(!sequence)
      break;
    if(!htsmsg_get_u32(m, "seq", &seq) && seq == iSequence)
      break;

    queue.push_back(m);
    if(queue.size() >= m_iQueueSize)
    {
      XBMC->Log(LOG_ERROR, "%s - maximum queue size (%u) reached", __FUNCTION__, m_iQueueSize);
      m_queue.swap(queue);
      return NULL;
    }
  }

  m_queue.swap(queue);

  const char* error;
  if(m && (error = htsmsg_get_str(m, "error")))
  {
    XBMC->Log(LOG_ERROR, "%s - error (%s)", __FUNCTION__, error);
    htsmsg_destroy(m);
    return NULL;
  }
  uint32_t noaccess;
  if(m && !htsmsg_get_u32(m, "noaccess", &noaccess) && noaccess)
  {

    XBMC->Log(LOG_ERROR, "%s - access denied (%d)", __FUNCTION__, noaccess);
    XBMC->QueueNotification(QUEUE_ERROR, "access denied (%d)", noaccess);
    htsmsg_destroy(m);
    return NULL;
  }

  return m;
}

bool CHTSPConnection::ReadSuccess(htsmsg_t* m, bool sequence, std::string action)
{
  if((m = ReadResult(m, sequence)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to %s", __FUNCTION__, action.c_str());
    return false;
  }
  htsmsg_destroy(m);
  return true;
}

bool CHTSPConnection::SendGreeting(void)
{
  htsmsg_t *m;
  const char *method, *server, *version;
  const void * chall = NULL;
  size_t chall_len = 0;
  int32_t proto = 0;

  /* send hello */
  m = htsmsg_create_map();
  htsmsg_add_str(m, "method", "hello");
  htsmsg_add_str(m, "clientname", "XBMC Media Center");
  htsmsg_add_u32(m, "htspversion", 1);

  /* read welcome */
  if((m = ReadResult(m)) == NULL)
    return false;

  method  = htsmsg_get_str(m, "method");
            htsmsg_get_s32(m, "htspversion", &proto);
  server  = htsmsg_get_str(m, "servername");
  version = htsmsg_get_str(m, "serverversion");
            htsmsg_get_bin(m, "challenge", &chall, &chall_len);

  m_strServerName = server;
  m_strVersion    = version;
  m_iProtocol     = proto;

  if(chall && chall_len)
  {
    m_challenge        = malloc(chall_len);
    m_iChallengeLength = chall_len;
    memcpy(m_challenge, chall, chall_len);
  }

  htsmsg_destroy(m);

  return true;
}

bool CHTSPConnection::Auth(void)
{
  if (m_strUsername.empty())
  {
    XBMC->Log(LOG_DEBUG, "CHTSPConnection - %s - no username set. not authenticating", __FUNCTION__);
    return true;
  }

  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"  , "authenticate");
  htsmsg_add_str(m, "username", m_strUsername.c_str());

  if(m_strPassword != "" && m_challenge)
  {
    XBMC->Log(LOG_DEBUG, "CHTSPConnection - %s - authenticating as user '%s' with a password", __FUNCTION__, m_strUsername.c_str());

    struct HTSSHA1* shactx = (struct HTSSHA1*) malloc(hts_sha1_size);
    uint8_t d[20];
    hts_sha1_init(shactx);
    hts_sha1_update(shactx, (const uint8_t *) m_strPassword.c_str(), m_strPassword.length());
    hts_sha1_update(shactx, (const uint8_t *) m_challenge, m_iChallengeLength);
    hts_sha1_final(shactx, d);
    htsmsg_add_bin(m, "digest", d, 20);
    free(shactx);
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "CHTSPConnection - %s - authenticating as user '%s' without a password", __FUNCTION__, m_strUsername.c_str());
  }

  return ReadSuccess(m, false, "get reply from authentication with server");
}
