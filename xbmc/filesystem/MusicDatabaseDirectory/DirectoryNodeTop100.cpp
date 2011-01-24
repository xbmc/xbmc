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

#include "DirectoryNodeTop100.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"

using namespace std;
using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeTop100::CDirectoryNodeTop100(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TOP100, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeTop100::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_SONG_TOP100;
  else if (GetName()=="2")
    return NODE_TYPE_ALBUM_TOP100;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeTop100::GetContent(CFileItemList& items)
{
  vector<CStdString> vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(10504));  // Top 100 Songs
  vecRoot.push_back(g_localizeStrings.Get(10505));  // Top 100 Albums

  for (int i = 0; i < (int)vecRoot.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(vecRoot[i]));
    CStdString strDir;
    strDir.Format("%i/", i+1);
    pItem->m_strPath += BuildPath() + strDir;
    pItem->m_bIsFolder = true;
    items.Add(pItem);
  }

  return true;
}
