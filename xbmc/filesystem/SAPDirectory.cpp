/*
 *  SAP-Announcement Support for XBMC
 *      Copyright (c) 2008 elupus (Joakim Plate)
 *      Copyright (C) 2008-2013 Team XBMC
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

#include "threads/SystemClock.h"
#include "system.h" // WIN32INCLUDES - not sure why this is needed
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include <string.h>
#include "SAPDirectory.h"
#include "FileItem.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"
#if defined(TARGET_DARWIN)
#include "osx/OSXGNUReplacements.h" // strnlen
#endif
#ifdef TARGET_FREEBSD
#include "freebsd/FreeBSDGNUReplacements.h"
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>

//using namespace std; On VS2010, bind conflicts with std::bind

CSAPSessions g_sapsessions;

namespace SDP
{
  /* SAP is always on that port */
  #define SAP_PORT 9875
  /* Global-scope SAP address */
  #define SAP_V4_GLOBAL_ADDRESS   "224.2.127.254"
  /* Organization-local SAP address */
  #define SAP_V4_ORG_ADDRESS      "239.195.255.255"
  /* Local (smallest non-link-local scope) SAP address */
  #define SAP_V4_LOCAL_ADDRESS    "239.255.255.255"
  /* Link-local SAP address */
  #define SAP_V4_LINK_ADDRESS     "224.0.0.255"
  #define ADD_SESSION 1

  #define SAP_V6_1 "FF0"
  /* Scope is inserted between them */
  #define SAP_V6_2 "::2:7FFE"

  int parse_sap(const char* data, int len, struct sap_desc *h)
  {
    const char* data_orig = data;

    if(len < 4)
      return -1;

    h->clear();
    h->version    = (data[0] >> 5) & 0x7;
    h->addrtype   = (data[0] >> 4) & 0x1;
    h->msgtype    = (data[0] >> 2) & 0x1;
    h->encrypted  = (data[0] >> 1) & 0x1;
    h->compressed = (data[0] >> 0) & 0x1;

    h->authlen    =  data[1];
    h->msgid      = ((data[2] << 8) | data[3]);

    data += 4;
    len  -= 4;

    if(h->addrtype) {

      if(len < 16) {
        CLog::Log(LOGERROR, "%s - too little data for origin address", __FUNCTION__);
        return -1;
      }

      CLog::Log(LOGERROR, "%s - ipv6 addresses currently unsupported", __FUNCTION__);
      return -1;
    } else {

      if(len < 4) {
        CLog::Log(LOGERROR, "%s - too little data for origin address", __FUNCTION__);
        return -1;
      }

      h->origin = inet_ntoa(*(struct in_addr*)data);
      data += 4;
      len  -= 4;
    }

    /* skip authentication data */
    data += h->authlen;
    len  -= h->authlen;

    /* payload type may be missing, then it's assumed to be sdp */
    if(data[0] == 'v'
    && data[1] == '='
    && data[2] == '0') {
      h->payload_type = "application/sdp";
      return data - data_orig;
    }

    /* read payload type */
    //no strnlen on non linux/win32 platforms, use handcrafted version
    int payload_size = strnlen(data, len);

    if (payload_size == len)
      return -1;
    h->payload_type.assign(data, payload_size);

    data += payload_size+1;
    len  -= payload_size+1;

    return data - data_orig;
  }

  static int parse_sdp_line(const char* data, std::string& type, std::string& value)
  {
    const char* data2 = data;
    int l;

    l = strcspn(data, "=\n\r");
    type.assign(data, l);
    data += l;
    if(*data == '=')
      data++;

    l = strcspn(data, "\n\r");
    value.assign(data, l);
    data += l;
    data += strspn(data, "\n\r");

    return data - data2;
  }

  static int parse_sdp_type(const char** data, std::string type, std::string& value)
  {
    std::string type2;
    const char* data2 = *data;

    while(*data2 != 0) {
      data2 += parse_sdp_line(data2, type2, value);
      if(type2 == type) {
        *data = data2;
        return 1;
      }
    }
    return 0;
  }

  int parse_sdp_token(const char* data, std::string &value)
  {
    int l;
    l = strcspn(data, " \n\r");
    value.assign(data, l);
    if(data[l] == '\0')
      return l;
    else
      return l+1;
  }

  int parse_sdp_token(const char* data, int &value)
  {
    int l;
    std::string str;
    l = parse_sdp_token(data, str);
    value = atoi(str.c_str());
    return l;
  }

  int parse_sdp_origin(const char* data, struct sdp_desc_origin *o)
  {
    const char *data2 = data;
    data += parse_sdp_token(data, o->username);
    data += parse_sdp_token(data, o->sessionid);
    data += parse_sdp_token(data, o->sessionver);
    data += parse_sdp_token(data, o->nettype);
    data += parse_sdp_token(data, o->addrtype);
    data += parse_sdp_token(data, o->address);
    return data - data2;
  }

  int parse_sdp(const char* data, struct sdp_desc* sdp)
  {
    const char *data2 = data;
    std::string value;

    // SESSION DESCRIPTION
    if(parse_sdp_type(&data, "v", value)) {
      sdp->version = value;
    } else
      return 0;

    if(parse_sdp_type(&data, "o", value)) {
      sdp->origin = value;
    } else
      return 0;

    if(parse_sdp_type(&data, "s", value)) {
      sdp->name = value;
    } else
      return 0;

    if(parse_sdp_type(&data, "i", value))
      sdp->info = value;

    if(parse_sdp_type(&data, "b", value))
      sdp->bandwidth = value;

    while(true) {
      if(parse_sdp_type(&data, "a", value))
        sdp->attributes.push_back(value);
      else
        break;
    }

    // TIME DESCRIPTIONS
    while(true) {
      sdp_desc_time time;
      if(parse_sdp_type(&data, "t", value))
        time.active = value;
      else
        break;

      if(parse_sdp_type(&data, "r", value))
        time.repeat = value;

      sdp->times.push_back(time);
    }

    // MEDIA DESCRIPTIONS
    while(true) {
      sdp_desc_media media;
      if(parse_sdp_type(&data, "m", value))
        media.name = value;
      else
        break;

      if(parse_sdp_type(&data, "i", value))
        media.title = value;

      if(parse_sdp_type(&data, "c", value))
        media.connection = value;

      while(true) {
        if(parse_sdp_type(&data, "a", value))
          media.attributes.push_back(value);
        else
          break;
      }

      sdp->media.push_back(media);
    }

    return data - data2;
  }
}


