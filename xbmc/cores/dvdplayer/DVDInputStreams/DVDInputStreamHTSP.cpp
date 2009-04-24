/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "DVDInputStreamHTSP.h"
#include "URL.h"
#include "VideoInfoTag.h"
#include "FileItem.h"
#include "utils/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

extern "C" {
#include "lib/libhts/net.h"
#include "lib/libhts/htsmsg.h"
#include "lib/libhts/htsmsg_binary.h"
#include "lib/libhts/sha1.h"
}

using namespace std;

CHTSPSession::CHTSPSession()
  : m_fd(INVALID_SOCKET)
  , m_seq(0)
  , m_challenge(NULL)
  , m_challenge_len(0)
  , m_queue_size(1000)
{
}

CHTSPSession::~CHTSPSession()
{
  Close();
}

void CHTSPSession::Close()
{
  if(m_fd != INVALID_SOCKET)
  {
    closesocket(m_fd);
    m_fd = INVALID_SOCKET;
  }

  if(m_challenge)
  {
    free(m_challenge);
    m_challenge     = NULL;
    m_challenge_len = 0;
  }
}

bool CHTSPSession::Connect(const std::string& hostname, int port)
{
  char errbuf[1024];
  int  errlen = sizeof(errbuf);
  htsmsg_t *m;
  const char *method, *server, *version;
  const void * chall = NULL;
  size_t chall_len = 0;
  int32_t proto = 0;

  if(port == 0)
    port = 9982;

  m_fd = htsp_tcp_connect(hostname.c_str()
                        , port
                        , errbuf, errlen, 3000);
  if(m_fd == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "CHTSPSession::Open - failed to connect to server (%s)\n", errbuf);
    return false;
  }

  // send hello
  m = htsmsg_create_map();
  htsmsg_add_str(m, "method", "hello");
  htsmsg_add_str(m, "clientname", "XBMC Media Center");
  htsmsg_add_u32(m, "htspversion", 1);

  // read welcome
  if((m = ReadResult(m)) == NULL)
  {
    CLog::Log(LOGERROR, "CHTSPSession::Open - failed to read greeting from server");
    return false;
  }
  method  = htsmsg_get_str(m, "method");
            htsmsg_get_s32(m, "htspversion", &proto);
  server  = htsmsg_get_str(m, "servername");
  version = htsmsg_get_str(m, "serverversion");
            htsmsg_get_bin(m, "challenge", &chall, &chall_len);

  CLog::Log(LOGDEBUG, "CHTSPSession::Open - connected to server: [%s], version: [%s], proto: %d"
                    , server ? server : "", version ? version : "", proto);

  if(chall && chall_len)
  {
    m_challenge     = malloc(chall_len);
    m_challenge_len = chall_len;
    memcpy(m_challenge, chall, chall_len);
  }

  htsmsg_destroy(m);
  return true;
}

bool CHTSPSession::Auth(const std::string& username, const std::string& password)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"  , "authenticate");
  htsmsg_add_str(m, "username", username.c_str());

  if(password != "" && m_challenge)
  {
    struct HTSSHA1* shactx = (struct HTSSHA1*) malloc(hts_sha1_size);
    uint8_t d[20];
    hts_sha1_init(shactx);
    hts_sha1_update(shactx
                 , (const uint8_t *)password.c_str()
                 , password.length());
    hts_sha1_update(shactx
                 , (const uint8_t *)m_challenge
                 , m_challenge_len);
    hts_sha1_final(shactx, d);
    htsmsg_add_bin(m, "digest", d, 20);
    free(shactx);
  }

  return ReadSuccess(m, false, "get reply from authentication with server");
}

htsmsg_t* CHTSPSession::ReadMessage()
{
  void*    buf;
  uint32_t l;

  if(m_queue.size())
  {
    htsmsg_t* m = m_queue.front();
    m_queue.pop_front();
    return m;
  }

  if(htsp_tcp_read_timeout(m_fd, &l, 4, 10000))
  {
    printf("Failed to read packet size\n");
    return NULL;
  }

  l   = ntohl(l);
  buf = malloc(l);

  if(htsp_tcp_read_timeout(m_fd, buf, l, 10000))
  {
    printf("Failed to read packet\n");
    free(buf);
    return NULL;
  }

  return htsmsg_binary_deserialize(buf, l, buf); /* consumes 'buf' */
}

bool CHTSPSession::SendMessage(htsmsg_t* m)
{
  void*  buf;
  size_t len;

  if(htsmsg_binary_serialize(m, &buf, &len, -1) < 0)
  {
    htsmsg_destroy(m);
    return false;
  }
  htsmsg_destroy(m);

  if(send(m_fd, buf, len, 0) < 0)
  {
    free(buf);
    return false;
  }
  free(buf);
  return true;
}

