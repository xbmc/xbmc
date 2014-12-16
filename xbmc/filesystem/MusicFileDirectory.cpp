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

#include "MusicFileDirectory.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicFileDirectory::CMusicFileDirectory(void)
{
}

CMusicFileDirectory::~CMusicFileDirectory(void)
{
}

bool CMusicFileDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string strPath=url.Get();

  std::string strFileName;
  strFileName = URIUtils::GetFileName(strPath);
  URIUtils::RemoveExtension(strFileName);

  int iStreams = GetTrackCount(strPath);

  URIUtils::AddSlashAtEnd(strPath);

  for (int i=0; i<iStreams; ++i)
  {
    std::string strLabel = StringUtils::Format("%s - %s %02.2i", strFileName.c_str(), g_localizeStrings.Get(554).c_str(), i+1);
    CFileItemPtr pItem(new CFileItem(strLabel));
    strLabel = StringUtils::Format("%s%s-%i.%s", strPath.c_str(), strFileName.c_str(), i+1, m_strExt.c_str());
    pItem->SetPath(strLabel);

    if (m_tag.Loaded())
      *pItem->GetMusicInfoTag() = m_tag;

    pItem->GetMusicInfoTag()->SetTrackNumber(i+1);
    items.Add(pItem);
  }

  return true;
}

bool CMusicFileDirectory::Exists(const CURL& url)
{
  return true;
}

bool CMusicFileDirectory::ContainsFiles(const CURL &url)
{
  const std::string pathToUrl(url.Get());
  if (GetTrackCount(pathToUrl) > 1)
    return true;

  return false;
}
