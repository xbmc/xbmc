/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

namespace XFILE
{

namespace VIDEODATABASEDIRECTORY
{
class CQueryParams;
enum class NodeType;
} // namespace VIDEODATABASEDIRECTORY

  class CVideoDatabaseDirectory : public IDirectory
  {
  public:
    CVideoDatabaseDirectory(void);
    ~CVideoDatabaseDirectory(void) override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool Exists(const CURL& url) override;
    bool AllowAll() const override { return true; }
    static VIDEODATABASEDIRECTORY::NodeType GetDirectoryChildType(const std::string& strPath);
    static VIDEODATABASEDIRECTORY::NodeType GetDirectoryType(const std::string& strPath);
    static VIDEODATABASEDIRECTORY::NodeType GetDirectoryParentType(const std::string& strPath);
    static bool GetQueryParams(const std::string& strPath, VIDEODATABASEDIRECTORY::CQueryParams& params);
    void ClearDirectoryCache(const std::string& strDirectory);
    static bool IsAllItem(const std::string& strDirectory);
    static bool GetLabel(const std::string& strDirectory, std::string& strLabel);
    static std::string GetIcon(const std::string& strDirectory);
    bool ContainsMovies(const std::string &path);
    static bool CanCache(const std::string &path);
  };
}