using namespace SDP;


CSAPSessions::CSAPSessions() : CThread("SAPSessions")
{
  m_socket = INVALID_SOCKET;
}

CSAPSessions::~CSAPSessions()
{
  StopThread();
}

void CSAPSessions::StopThread(bool bWait /*= true*/)
{
  if(m_socket != INVALID_SOCKET)
  {
    if(shutdown(m_socket, SHUT_RDWR) == SOCKET_ERROR)
      CLog::Log(LOGERROR, "s - failed to shutdown socket");
#ifdef WINSOCK_VERSION
    closesocket(m_socket);
#endif
  }
  CThread::StopThread(bWait);
}

bool CSAPSessions::ParseAnnounce(char* data, int len)
{
  CSingleLock lock(m_section);

  struct sap_desc header;
  int size = parse_sap(data, len, &header);
  if(size < 0)
  {
    CLog::Log(LOGERROR, "%s - failed to parse sap announcment", __FUNCTION__);
    return false;
  }

  // we only want sdp payloads
  if(header.payload_type != "application/sdp")
  {
    CLog::Log(LOGERROR, "%s - unknown payload type '%s'", __FUNCTION__, header.payload_type.c_str());
    return false;
  }

  data += size;
  len  -= size;

  sdp_desc desc;
  if(parse_sdp(data, &desc) < 0)
  {
    CLog::Log(LOGERROR, "%s - failed to parse sdp [ --->\n%s\n<--- ]", __FUNCTION__, data);
    return false;
  }

  // check if we can find this session in our cache
  for(std::vector<CSession>::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
  {
    if(it->origin         == header.origin
    && it->msgid          == header.msgid
    && it->payload_origin == desc.origin)
    {
      if(header.msgtype == 1)
      {
        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam("sap://");
        g_windowManager.SendThreadMessage(message);
        m_sessions.erase(it);
        return true;
      }

      // should be improved in the case of sdp
      it->timeout = XbmcThreads::SystemClockMillis() + 60*60*1000;
      return true;
    }
  }

  if(header.msgtype == 1)
    return true;

  sdp_desc_origin origin;
  if(parse_sdp_origin(desc.origin.c_str(), &origin) < 0)
  {
    CLog::Log(LOGERROR, "%s - failed to parse origin '%s'", __FUNCTION__, desc.origin.c_str());
    return false;
  }

  // add a new session to our buffer
  std::string user = origin.username;
  user = CURL::Encode(user);
  std::string path = StringUtils::Format("sap://%s/%s/0x%x.sdp", header.origin.c_str(), desc.origin.c_str(), header.msgid);
  CSession session;
  session.path           = path;
  session.origin         = header.origin;
  session.msgid          = header.msgid;
  session.payload_type   = header.payload_type;
  session.payload_origin = desc.origin;
  session.payload.assign(data, len);
  session.timeout = XbmcThreads::SystemClockMillis() + 60*60*1000;
  m_sessions.push_back(session);

  CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
  message.SetStringParam("sap://");
  g_windowManager.SendThreadMessage(message);

  return true;
}

void CSAPSessions::Process()
{
  struct ip_mreq mreq;

  m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(m_socket == INVALID_SOCKET)
    return;

#ifdef TARGET_WINDOWS
  unsigned long nonblocking = 1;
  ioctlsocket(m_socket, FIONBIO, &nonblocking);
#else
  fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFL) | O_NONBLOCK);
