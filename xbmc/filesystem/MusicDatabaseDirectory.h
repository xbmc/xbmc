/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "MusicDatabaseDirectory/DirectoryNode.h"
#include "MusicDatabaseDirectory/QueryParams.h"

namespace XFILE
{
  class CMusicDatabaseDirectory : public IDirectory
  {
  public:
    CMusicDatabaseDirectory(void);
    ~CMusicDatabaseDirectory(void) override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool AllowAll() const override { return true; }
    bool Exists(const CURL& url) override;
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const std::string& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const std::string& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const std::string& strPath);
    static bool GetDirectoryNodeInfo(const std::string& strPath, MUSICDATABASEDIRECTORY::NODE_TYPE& type, MUSICDATABASEDIRECTORY::NODE_TYPE& childtype, MUSICDATABASEDIRECTORY::CQueryParams& params);
    bool IsArtistDir(const std::string& strDirectory);
    void ClearDirectoryCache(const std::string& strDirectory);
    static bool IsAllItem(const std::string& strDirectory);
    static bool GetLabel(const std::string& strDirectory, std::string& strLabel);
    bool ContainsSongs(const std::string &path);
    static bool CanCache(const std::string& strPath);
    static std::string GetIcon(const std::string& strDirectory);
  };
}