htsmsg_t* CHTSPSession::ReadResult(htsmsg_t* m, bool sequence)
{
  if(sequence)
    htsmsg_add_u32(m, "seq", ++m_seq);

  if(!SendMessage(m))
    return NULL;

  std::deque<htsmsg_t*> queue;
  m_queue.swap(queue);

  while((m = ReadMessage()))
  {
    uint32_t seq;
    if(!sequence)
      break;
    if(!htsmsg_get_u32(m, "seq", &seq) && seq == m_seq)
      break;

    queue.push_back(m);
    if(queue.size() >= m_queue_size)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamHTSP::ReadResult - maximum queue size (%u) reached", m_queue_size);
      m_queue.swap(queue);
      return NULL;
    }
  }

  m_queue.swap(queue);

  const char* error;
  if(m && (error = htsmsg_get_str(m, "error")))
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::ReadResult - error (%s)", error);
    htsmsg_destroy(m);
    return NULL;
  }
  uint32_t noaccess;
  if(m && !htsmsg_get_u32(m, "noaccess", &noaccess) && noaccess)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::ReadResult - access denied (%d)", noaccess);
    htsmsg_destroy(m);
    return NULL;
  }

  return m;
}

bool CHTSPSession::ReadSuccess(htsmsg_t* m, bool sequence, std::string action)
{
  if((m = ReadResult(m, sequence)) == NULL)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamHTSP::ReadSuccess - failed to %s", action.c_str());
    return false;
  }
  htsmsg_destroy(m);
  return true;
}

bool CHTSPSession::SendSubscribe(int subscription, int channel)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "subscribe");
  htsmsg_add_s32(m, "channelId"     , channel);
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return ReadSuccess(m, true, "subscribe to channel");
}

bool CHTSPSession::SendUnsubscribe(int subscription)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "unsubscribe");
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return ReadSuccess(m, true, "unsubscribe from channel");
}

bool CHTSPSession::SendEnableAsync()
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method", "enableAsyncMetadata");
  return ReadSuccess(m, true, "enableAsyncMetadata failed");
}

bool CHTSPSession::GetEvent(SEvent& event, int id)
{
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", id);
  if((msg = ReadResult(msg, true)) == NULL)
  {
    CLog::Log(LOGDEBUG, "CHTSPSession::GetEvent - failed to get event %d", id);
    return false;
  }
  uint32_t start, stop, next;
  const char *title, *desc;
  if(         htsmsg_get_u32(msg, "start", &start)
  ||          htsmsg_get_u32(msg, "stop" , &stop)
  || (title = htsmsg_get_str(msg, "title")) == NULL
  || (desc  = htsmsg_get_str(msg, "description"))  == NULL )
  {
    CLog::Log(LOGDEBUG, "CHTSPSession::GetEvent - malformed event");
    htsmsg_print(msg);
    htsmsg_destroy(msg);
    return false;
  }
  event.Clear();
  event.id    = id;
  event.start = start;
  event.stop  = stop;
  event.title = title;
  event.descs = desc;
  if(htsmsg_get_u32(msg, "nextEventId", &next))
    event.next = 0;
  else
    event.next = next;

  CLog::Log(LOGDEBUG, "CHTSPSession::GetEvent - id:%u, title:'%s', desc:'%s', start:%u, stop:%u, next:%u"
                    , event.id
                    , event.title.c_str()
                    , event.descs.c_str()
                    , event.start
                    , event.stop
                    , event.next);

  return true;
}

void CHTSPSession::OnChannelUpdate(htsmsg_t* msg, SChannels &channels)
{
  uint32_t id, event = 0;
  const char *name, *icon;
  if(         htsmsg_get_u32(msg, "channelId", &id)
  ||  (name = htsmsg_get_str(msg, "channelName")) == NULL)
  {
    CLog::Log(LOGERROR, "CHTSPSession::OnChannelUpdate - malformed message received");
    htsmsg_print(msg);
    return;
  }

  if(htsmsg_get_u32(msg, "eventId", &event))
    event = 0;

  if((icon = htsmsg_get_str(msg, "channelIcon")) == NULL)
    icon = "";

  CLog::Log(LOGDEBUG, "CHTSPSession::OnChannelUpdate - id:%u, name:'%s', icon:'%s', event:%u"
                    , id, name, icon, event);

  SChannel &channel = channels[id];
  channel.id    = id;
  channel.name  = name;
  channel.icon  = icon;
  channel.event = event;
}

void CHTSPSession::OnChannelRemove(htsmsg_t* msg, SChannels &channels)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "channelId", &id))
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::OnChannelUpdate - malformed message received");
    htsmsg_print(msg);
    return;
  }
  CLog::Log(LOGDEBUG, "CHTSPSession::OnChannelRemove - id:%u", id);

  channels.erase(id);
}


