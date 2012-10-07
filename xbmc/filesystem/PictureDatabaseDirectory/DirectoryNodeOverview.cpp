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

#include "DirectoryNodeOverview.h"
#include "guilib/LocalizeStrings.h"
#include "pictures/PictureDatabase.h"
#include "FileItem.h"

namespace XFILE
{
  namespace PICTUREDATABASEDIRECTORY
  {
    Node OverviewChildren[] =
    {
      { NODE_TYPE_FOLDER,       20334 },
      { NODE_TYPE_YEAR,         345   },
      { NODE_TYPE_CAMERA,       21819 },
      { NODE_TYPE_TAGS,         20459 },
    };
  }
}

using namespace std;
using namespace XFILE::PICTUREDATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); i++)
    if (GetName() == GetVanity(OverviewChildren[i].node))
      return OverviewChildren[i].node;
  return NODE_TYPE_NONE;
}

CStdString CDirectoryNodeOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); i++)
    if (GetName() == GetVanity(OverviewChildren[i].node))
      return g_localizeStrings.Get(OverviewChildren[i].label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); i++)
  {
    CFileItemPtr pItem(new CFileItem);
    pItem->SetLabel(g_localizeStrings.Get(OverviewChildren[i].label));
    pItem->SetPath(BuildPath() + GetVanity(OverviewChildren[i].node) + "/");
    pItem->SetIconImage("DefaultFolder.png");
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }
  return true;
}
