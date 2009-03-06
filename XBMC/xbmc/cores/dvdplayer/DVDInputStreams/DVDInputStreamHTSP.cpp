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

htsmsg_t* CDVDInputStreamHTSP::ReadMessage()
{
  void*    buf;
  uint32_t l;

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

bool CDVDInputStreamHTSP::SendMessage(htsmsg_t* m)
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

htsmsg_t* CDVDInputStreamHTSP::ReadResult(htsmsg_t* m, bool sequence)
{
  if(sequence)
    htsmsg_add_u32(m, "seq", ++m_seq);

  if(!SendMessage(m))
    return NULL;

  while((m = ReadMessage()))
  {
    uint32_t seq;
    if(!sequence)
      break;
    if(!htsmsg_get_u32(m, "seq", &seq) && seq == m_seq)
      break;

    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::ReadResult - discarded message with invalid sequence number");
    htsmsg_print(m);
    htsmsg_destroy(m);
  }

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

bool CDVDInputStreamHTSP::ReadSuccess(htsmsg_t* m, bool sequence, std::string action)
{
  if((m = ReadResult(m, sequence)) == NULL)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamHTSP::ReadSuccess - failed to %s", action.c_str());
    return false;
  }
  htsmsg_destroy(m);
  return true;
}

htsmsg_t* CDVDInputStreamHTSP::ReadStream()
{
  htsmsg_t* msg;

  while((msg = ReadMessage()))
  {
    uint32_t subs;
    if(htsmsg_get_u32(msg, "subscriptionId", &subs) || subs != m_subs)
    {
      htsmsg_destroy(msg);
      continue;
    }
    m_startup = false;
    return msg;
  }
  return NULL;
}

CDVDInputStreamHTSP::CDVDInputStreamHTSP() 
  : CDVDInputStream(DVDSTREAM_TYPE_HTSP)
  , m_fd(INVALID_SOCKET)
  , m_seq(0)
  , m_subs(0)
  , m_channel(0)
  , m_startup(false)
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

  char errbuf[1024];
  int  errlen = sizeof(errbuf);
  htsmsg_t *m;
  const char *method, *server, *version;
  const void * chall = NULL;
  size_t chall_len = 0;
  int32_t proto = 0;

  if(url.GetPort() == 0)
    url.SetPort(9982);

  if(sscanf(url.GetFileName().c_str(), "channels/%d", &m_channel) != 1)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::Open - invalid url (%s)\n", url.GetFileName().c_str());
    return false;
  }

  m_fd = htsp_tcp_connect(url.GetHostName().c_str()
                        , url.GetPort()
                        , errbuf, errlen, 3000);
  if(m_fd == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::Open - failed to connect to server (%s)\n", errbuf);
    return false;
  }

  // read welcome
  if((m = ReadMessage()) == NULL)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::Open - failed to read greeting from server");
    return false;
  }
  method  = htsmsg_get_str(m, "method");
            htsmsg_get_s32(m, "htspversion", &proto);
  server  = htsmsg_get_str(m, "servername");
  version = htsmsg_get_str(m, "serverversion");
            htsmsg_get_bin(m, "challenge", &chall, &chall_len);

  CLog::Log(LOGDEBUG, "CDVDInputStreamHTSP::Open - connected to server: [%s], version: [%s], proto: %d"
                    , server ? server : "", version ? version : "", proto);

  htsmsg_destroy(m);

  m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "login");
  htsmsg_add_s32(m, "htspversion"   , proto);
  if(!url.GetUserName().IsEmpty())
    htsmsg_add_str(m, "username", url.GetUserName().c_str());

  if(!url.GetPassWord().IsEmpty() && chall)
  {
    struct HTSSHA1* shactx = (struct HTSSHA1*) malloc(hts_sha1_size);
    uint8_t d[20];
    hts_sha1_init(shactx);
    hts_sha1_update(shactx, (const uint8_t *)url.GetPassWord().c_str(), url.GetPassWord().length());
    hts_sha1_update(shactx, (const uint8_t *)chall, chall_len);
    hts_sha1_final(shactx, d);
    htsmsg_add_bin(m, "digest", d, 20);
    free(shactx);
  }

  if(!ReadSuccess(m, false, "get reply from authentication with server"))
    return false;

  if(!SendSubscribe(m_subs, m_channel))
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
  if(m_fd != INVALID_SOCKET)
    closesocket(m_fd);
  m_fd = INVALID_SOCKET;
}

int CDVDInputStreamHTSP::Read(BYTE* buf, int buf_size)
{
  return -1;
}

bool CDVDInputStreamHTSP::SendSubscribe(int subscription, int channel)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "subscribe");
  htsmsg_add_s32(m, "channelId"     , channel);
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return ReadSuccess(m, true, "subscribe to channel");
}

bool CDVDInputStreamHTSP::SendUnsubscribe(int subscription)
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method"        , "unsubscribe");
  htsmsg_add_s32(m, "subscriptionId", subscription);
  return ReadSuccess(m, true, "unsubscribe from channel");
}

bool CDVDInputStreamHTSP::SetChannel(int channel)
{

  if(!SendUnsubscribe(m_subs))
    return false;

  if(!SendSubscribe(m_subs+1, channel))
  {
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::SetChannel - failed to set channel, trying to restore...");
    SendSubscribe(m_subs, m_channel);
    return false;
  }
  m_channel = channel;
  m_subs    = m_subs+1;
  m_startup = true;
  return true;
}

bool CDVDInputStreamHTSP::NextChannel()
{
  return SetChannel(m_channel + 1);
}

bool CDVDInputStreamHTSP::PrevChannel()
{
  return SetChannel(m_channel - 1);
}
