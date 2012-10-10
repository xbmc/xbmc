/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "HTSPSession.h"
#include "URL.h"
#include "video/VideoInfoTag.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

extern "C" {
#include "libhts/net.h"
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}


struct SContentType
{
  unsigned    id;
  const char* genre; 
};

static const SContentType g_dvb_content_group[] =
{ { 0x1, "Movie/Drama" }
, { 0x2, "News/Current Affairs" }
, { 0x3, "Show/Game show" }
, { 0x4, "Sports" }
, { 0x5, "Children's/Youth" }
, { 0x6, "Music/Ballet/Dance" }
, { 0x7, "Arts/Culture (without music)" }
, { 0x8, "Social/Political issues/Economics" }
, { 0x9, "Childrens/Youth Education/Science/Factual" }
, { 0xa, "Leisure hobbies" }
, { 0xb, "Misc" }
, { 0xf, "Unknown" }
};

static const SContentType g_dvb_content_type[] =
{
// movie/drama
  { 0x11, "Detective/Thriller" }
, { 0x12, "Adventure/Western/War" }
, { 0x13, "Science Fiction/Fantasy/Horror" }
, { 0x14, "Comedy" }
, { 0x15, "Soap/Melodrama/Folkloric" }
, { 0x16, "Romance" }
, { 0x17, "Serious/ClassicalReligion/Historical" }
, { 0x18, "Adult Movie/Drama" }

// news/current affairs
, { 0x21, "News/Weather Report" }
, { 0x22, "Magazine" }
, { 0x23, "Documentary" }
, { 0x24, "Discussion/Interview/Debate" }

// show/game show
, { 0x31, "Game show/Quiz/Contest" }
, { 0x32, "Variety" }
, { 0x33, "Talk" }

// sports
, { 0x41, "Special Event (Olympics/World cup/...)" }
, { 0x42, "Magazine" }
, { 0x43, "Football/Soccer" }
, { 0x44, "Tennis/Squash" }
, { 0x45, "Team sports (excluding football)" }
, { 0x46, "Athletics" }
, { 0x47, "Motor Sport" }
, { 0x48, "Water Sport" }
, { 0x49, "Winter Sports" }
, { 0x4a, "Equestrian" }
, { 0x4b, "Martial sports" }

// childrens/youth
, { 0x51, "Pre-school" }
, { 0x52, "Entertainment (6 to 14 year-olds)" }
, { 0x53, "Entertainment (10 to 16 year-olds)" }
, { 0x54, "Informational/Educational/Schools" }
, { 0x55, "Cartoons/Puppets" }

// music/ballet/dance
, { 0x61, "Rock/Pop" }
, { 0x62, "Serious music/Classical Music" }
, { 0x63, "Folk/Traditional music" }
, { 0x64, "Jazz" }
, { 0x65, "Musical/Opera" }
, { 0x66, "Ballet" }

// arts/culture
, { 0x71, "Performing Arts" }
, { 0x72, "Fine Arts" }
, { 0x73, "Religion" }
, { 0x74, "Popular Culture/Tradital Arts" }
, { 0x75, "Literature" }
, { 0x76, "Film/Cinema" }
, { 0x77, "Experimental Film/Video" }
, { 0x78, "Broadcasting/Press" }
, { 0x79, "New Media" }
, { 0x7a, "Magazine" }
, { 0x7b, "Fashion" }

// social/political/economic
, { 0x81, "Magazine/Report/Domentary" }
, { 0x82, "Economics/Social Advisory" }
, { 0x83, "Remarkable People" }

// children's youth: educational/science/factual
, { 0x91, "Nature/Animals/Environment" }
, { 0x92, "Technology/Natural sciences" }
, { 0x93, "Medicine/Physiology/Psychology" }
, { 0x94, "Foreign Countries/Expeditions" }
, { 0x95, "Social/Spiritual Sciences" }
, { 0x96, "Further Education" }
, { 0x97, "Languages" }

// leisure hobbies
, { 0xa1, "Tourism/Travel" }
, { 0xa2, "Handicraft" }
, { 0xa3, "Motoring" }
, { 0xa4, "Fitness & Health" }
, { 0xa5, "Cooking" }
, { 0xa6, "Advertisement/Shopping" }
, { 0xa7, "Gardening" }

// misc
, { 0xb0, "Original Language" }
, { 0xb1, "Black and White" }
, { 0xb2, "Unpublished" }
, { 0xb3, "Live Broadcast" }
};

