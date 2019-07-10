/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

#include <set>
#include <string>
#include <vector>

namespace XFILE
{
class CMultiPathDirectory :
      public IDirectory
{
public:
  CMultiPathDirectory(void);
  ~CMultiPathDirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList &items) override;
  bool Exists(const CURL& url) override;
  bool Remove(const CURL& url) override;

  static std::string GetFirstPath(const std::string &strPath);
  static bool SupportsWriteFileOperations(const std::string &strPath);
  static bool GetPaths(const CURL& url, std::vector<std::string>& vecPaths);
  static bool GetPaths(const std::string& path, std::vector<std::string>& paths);
  static bool HasPath(const std::string& strPath, const std::string& strPathToFind);
  static std::string ConstructMultiPath(const std::vector<std::string> &vecPaths);
  static std::string ConstructMultiPath(const std::set<std::string> &setPaths);

private:
  void MergeItems(CFileItemList &items);
  static void AddToMultiPath(std::string& strMultiPath, const std::string& strPath);
  std::string ConstructMultiPath(const CFileItemList& items, const std::vector<int> &stack);
};
}
