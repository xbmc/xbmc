/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeRoot.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeRoot::CDirectoryNodeRoot(const std::string& strName, CDirectoryNode* pParent, const std::string& strOrigin)
  : CDirectoryNode(NODE_TYPE_ROOT, strName, pParent, strOrigin)
{

}

NODE_TYPE CDirectoryNodeRoot::GetChildType() const
{
  return NODE_TYPE_OVERVIEW;
}
