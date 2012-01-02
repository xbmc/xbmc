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
#include "client.h"

extern "C" {
#include "libTcpSocket/os-dependent_socket.h"
#include "cmyth/include/refmem/atomic.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

using namespace std;
using namespace ADDON;

CHTSPConnection::CHTSPConnection() :
    m_fd(INVALID_SOCKET),
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

  cTimeMs RetryTimeout;
  char errbuf[1024];
  int  errlen = sizeof(errbuf);

  XBMC->Log(LOG_DEBUG, "%s - connecting to '%s', port '%d'", __FUNCTION__, m_strHostname.c_str(), m_iPortnumber);

  m_fd = INVALID_SOCKET;
  while (m_fd == INVALID_SOCKET && RetryTimeout.Elapsed() < (uint)m_iConnectTimeout * 1000)
  {
    m_fd = tcp_connect(m_strHostname.c_str(), m_iPortnumber, errbuf, errlen,
        m_iConnectTimeout * 1000 - RetryTimeout.Elapsed());
    cCondWait::SleepMs(100);
  }

  if(m_fd == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "%s - failed to connect to the backend (%s)", __FUNCTION__, errbuf);
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

  if(m_fd != INVALID_SOCKET)
  {
    tcp_close(m_fd);
    m_fd = INVALID_SOCKET;
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

  tcp_shutdown(m_fd);
}

htsmsg_t* CHTSPConnection::ReadMessage(int timeout)
{
  void*    buf;
  uint32_t l;
  int      x;

  if(m_queue.size())
  {
    htsmsg_t* m = m_queue.front();
    m_queue.pop_front();
    return m;
  }

  if (!IsConnected() || m_fd == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "%s - not connected", __FUNCTION__);
    return NULL;
  }

  x = tcp_read_timeout(m_fd, &l, 4, timeout);
  if(x == ETIMEDOUT)
    return htsmsg_create_map();

  if(x)
  {
    XBMC->Log(LOG_ERROR, "%s - Failed to read packet size (%d)", __FUNCTION__, x);
    Close();
    return NULL;
  }

  l   = ntohl(l);
  if(l == 0)
    return htsmsg_create_map();

  buf = malloc(l);

  x = tcp_read(m_fd, buf, l);
  if(x)
  {
    XBMC->Log(LOG_ERROR, "%s - Failed to read packet (%d)", __FUNCTION__, x);
    free(buf);
    Close();
    return NULL;
  }

  return htsmsg_binary_deserialize(buf, l, buf); /* consumes 'buf' */
}

