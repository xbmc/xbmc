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

#include "MultiTrackDirectory.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMultiTrackDirectory::CMultiTrackDirectory(int tracks)
{
  m_tracks = tracks;
}

CMultiTrackDirectory::~CMultiTrackDirectory(void)
{
}

bool CMultiTrackDirectory::GetDirectory(const CStdString& strPath,
                                        CFileItemList &items)
{
  CStdString strFileName;
  strFileName = URIUtils::GetFileName(strPath);
  URIUtils::RemoveExtension(strFileName);

  for (int i=0;i<m_tracks;++i)
  {
    CStdString strLabel;
    strLabel.Format("%s - %s %02.2i", strFileName.c_str(),g_localizeStrings.Get(554).c_str(),i+1);
    CFileItemPtr pItem(new CFileItem(strLabel));
    pItem->m_strPath = strPath;
    pItem->m_lStartOffset = i+1;

    items.Add(pItem);
  }

  return true;
}

bool CMultiTrackDirectory::Exists(const char* strPath)
{
  return true;
}

bool CMultiTrackDirectory::ContainsFiles(const CStdString& strPath)
{
  return (m_tracks > 1);
}