htsmsg_t* CDVDInputStreamHTSP::ReadStream()
{
  htsmsg_t* msg;

  while((msg = m_session.ReadMessage()))
  {
    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
      continue;

    if     (strstr(method, "channelAdd"))
      CHTSPSession::OnChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      CHTSPSession::OnChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelRemove"))
      CHTSPSession::OnChannelRemove(msg, m_channels);

    uint32_t subs;
    if(htsmsg_get_u32(msg, "subscriptionId", &subs) || subs != m_subs)
    {
      htsmsg_destroy(msg);
      continue;
    }

    // after we get the first subscriptionStart, no demuxer can start
    if(m_startup && strstr(method, "subscriptionStart"))
      m_startup = false;

    return msg;
  }
  return NULL;
}

CDVDInputStreamHTSP::CDVDInputStreamHTSP() 
  : CDVDInputStream(DVDSTREAM_TYPE_HTSP)
  , m_subs(0)
  , m_startup(false)
  , m_channel(0)
{
}

CDVDInputStreamHTSP::~CDVDInputStreamHTSP()
{
  Close();
}

bool CDVDInputStreamHTSP::Open(const char* file, const std::string& content)
{
  if (!CDVDInputStream::Open(file, content)) 
    return false;

  CURL url(file);
  if(sscanf(url.GetFileName().c_str(), "channels/%d", &m_channel) != 1)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::Open - invalid url (%s)\n", url.GetFileName().c_str());
    return false;
  }

  if(!m_session.Connect(url.GetHostName(), url.GetPort()))
    return false;

  if(!url.GetUserName().IsEmpty())
    m_session.Auth(url.GetUserName(), url.GetPassWord());

  m_session.SendEnableAsync();

  if(!m_session.SendSubscribe(m_subs, m_channel))
    return false;

  m_startup = true;
  return true;
}

bool CDVDInputStreamHTSP::IsEOF()
{
  return false;
}

void CDVDInputStreamHTSP::Close()
{
  CDVDInputStream::Close();
  m_session.Close();
}

int CDVDInputStreamHTSP::Read(BYTE* buf, int buf_size)
{
  return -1;
}

bool CDVDInputStreamHTSP::SetChannel(int channel)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamHTSP::SetChannel - changing to channel %d", channel);

  if(!m_session.SendSubscribe(m_subs+1, channel))
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::SetChannel - failed to set channel");
    return false;
  }

  if(!m_session.SendUnsubscribe(m_subs))
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::SetChannel - failed to unsubscribe from previous channel");

  m_channel = channel;
  m_subs    = m_subs+1;
  m_startup = true;
  return true;
}

bool CDVDInputStreamHTSP::NextChannel()
{
  if(m_channels.size() == 0)
    return SetChannel(m_channel + 1);
  if(m_channels.size() == 1)
    return false;

  SChannels::iterator it = m_channels.find(m_channel);
  if(it == m_channels.end())
    it = m_channels.begin();
  else
    it++;

  if(it == m_channels.end())
    it = m_channels.begin();
  return SetChannel(it->first);
}

bool CDVDInputStreamHTSP::PrevChannel()
{
  if(m_channels.size() == 0)
    return SetChannel(m_channel - 1);
  if(m_channels.size() == 1)
    return false;

  SChannels::iterator it = m_channels.find(m_channel);
  if(it == m_channels.begin())
    it = m_channels.end();
  it--;
  return SetChannel(it->first);
}

bool CDVDInputStreamHTSP::UpdateItem(CFileItem& item)
{
  SChannels::iterator it = m_channels.find(m_channel);
  if(it == m_channels.end())
    return false;

  SChannel&  channel = it->second;
  CVideoInfoTag* tag = item.GetVideoInfoTag();

  if(channel.event != m_event.id)
  {
    if(!m_session.GetEvent(m_event, channel.event))
    {
      m_event.Clear();
      m_event.id = channel.event;
    }
  }
  CStdString temp, path;

  CURL url(item.m_strPath);
  temp.Format("channels/%d", channel.id);
  url.SetFileName(temp);
  url.GetURL(path);

  /* check if we don't need to modify */
  if(tag->m_strAlbum     == channel.name
  && tag->m_strShowTitle == m_event.title
  && tag->m_strPlot      == m_event.descs
  && tag->m_iSeason      == 0
  && tag->m_iEpisode     == 0
  && item.m_strPath      == path)
    return false;

  tag->m_iSeason  = 0;
  tag->m_iEpisode = 0;
  tag->m_strAlbum     = channel.name;
  tag->m_strShowTitle = m_event.title;
  tag->m_strPlot      = m_event.descs;

  tag->m_strTitle = tag->m_strAlbum;
  if(tag->m_strShowTitle.length() > 0)
    tag->m_strTitle += " : " + tag->m_strShowTitle;

  item.m_strPath  = path;
  item.m_strTitle = tag->m_strAlbum;
  item.SetThumbnailImage(channel.icon);
  item.SetCachedVideoThumb();
  return true;
}
