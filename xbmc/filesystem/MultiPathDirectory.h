/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <set>
#include <string>
#include <vector>

#include "IDirectory.h"

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