#endif

  const char one = 1;
  setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  /* bind to SAP port */
  struct sockaddr_in addr;
  addr.sin_family           = AF_INET;
  addr.sin_addr.s_addr      = INADDR_ANY;
  addr.sin_port             = htons(SAP_PORT);
  if(bind(m_socket, (const sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
    CLog::Log(LOGERROR, "CSAPSessions::Process - failed to bind to SAP port");
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
    return;
  }

  /* subscribe to all SAP multicast addresses */
  mreq.imr_multiaddr.s_addr = inet_addr(SAP_V4_GLOBAL_ADDRESS);
  mreq.imr_interface.s_addr = INADDR_ANY;
  setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

  mreq.imr_multiaddr.s_addr = inet_addr(SAP_V4_ORG_ADDRESS);
  mreq.imr_interface.s_addr = INADDR_ANY;
  setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

  mreq.imr_multiaddr.s_addr = inet_addr(SAP_V4_LOCAL_ADDRESS);
  mreq.imr_interface.s_addr = INADDR_ANY;
  setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

  mreq.imr_multiaddr.s_addr = inet_addr(SAP_V4_LINK_ADDRESS);
  mreq.imr_interface.s_addr = INADDR_ANY;
  setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

  /* start listening for announces */
  fd_set         readfds, expfds;
  struct timeval timeout;
  int count;

  char data[65507+1];

  while(!m_bStop)
  {
    timeout.tv_sec  = 5;
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_ZERO(&expfds);
    FD_SET(m_socket, &readfds);
    FD_SET(m_socket, &expfds);

    count = select((int)m_socket+1, &readfds, NULL, &expfds, &timeout);
    if(count == SOCKET_ERROR) {
      CLog::Log(LOGERROR, "%s - select returned error", __FUNCTION__);
      break;
    }

    if(FD_ISSET(m_socket, &readfds)) {
      count = recv(m_socket, data, sizeof(data), 0);

      if(count == SOCKET_ERROR) {
        CLog::Log(LOGERROR, "%s - recv returned error", __FUNCTION__);
        break;
      }
      /* data must be string terminated for parsers */
      data[count] = '\0';
      ParseAnnounce(data, count);
    }

  }

  closesocket(m_socket);
  m_socket = INVALID_SOCKET;
}

namespace XFILE
{

  CSAPDirectory::CSAPDirectory(void)
  {
  }



  CSAPDirectory::~CSAPDirectory(void)
  {
  }

  bool CSAPDirectory::GetDirectory(const CURL& url, CFileItemList &items)
  {
    if(!url.IsProtocol("sap"))
      return false;

    CSingleLock lock(g_sapsessions.m_section);

    if(!g_sapsessions.IsRunning())
      g_sapsessions.Create();

    // check if we can find this session in our cache
    for(std::vector<CSAPSessions::CSession>::iterator it = g_sapsessions.m_sessions.begin(); it != g_sapsessions.m_sessions.end(); ++it)
    {

      if(it->payload_type != "application/sdp")
      {
        CLog::Log(LOGDEBUG, "%s - unknown sdp payload type [%s]", __FUNCTION__, it->payload_type.c_str());
        continue;
      }
      struct sdp_desc desc;
      if(parse_sdp(it->payload.c_str(), &desc) <= 0)
      {
        CLog::Log(LOGDEBUG, "%s - invalid sdp payload [ --->\n%s\n<--- ]", __FUNCTION__, it->payload.c_str());
        continue;
      }

      struct sdp_desc_origin origin;
      if(parse_sdp_origin(desc.origin.c_str(), &origin) <= 0)
      {
        CLog::Log(LOGDEBUG, "%s - invalid sdp origin [ --->\n%s\n<--- ]", __FUNCTION__, desc.origin.c_str());
        continue;
      }

      CFileItemPtr item(new CFileItem());

      item->m_strTitle = desc.name;
      item->SetLabel(item->m_strTitle);
      if(desc.info.length() > 0 && desc.info != "N/A")
        item->SetLabel2(desc.info);
      item->SetLabelPreformated(true);

      item->SetPath(it->path);
      items.Add(item);
    }

    return true;
  }


}

