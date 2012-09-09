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

#include "MusicFileDirectory.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicFileDirectory::CMusicFileDirectory(void)
{
}

CMusicFileDirectory::~CMusicFileDirectory(void)
{
}

bool CMusicFileDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  CStdString strPath=strPath1;

  CStdString strFileName;
  strFileName = URIUtils::GetFileName(strPath);
  URIUtils::RemoveExtension(strFileName);

  int iStreams = GetTrackCount(strPath1);

  URIUtils::AddSlashAtEnd(strPath);

  for (int i=0; i<iStreams; ++i)
  {
    CStdString strLabel;
    strLabel.Format("%s - %s %02.2i", strFileName.c_str(),g_localizeStrings.Get(554).c_str(),i+1);
    CFileItemPtr pItem(new CFileItem(strLabel));
    strLabel.Format("%s%s-%i.%s", strPath.c_str(),strFileName.c_str(),i+1,m_strExt.c_str());
    pItem->SetPath(strLabel);

    if (m_tag.Loaded())
      *pItem->GetMusicInfoTag() = m_tag;

    pItem->GetMusicInfoTag()->SetTrackNumber(i+1);
    items.Add(pItem);
  }

  return true;
}

bool CMusicFileDirectory::Exists(const char* strPath)
{
  return true;
}

bool CMusicFileDirectory::ContainsFiles(const CStdString& strPath)
{
  if (GetTrackCount(strPath) > 1)
    return true;

  return false;
}
