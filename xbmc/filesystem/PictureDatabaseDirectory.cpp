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

#include "PictureDatabaseDirectory.h"
#include "FileItem.h"

using namespace std;
using namespace XFILE;
using namespace PICTUREDATABASEDIRECTORY;

bool CPictureDatabaseDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return false;

  bool bResult = pNode->GetChilds(items);
  items.SetLabel(pNode->GetLocalizedName());

  return bResult;
}

NODE_TYPE CPictureDatabaseDirectory::GetDirectoryChildType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (pNode.get())
    return pNode->GetChildType();
  return NODE_TYPE_NONE;
}

NODE_TYPE CPictureDatabaseDirectory::GetDirectoryType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (pNode.get())
    return pNode->GetType();
  return NODE_TYPE_NONE;
}

NODE_TYPE CPictureDatabaseDirectory::GetDirectoryParentType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (pNode.get())
  {
    CDirectoryNode* pParentNode = pNode->GetParent();
    if (pParentNode)
      return pParentNode->GetChildType();
  }
  return NODE_TYPE_NONE;
}