using namespace std;
using namespace HTSP;

string CHTSPSession::GetGenre(unsigned type)
{
  // look for full content
  for(unsigned int i = 0; i < sizeof(g_dvb_content_type) / sizeof(g_dvb_content_type[0]); i++)
  {
    if(g_dvb_content_type[i].id == type)
      return g_dvb_content_type[i].genre;
  }

  // look for group
  type = (type >> 4) & 0xf;
  for(unsigned int i = 0; i < sizeof(g_dvb_content_group) / sizeof(g_dvb_content_group[0]); i++)
  {
    if(g_dvb_content_group[i].id == type)
      return g_dvb_content_group[i].genre;
  }

  return "";
}

CHTSPSession::CHTSPSession()
  : m_fd(INVALID_SOCKET)
  , m_seq(0)
  , m_challenge(NULL)
  , m_challenge_len(0)
  , m_protocol(0)
  , m_queue_size(1000)
{
}

CHTSPSession::~CHTSPSession()
{
  Close();
}

void CHTSPSession::Abort()
{
  shutdown(m_fd, SHUT_RDWR);
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
//  const char *method;
  const char *server, *version;
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
//  method  = htsmsg_get_str(m, "method");
            htsmsg_get_s32(m, "htspversion", &proto);
  server  = htsmsg_get_str(m, "servername");
  version = htsmsg_get_str(m, "serverversion");
            htsmsg_get_bin(m, "challenge", &chall, &chall_len);

  CLog::Log(LOGDEBUG, "CHTSPSession::Open - connected to server: [%s], version: [%s], proto: %d"
                    , server ? server : "", version ? version : "", proto);

  m_protocol = proto;

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

htsmsg_t* CHTSPSession::ReadMessage(int timeout)
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

  x = htsp_tcp_read_timeout(m_fd, &l, 4, timeout);
  if(x == ETIMEDOUT)
    return htsmsg_create_map();

  if(x)
  {
    CLog::Log(LOGERROR, "CHTSPSession::ReadMessage - Failed to read packet size (%d)\n", x);
    return NULL;
  }

  l   = ntohl(l);
  if(l == 0)
    return htsmsg_create_map();

  buf = malloc(l);

  x = htsp_tcp_read(m_fd, buf, l);
  if(x)
  {
    CLog::Log(LOGERROR, "CHTSPSession::ReadMessage - Failed to read packet (%d)\n", x);
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

  if(send(m_fd, (char*)buf, len, 0) < 0)
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

bool CHTSPSession::GetEvent(SEvent& event, uint32_t id)
{
  if(id == 0)
  {
    event.Clear();
    return false;
  }

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", id);
  if((msg = ReadResult(msg, true)) == NULL)
  {
    CLog::Log(LOGDEBUG, "CHTSPSession::GetEvent - failed to get event %d", id);
    return false;
  }
  return ParseEvent(msg, id, event);
}

bool CHTSPSession::ParseEvent(htsmsg_t* msg, uint32_t id, SEvent &event)
{
  uint32_t start, stop, next, content;
  const char *title, *desc;
  if(         htsmsg_get_u32(msg, "start", &start)
  ||          htsmsg_get_u32(msg, "stop" , &stop)
  || (title = htsmsg_get_str(msg, "title")) == NULL)
  {
    CLog::Log(LOGDEBUG, "CHTSPSession::ParseEvent - malformed event");
    htsmsg_print(msg);
    htsmsg_destroy(msg);
    return false;
  }
  event.Clear();
  event.id    = id;
  event.start = start;
  event.stop  = stop;
  event.title = title;
  if((desc = htsmsg_get_str(msg, "description")))
    event.descs = desc;
  if(htsmsg_get_u32(msg, "nextEventId", &next))
    event.next = 0;
  else
    event.next = next;

  if(htsmsg_get_u32(msg, "contentType", &content) == 0)
    event.content = content;

  CLog::Log(LOGDEBUG, "CHTSPSession::ParseEvent - id:%u, title:'%s', desc:'%s', start:%u, stop:%u, next:%u, content:%u"
                    , event.id
                    , event.title.c_str()
                    , event.descs.c_str()
                    , event.start
                    , event.stop
                    , event.next
                    , event.content);

  return true;
}

void CHTSPSession::ParseChannelUpdate(htsmsg_t* msg, SChannels &channels)
{
  uint32_t id, event = 0, num = 0;
  const char *name, *icon;
  if(htsmsg_get_u32(msg, "channelId", &id))
  {
    CLog::Log(LOGERROR, "CHTSPSession::ParseChannelUpdate - malformed message received");
    htsmsg_print(msg);
    return;
  }

  SChannel &channel = channels[id];
  channel.id = id;

  if(htsmsg_get_u32(msg, "eventId", &event) == 0)
    channel.event = event;

  if((name = htsmsg_get_str(msg, "channelName")))
    channel.name = name;

  if((icon = htsmsg_get_str(msg, "channelIcon")))
    channel.icon = icon;

  if(htsmsg_get_u32(msg, "channelNumber", &num) == 0)
  {
    if(num == 0)
      channel.num = id + 1000;
    else
      channel.num = num;
  }
  else
    channel.num = id; // fallback older servers

  htsmsg_t *tags;

  if((tags = htsmsg_get_list(msg, "tags")))
  {
    channel.tags.clear();

    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, tags)
    {
      if(f->hmf_type != HMF_S64)
        continue;
      channel.tags.push_back((int)f->hmf_s64);
    }
  }


  CLog::Log(LOGDEBUG, "CHTSPSession::ParseChannelUpdate - id:%u, name:'%s', icon:'%s', event:%u"
                    , id, name ? name : "(null)", icon ? icon : "(null)", event);
}

void CHTSPSession::ParseChannelRemove(htsmsg_t* msg, SChannels &channels)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "channelId", &id))
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::ParseChannelRemove - malformed message received");
    htsmsg_print(msg);
    return;
  }
  CLog::Log(LOGDEBUG, "CHTSPSession::ParseChannelRemove - id:%u", id);

  channels.erase(id);
}