bool CHTSPConnection::SendMessage(htsmsg_t* m)
{
  void*  buf;
  size_t len;

  if(htsmsg_binary_serialize(m, &buf, &len, -1) < 0)
  {
    htsmsg_destroy(m);
    return false;
  }
  htsmsg_destroy(m);

  if(tcp_send(m_fd, (char*)buf, len, 0) < 0)
  {
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

  if(!SendMessage(m))
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

bool CHTSPConnection::ParseEvent(htsmsg_t* msg, uint32_t id, SEvent &event)
{
  uint32_t start, stop, next, chan_id, content;
  const char *title, *desc, *ext_desc;
  if(         htsmsg_get_u32(msg, "start", &start)
  ||          htsmsg_get_u32(msg, "stop" , &stop)
  || (title = htsmsg_get_str(msg, "title")) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - malformed event", __FUNCTION__);
    htsmsg_print(msg);
    htsmsg_destroy(msg);
    return false;
  }
  event.Clear();
  event.id    = id;
  event.start = start;
  event.stop  = stop;
  event.title = title;

  desc     = htsmsg_get_str(msg, "description");
  ext_desc = htsmsg_get_str(msg, "ext_text");

  if (desc && ext_desc)
  {
    string strBuf = desc;
    strBuf.append(ext_desc);
    event.descs = strBuf;
  }
  else if (desc)
    event.descs = desc;
  else if (ext_desc)
    event.descs = ext_desc;
  else
    event.descs = "";

  if(htsmsg_get_u32(msg, "nextEventId", &next))
    event.next = 0;
  else
    event.next = next;
  if(htsmsg_get_u32(msg, "channelId", &chan_id))
    event.chan_id = -1;
  else
    event.chan_id = chan_id;
  if(htsmsg_get_u32(msg, "contentType", &content))
    event.content = -1;
  else
    event.content = content;

  XBMC->Log(LOG_DEBUG, "%s - id:%u, chan_id:%u, title:'%s', genre_type:%u, genre_sub_type:%u, desc:'%s', start:%u, stop:%u, next:%u"
                    , __FUNCTION__
                    , event.id
                    , event.chan_id
                    , event.title.c_str()
                    , event.content & 0x0F
                    , event.content & 0xF0
                    , event.descs.c_str()
                    , event.start
                    , event.stop
                    , event.next);

  return true;
}

void CHTSPConnection::ParseChannelUpdate(htsmsg_t* msg, SChannels &channels)
{
  bool bChanged(false);
  uint32_t iChannelId, iEventId = 0, iChannelNumber = 0, iCaid = 0;
  const char *strName, *strIconPath;
  if(htsmsg_get_u32(msg, "channelId", &iChannelId))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }

  SChannel &channel = channels[iChannelId];
  channel.id = iChannelId;

  if(htsmsg_get_u32(msg, "eventId", &iEventId) == 0)
    channel.event = iEventId;

  if((strName = htsmsg_get_str(msg, "channelName")))
  {
    bChanged = (channel.name != strName);
    channel.name = strName;
  }

  if((strIconPath = htsmsg_get_str(msg, "channelIcon")))
  {
    bChanged = (channel.icon != strIconPath);
    channel.icon = strIconPath;
  }

  if(htsmsg_get_u32(msg, "channelNumber", &iChannelNumber) == 0)
  {
    int iNewChannelNumber = (iChannelNumber == 0) ? iChannelId + 1000 : iChannelNumber;
    bChanged = (channel.num != iNewChannelNumber);
    channel.num = iNewChannelNumber;
  }

  htsmsg_t *tags;

  if((tags = htsmsg_get_list(msg, "tags")))
  {
    bChanged = true;
    channel.tags.clear();

    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, tags)
    {
      if(f->hmf_type != HMF_S64)
        continue;
      channel.tags.push_back((int)f->hmf_s64);
    }
  }

  htsmsg_t *services;
  
  if((services = htsmsg_get_list(msg, "services")))
  {
    bChanged = true;
    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, services)
    {
      if(f->hmf_type != HMF_MAP)
        continue;

      htsmsg_t *service = &f->hmf_msg;
      const char *service_type = htsmsg_get_str(service, "type");
      if(service_type != NULL)
      {
        channel.radio = !strcmp(service_type, "Radio");
      }

      if(!htsmsg_get_u32(service, "caid", &iCaid))
        channel.caid = (int) iCaid;
    }
  }
  
  XBMC->Log(LOG_DEBUG, "%s - id:%u, name:'%s', icon:'%s', event:%u",
      __FUNCTION__, iChannelId, strName ? strName : "(null)", strIconPath ? strIconPath : "(null)", iEventId);

  if (bChanged)
    PVR->TriggerChannelUpdate();
}

void CHTSPConnection::ParseChannelRemove(htsmsg_t* msg, SChannels &channels)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "channelId", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }
  XBMC->Log(LOG_DEBUG, "%s - id:%u", __FUNCTION__, id);

  channels.erase(id);

  PVR->TriggerChannelUpdate();
}

void CHTSPConnection::ParseTagUpdate(htsmsg_t* msg, STags &tags)
{
  uint32_t id;
  const char *name, *icon;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }
  STag &tag = tags[id];
  tag.id = id;

  if((icon = htsmsg_get_str(msg, "tagIcon")))
    tag.icon  = icon;

  if((name = htsmsg_get_str(msg, "tagName")))
    tag.name  = name;

  htsmsg_t *channels;

  if((channels = htsmsg_get_list(msg, "members")))
  {
    tag.channels.clear();

    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, channels)
    {
      if(f->hmf_type != HMF_S64)
        continue;
      tag.channels.push_back((int)f->hmf_s64);
    }
  }

  XBMC->Log(LOG_DEBUG, "%s - id:%u, name:'%s', icon:'%s'"
      , __FUNCTION__, id, name ? name : "(null)", icon ? icon : "(null)");

  PVR->TriggerChannelGroupsUpdate();
}

void CHTSPConnection::ParseTagRemove(htsmsg_t* msg, STags &tags)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }
  XBMC->Log(LOG_DEBUG, "%s - id:%u", __FUNCTION__, id);

  tags.erase(id);

  PVR->TriggerChannelGroupsUpdate();
}

