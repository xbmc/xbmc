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


CHTSPDirectory::CHTSPDirectory(void)
{
}

CHTSPDirectory::~CHTSPDirectory(void)
{
}

bool CHTSPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL                    url(strPath);
  CHTSPSession            session;
  CHTSPSession::SChannels channels;

  if(!session.Connect(url.GetHostName(), url.GetPort()))
    return false;

  if(session.GetProtocol() < 2)
  {
    CLog::Log(LOGERROR, "CHTSPDirectory::GetDirectory - incompatible protocol version %d", session.GetProtocol());
    return false;
  }

  if(!url.GetUserName().IsEmpty())
    session.Auth(url.GetUserName(), url.GetPassWord());

  session.SendEnableAsync();

  DWORD     timeout = GetTickCount() + 20000;
  htsmsg_t* msg     = NULL;

  while((msg = session.ReadMessage()))
  {
    if(GetTickCount() > timeout)
    {
      htsmsg_destroy(msg);
      break;
    }

    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
    {
      htsmsg_destroy(msg);
      continue;
    }

    if     (strstr(method, "channelAdd"))
      CHTSPSession::OnChannelUpdate(msg, channels);
    else if(strstr(method, "channelUpdate"))
      CHTSPSession::OnChannelUpdate(msg, channels);
    else if(strstr(method, "channelRemove"))
      CHTSPSession::OnChannelRemove(msg, channels);
    else if(strstr(method, "initialSyncCompleted"))
    {
      htsmsg_destroy(msg);
      break;
    }

    htsmsg_destroy(msg);
  }

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
    CHTSPSession::UpdateItem(*item, channel, event);
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
