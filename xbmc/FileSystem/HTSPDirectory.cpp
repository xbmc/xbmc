/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "HTSPDirectory.h"
#include "URL.h"
#include "Util.h"
#include "FileItem.h"
#include "Settings.h"
#include "cores/dvdplayer/DVDInputStreams/DVDInputStreamHTSP.h"

extern "C" {
#include "lib/libhts/htsmsg.h"
#include "lib/libhts/htsmsg_binary.h"
}

using namespace XFILE;
using namespace DIRECTORY;

CHTSPDirectorySession::CHTSPDirectorySession()
{
}

CHTSPDirectorySession::~CHTSPDirectorySession()
{
  Close();
}

CHTSPDirectorySession* CHTSPDirectorySession::Aquire(const CURL& url)
{
  CHTSPDirectorySession* session = new CHTSPDirectorySession();
  if(session->Open(url))
    return session;

  delete session;
  return NULL;
}

void CHTSPDirectorySession::Release(CHTSPDirectorySession* session)
{
  delete session;
}

bool CHTSPDirectorySession::Open(const CURL& url)
{
  if(!m_session.Connect(url.GetHostName(), url.GetPort()))
    return false;

  if(m_session.GetProtocol() < 2)
  {
    CLog::Log(LOGERROR, "CHTSPDirectory::GetDirectory - incompatible protocol version %d", m_session.GetProtocol());
    return false;
  }

  if(!url.GetUserName().IsEmpty())
    m_session.Auth(url.GetUserName(), url.GetPassWord());

  Create();

  m_started.WaitMSec(30000);
  return true;
}

void CHTSPDirectorySession::Close()
{
  m_bStop = true;
  m_session.Abort();
  StopThread();
}

htsmsg_t* CHTSPDirectorySession::ReadResult(htsmsg_t* m)
{
  CSingleLock lock(m_section);
  unsigned    seq (m_session.AddSequence());

  SMessage &message(m_queue[seq]);
  message.event = new CEvent();
  message.msg   = NULL;

  lock.Leave();
  htsmsg_add_u32(m, "seq", seq);
  if(!m_session.SendMessage(m))
  {
    m_queue.erase(seq);
    return NULL;
  }

  if(!message.event->WaitMSec(2000))
    CLog::Log(LOGERROR, "CHTSPDirectorySession::ReadResult - Timeout waiting for response");
  lock.Enter();

  m =    message.msg;
  delete message.event;

  m_queue.erase(seq);

  return m;
}

bool CHTSPDirectorySession::GetEvent(CHTSPSession::SEvent& event, uint32_t id)
{
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", id);
  if((msg = ReadResult(msg)) == NULL)
  {
    CLog::Log(LOGDEBUG, "CHTSPSession::GetEvent - failed to get event %u", id);
    return false;
  }
  return CHTSPSession::ParseEvent(msg, id, event);
}

void CHTSPDirectorySession::Process()
{
  CLog::Log(LOGDEBUG, "CHTSPDirectorySession::Process() - Starting");
  m_session.SendEnableAsync();

  htsmsg_t* msg;

  while(!m_bStop)
  {
    if((msg = m_session.ReadMessage()) == NULL)
      continue;

    uint32_t seq;
    if(htsmsg_get_u32(msg, "seq", &seq) == 0)
    {
      CSingleLock lock(m_section);
      SMessages::iterator it = m_queue.find(seq);
      if(it != m_queue.end())
      {
        it->second.msg = msg;
        it->second.event->Set();
        continue;
      }
    }

    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
    {
      htsmsg_destroy(msg);
      continue;
    }

    if     (strstr(method, "channelAdd"))
      CHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      CHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelRemove"))
      CHTSPSession::ParseChannelRemove(msg, m_channels);
    else if(strstr(method, "initialSyncCompleted"))
      m_started.Set();

    htsmsg_destroy(msg);
  }

  m_session.Close();
  CLog::Log(LOGDEBUG, "CHTSPDirectorySession::Process() - Exiting");
}

CHTSPSession::SChannels CHTSPDirectorySession::GetChannels()
{
  CSingleLock lock(m_section);
  return m_channels;
}

CHTSPDirectory::CHTSPDirectory(void)
{
}

CHTSPDirectory::~CHTSPDirectory(void)
{
}

bool CHTSPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL                    url(strPath);
  CHTSPDirectorySession   session;
  CHTSPSession::SChannels channels;

  if(!session.Open(url))
    return false;

  channels = session.GetChannels();

  for(CHTSPSession::SChannels::iterator it = channels.begin(); it != channels.end(); it++)
  {
    CHTSPSession::SChannel& channel(it->second);
    CHTSPSession::SEvent    event;

    if(!session.GetEvent(event, channel.event))
    {
      CLog::Log(LOGERROR, "CHTSPDirectory::GetDirectory - failed to get event %d", channel.event);
      event.Clear();
    }

    CFileItemPtr item(new CFileItem());

    url.SetFileName("");
    url.GetURL(item->m_strPath);
    CHTSPSession::ParseItem(channel, event, *item);
    item->m_bIsFolder = false;
    item->m_strTitle.Format("%d", channel.id);

    items.Add(item);
  }

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K[ - %B]", "%Z", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K[ - %B]", "%Z", "%L", ""));

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 20364, LABEL_MASKS("%Z", "%B", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 20364, LABEL_MASKS("%Z", "%B", "%L", ""));

  return channels.size() > 0;
}