bool CHTSPConnection::ParseSignalStatus (htsmsg_t* msg, SQuality &quality)
{
  if(htsmsg_get_u32(msg, "feSNR", &quality.fe_snr))
    quality.fe_snr = -2;

  if(htsmsg_get_u32(msg, "feSignal", &quality.fe_signal))
    quality.fe_signal = -2;

  if(htsmsg_get_u32(msg, "feBER", &quality.fe_ber))
    quality.fe_ber = -2;

  if(htsmsg_get_u32(msg, "feUNC", &quality.fe_unc))
    quality.fe_unc = -2;

  const char* status;
  if((status = htsmsg_get_str(msg, "feStatus")))
    quality.fe_status = status;
  else
    quality.fe_status = "(unknown)";

//  XBMC->Log(LOG_DEBUG, "%s - updated signal status: snr=%d, signal=%d, ber=%d, unc=%d, status=%s"
//      , __FUNCTION__, quality.fe_snr, quality.fe_signal, quality.fe_ber
//      , quality.fe_unc, quality.fe_status.c_str());

  return true;
}

bool CHTSPConnection::ParseSourceInfo (htsmsg_t* msg, SSourceInfo &si)
{
  htsmsg_t       *sourceinfo;
  if((sourceinfo = htsmsg_get_map(msg, "sourceinfo")) == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message", __FUNCTION__);
    return false;
  }

  const char* str;
  if((str = htsmsg_get_str(sourceinfo, "adapter")) == NULL)
    si.si_adapter = "";
  else
    si.si_adapter = str;

  if((str = htsmsg_get_str(sourceinfo, "mux")) == NULL)
    si.si_mux = "";
  else
    si.si_mux = str;

  if((str = htsmsg_get_str(sourceinfo, "network")) == NULL)
    si.si_network = "";
  else
    si.si_network = str;

  if((str = htsmsg_get_str(sourceinfo, "provider")) == NULL)
    si.si_provider = "";
  else
    si.si_provider = str;

  if((str = htsmsg_get_str(sourceinfo, "service")) == NULL)
    si.si_service = "";
  else
    si.si_service = str;

  return true;
}

bool CHTSPConnection::ParseQueueStatus (htsmsg_t* msg, SQueueStatus &queue)
{
  if(htsmsg_get_u32(msg, "packets", &queue.packets)
  || htsmsg_get_u32(msg, "bytes",   &queue.bytes)
  || htsmsg_get_u32(msg, "Bdrops",  &queue.bdrops)
  || htsmsg_get_u32(msg, "Pdrops",  &queue.pdrops)
  || htsmsg_get_u32(msg, "Idrops",  &queue.idrops))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return false;
  }

  /* delay isn't always transmitted */
  if(htsmsg_get_u32(msg, "delay", &queue.delay))
    queue.delay = 0;

  return true;
}

void CHTSPConnection::ParseDVREntryUpdate(htsmsg_t* msg, SRecordings &recordings)
{
  SRecording recording;
  const char *state;

  if(htsmsg_get_u32(msg, "id",      &recording.id)
  || htsmsg_get_u32(msg, "channel", &recording.channel)
  || htsmsg_get_u32(msg, "start",   &recording.start)
  || htsmsg_get_u32(msg, "stop",    &recording.stop)
  || (state = htsmsg_get_str(msg, "state")) == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }

  /* parse the dvr entry's state */
  if     (strstr(state, "scheduled"))
    recording.state = ST_SCHEDULED;
  else if(strstr(state, "recording"))
    recording.state = ST_RECORDING;
  else if(strstr(state, "completed"))
    recording.state = ST_COMPLETED;
  else if(strstr(state, "invalid"))
    recording.state = ST_INVALID;

  const char* str;
  if((str = htsmsg_get_str(msg, "title")) == NULL)
    recording.title = "";
  else
    recording.title = str;

  if((str = htsmsg_get_str(msg, "description")) == NULL)
    recording.description = "";
  else
    recording.description = str;

  if((str = htsmsg_get_str(msg, "error")) == NULL)
    recording.error = "";
  else
    recording.error = str;

  // if the user has aborted the recording then the recording.error will be set to 300 by tvheadend
  if (recording.error == "300")
  {
    recording.state = ST_ABORTED;
    recording.error.clear();
  }

  XBMC->Log(LOG_DEBUG, "%s - id:%u, state:'%s', title:'%s', description: '%s'"
      , __FUNCTION__, recording.id, state, recording.title.c_str()
      , recording.description.c_str());

  recordings[recording.id] = recording;

  PVR->TriggerTimerUpdate();

  if (recording.state == ST_RECORDING)
   PVR->TriggerRecordingUpdate();
}

void CHTSPConnection::ParseDVREntryDelete(htsmsg_t* msg, SRecordings &recordings)
{
  uint32_t id;

  if(htsmsg_get_u32(msg, "id", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }

  XBMC->Log(LOG_DEBUG, "%s - Recording %i was deleted", __FUNCTION__, id);

  recordings.erase(id);

  PVR->TriggerTimerUpdate();
  PVR->TriggerRecordingUpdate();
}
