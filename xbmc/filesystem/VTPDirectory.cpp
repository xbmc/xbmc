/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef FILESYSTEM_VTPDIRECTORY_H_INCLUDED
#define FILESYSTEM_VTPDIRECTORY_H_INCLUDED
#include "VTPDirectory.h"
#endif

#ifndef FILESYSTEM_VTPSESSION_H_INCLUDED
#define FILESYSTEM_VTPSESSION_H_INCLUDED
#include "VTPSession.h"
#endif

#ifndef FILESYSTEM_URL_H_INCLUDED
#define FILESYSTEM_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef FILESYSTEM_FILEITEM_H_INCLUDED
#define FILESYSTEM_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif



using namespace std;
using namespace XFILE;

CVTPDirectory::CVTPDirectory()
{
  m_session = new CVTPSession();
}

CVTPDirectory::~CVTPDirectory()
{
  delete m_session;
}

bool CVTPDirectory::GetChannels(const CStdString& base, CFileItemList &items)
{
  vector<CVTPSession::Channel> channels;
  if(!m_session->GetChannels(channels))
    return false;

  vector<CVTPSession::Channel>::iterator it;
  for(it = channels.begin(); it != channels.end(); it++)
  {
    CFileItemPtr item(new CFileItem("", false));

    CStdString buffer = StringUtils::Format("%s/%d.ts", base.c_str(), it->index);
    item->SetPath(buffer);
    item->m_strTitle = it->name;
    buffer = StringUtils::Format("%d - %s", it->index, it->name.c_str());
    item->SetLabel(buffer);

    items.Add(item);
  }
  return true;
}

bool CVTPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);

  if(url.GetHostName() == "")
    url.SetHostName("localhost");

  CStdString base = url.Get();
  URIUtils::RemoveSlashAtEnd(base);

  // add port after, it changes the structure
  // and breaks CUtil::GetMatchingSource
  if(url.GetPort() == 0)
    url.SetPort(2004);

  if(!m_session->Open(url.GetHostName(), url.GetPort()))
    return false;

  if(url.GetFileName().empty())
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/channels/", true));
    item->SetLabel("Live Channels");
    item->SetLabelPreformated(true);
    items.Add(item);
    return true;
  }
  else if(url.GetFileName() == "channels/")
    return GetChannels(base, items);
  else
    return false;
}

