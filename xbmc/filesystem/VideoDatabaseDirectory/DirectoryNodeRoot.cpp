/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeRoot.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeRoot::CDirectoryNodeRoot(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ROOT, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeRoot::GetChildType() const
{
  return NODE_TYPE_OVERVIEW;
}