void CHTSPSession::ParseTagUpdate(htsmsg_t* msg, STags &tags)
{
  uint32_t id;
  const char *name, *icon;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    CLog::Log(LOGERROR, "CHTSPSession::ParseTagUpdate - malformed message received");
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

  CLog::Log(LOGDEBUG, "CHTSPSession::ParseTagUpdate - id:%u, name:'%s', icon:'%s'"
                    , id, name ? name : "(null)", icon ? icon : "(null)");

}

void CHTSPSession::ParseTagRemove(htsmsg_t* msg, STags &tags)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    CLog::Log(LOGERROR, "CHTSPSession::ParseTagRemove - malformed message received");
    htsmsg_print(msg);
    return;
  }
  CLog::Log(LOGDEBUG, "CHTSPSession::ParseTagRemove - id:%u", id);

  tags.erase(id);
}

bool CHTSPSession::ParseItem(const SChannel& channel, int tagid, const SEvent& event, CFileItem& item)
{
  CVideoInfoTag* tag = item.GetVideoInfoTag();

  CStdString temp;

  CURL url(item.GetPath());
  temp.Format("tags/%d/%d.ts", tagid, channel.id);
  url.SetFileName(temp);

  tag->m_iSeason  = 0;
  tag->m_iEpisode = 0;
  tag->m_iTrack       = channel.num;
  tag->m_strAlbum     = channel.name;
  tag->m_strShowTitle = event.title;
  tag->m_strPlot      = event.descs;
  tag->m_strStatus    = "livetv";
  tag->m_genre        = StringUtils::Split(GetGenre(event.content), g_advancedSettings.m_videoItemSeparator);

  tag->m_strTitle = tag->m_strAlbum;
  if(tag->m_strShowTitle.length() > 0)
    tag->m_strTitle += " : " + tag->m_strShowTitle;

  item.SetPath(url.Get());
  item.m_strTitle = tag->m_strTitle;
  item.SetArt("thumb", channel.icon);
  item.SetMimeType("video/X-htsp");
  return true;
}

bool CHTSPSession::ParseQueueStatus (htsmsg_t* msg, SQueueStatus &queue)
{
  if(htsmsg_get_u32(msg, "packets", &queue.packets)
  || htsmsg_get_u32(msg, "bytes",   &queue.bytes)
  || htsmsg_get_u32(msg, "Bdrops",  &queue.bdrops)
  || htsmsg_get_u32(msg, "Pdrops",  &queue.pdrops)
  || htsmsg_get_u32(msg, "Idrops",  &queue.idrops))
  {
    CLog::Log(LOGERROR, "CHTSPSession::ParseQueueStatus - malformed message received");
    htsmsg_print(msg);
    return false;
  }

  /* delay isn't always transmitted */
  if(htsmsg_get_u32(msg, "delay", &queue.delay))
    queue.delay = 0;

  return true;
}
