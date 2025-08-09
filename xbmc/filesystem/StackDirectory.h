/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "utils/RegExp.h"

#include <string>
#include <vector>

namespace XFILE
{
  class CStackDirectory : public IDirectory
  {
    typedef struct StackPart
    {
      std::string title;
      std::string volume{};

      auto operator<=>(const StackPart&) const = default;
    } StackPart;

  public:
    bool GetDirectory(const CURL& url, CFileItemList& items) override;
    bool AllowAll() const override { return true; }
    static std::string GetStackedTitlePath(const std::string& strPath);
    static std::string GetFirstStackedFile(const std::string &strPath);
    static bool GetPaths(const std::string& strPath, std::vector<std::string>& vecPaths);
    static std::string ConstructStackPath(const CFileItemList& items, const std::vector<int> &stack);
    static bool ConstructStackPath(const std::vector<std::string> &paths, std::string &stackedPath);
    static std::string GetParentPath(const std::string& stackPath);
  };
}
