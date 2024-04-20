/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeTop100.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

Node Top100Children[] = {
                          { NODE_TYPE_SONG_TOP100,  "songs",   10504 },
                          { NODE_TYPE_ALBUM_TOP100, "albums",  10505 },
                        };

CDirectoryNodeTop100::CDirectoryNodeTop100(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TOP100, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeTop100::GetChildType() const
{
  for (const Node& node : Top100Children)
    if (GetName() == node.id)
      return node.node;

  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeTop100::GetLocalizedName() const
{
  for (const Node& node : Top100Children)
    if (GetName() == node.id)
      return g_localizeStrings.Get(node.label);
  return "";
}

bool CDirectoryNodeTop100::GetContent(CFileItemList& items) const
{
  for (const Node& node : Top100Children)
  {
    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(node.label)));
    std::string strDir = StringUtils::Format("{}/", node.id);
    pItem->SetPath(BuildPath() + strDir);
    pItem->m_bIsFolder = true;
    items.Add(pItem);
  }

  return true;
}
