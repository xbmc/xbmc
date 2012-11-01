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

#include "DVDInputStreamHTSP.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/log.h"
#include <limits.h>

extern "C" {
#include "lib/libhts/net.h"
#include "lib/libhts/htsmsg.h"
#include "lib/libhts/htsmsg_binary.h"
#include "lib/libhts/sha1.h"
}

using namespace std;
using namespace HTSP;



htsmsg_t* CDVDInputStreamHTSP::ReadStream()
{
  htsmsg_t* msg;

  /* after anything has started reading, *
   * we can guarantee a new stream       */
  m_startup = false;

  while((msg = m_session.ReadMessage(1000)))
  {
    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
      return msg;

    if     (strstr(method, "channelAdd"))
      CHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      CHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelRemove"))
      CHTSPSession::ParseChannelRemove(msg, m_channels);

    uint32_t subs;
    if(htsmsg_get_u32(msg, "subscriptionId", &subs) || subs != m_subs)
    {
      htsmsg_destroy(msg);
      continue;
    }

    return msg;
  }
  return NULL;
}

CDVDInputStreamHTSP::CDVDInputStreamHTSP()
  : CDVDInputStream(DVDSTREAM_TYPE_HTSP)
  , m_subs(0)
  , m_startup(false)
  , m_channel(0)
  , m_tag(0)
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
  if(sscanf(url.GetFileName().c_str(), "tags/%d/%d", &m_tag, &m_channel) != 2)
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
  m_read.Clear();
}

int CDVDInputStreamHTSP::Read(BYTE* buf, int buf_size)
{
  size_t count = m_read.Size();
  if(count == 0)
  {
    htsmsg_t* msg = ReadStream();
    if(msg == NULL)
      return -1;

    uint8_t* p;
    if(htsmsg_binary_serialize(msg, (void**)&p, &count, INT_MAX) < 0)
    {
      htsmsg_destroy(msg);
      return -1;
    }
    htsmsg_destroy(msg);

    m_read.Clear();
    m_read.buf = p;
    m_read.cur = p;
    m_read.end = p + count;
  }

  if(count == 0)
    return 0;

  if(count > (size_t)buf_size)
    count = buf_size;

  memcpy(buf, m_read.cur, count);
  m_read.cur += count;
  return count;
}

bool CDVDInputStreamHTSP::SetChannel(int channel)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamHTSP::SetChannel - changing to channel %d", channel);

  if(!m_session.SendUnsubscribe(m_subs))
    CLog::Log(LOGERROR, "CDVDInputStreamHTSP::SetChannel - failed to unsubscribe from previous channel");

  if(!m_session.SendSubscribe(m_subs+1, channel))
  {
    if(m_session.SendSubscribe(m_subs, m_channel))
      CLog::Log(LOGERROR, "CDVDInputStreamHTSP::SetChannel - failed to set channel");
    else
      CLog::Log(LOGERROR, "CDVDInputStreamHTSP::SetChannel - failed to set channel and restore old channel");

    return false;
  }

  m_channel = channel;
  m_subs    = m_subs+1;
  m_startup = true;
  return true;
}

bool CDVDInputStreamHTSP::GetChannels(SChannelV &channels, SChannelV::iterator &it)
{
  for(SChannels::iterator it2 = m_channels.begin(); it2 != m_channels.end(); it2++)
  {
    if(m_tag == 0 || it2->second.MemberOf(m_tag))
      channels.push_back(it2->second);
  }
  sort(channels.begin(), channels.end());

  for(it = channels.begin(); it != channels.end(); it++)
    if(it->id == m_channel)
      return true;
  return false;
}

bool CDVDInputStreamHTSP::NextChannel(bool preview/* = false*/)
{
  SChannelV channels;
  SChannelV::iterator it;
  if(!GetChannels(channels, it))
    return false;

  SChannelC circ(channels.begin(), channels.end(), it);
  if(++circ == it)
    return false;
  else
    return SetChannel(circ->id);
}

bool CDVDInputStreamHTSP::PrevChannel(bool preview/* = false*/)
{
  SChannelV channels;
  SChannelV::iterator it;
  if(!GetChannels(channels, it))
    return false;

  SChannelC circ(channels.begin(), channels.end(), it);
  if(--circ == it)
    return false;
  else
    return SetChannel(circ->id);
}

bool CDVDInputStreamHTSP::SelectChannelByNumber(unsigned int channel)
{
  return SetChannel(channel);
}

bool CDVDInputStreamHTSP::UpdateItem(CFileItem& item)
{
  SChannels::iterator it = m_channels.find(m_channel);
  if(it == m_channels.end())
    return false;

  SChannel& channel = it->second;

  if(channel.event != m_event.id)
  {
    if(!m_session.GetEvent(m_event, channel.event))
    {
      m_event.Clear();
      m_event.id = channel.event;
    }
  }
  CFileItem current(item);
  CHTSPSession::ParseItem(channel, m_tag, m_event, item);
  item.SetArt("thumb", channel.icon);
  return current.GetPath()  != item.GetPath()
      || current.m_strTitle != item.m_strTitle;
}

int CDVDInputStreamHTSP::GetTotalTime()
{
    if(m_event.id == 0)
        return 0;
    return (m_event.stop - m_event.start) * 1000;
}

int CDVDInputStreamHTSP::GetTime()
{
  CDateTimeSpan time;
  time  = CDateTime::GetUTCDateTime()
        - CDateTime((time_t)m_event.start);

  return time.GetDays()    * 1000 * 60 * 60 * 24
       + time.GetHours()   * 1000 * 60 * 60
       + time.GetMinutes() * 1000 * 60
       + time.GetSeconds() * 1000;
}

void CDVDInputStreamHTSP::Abort()
{
  m_session.Abort();
}
