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
#include <netinet/in.h>
#include <netinet/tcp.h>

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

  while((msg = m_session.ReadMessage()))
  {
    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
      continue;

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

  SChannel& channel = it->second;

  if(channel.event != m_event.id)
  {
    if(!m_session.GetEvent(m_event, channel.event))
    {
      m_event.Clear();
      m_event.id = channel.event;
    }
  }
  CHTSPSession::ParseItem(channel, 0, m_event, item);
  item.SetThumbnailImage(channel.icon);
  item.SetCachedVideoThumb();
  return true;
}
