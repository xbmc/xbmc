/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DirectoryNode.h"

namespace XFILE
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeTitleMusicVideos : public CDirectoryNode
    {
    public:
      CDirectoryNodeTitleMusicVideos(const std::string& strEntryName, CDirectoryNode* pParent);
    protected:
      bool GetContent(CFileItemList& item) const override;
    };
  }
}
