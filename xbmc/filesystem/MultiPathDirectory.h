#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <set>
#include "IDirectory.h"
#include "utils/StdString.h"

namespace XFILE
{
class CMultiPathDirectory :
      public IDirectory
{
public:
  CMultiPathDirectory(void);
  virtual ~CMultiPathDirectory(void);
  virtual bool GetDirectory(const CURL& url, CFileItemList &items);
  virtual bool Exists(const CURL& url);
  virtual bool Remove(const CURL& url);

  static CStdString GetFirstPath(const CStdString &strPath);
  static bool SupportsWriteFileOperations(const CStdString &strPath);
  static bool GetPaths(const CURL& url, std::vector<CStdString>& vecPaths);
  static bool GetPaths(const CStdString& strPath, std::vector<CStdString>& vecPaths);
  static bool HasPath(const CStdString& strPath, const CStdString& strPathToFind);
  static CStdString ConstructMultiPath(const std::vector<std::string> &vecPaths);
  static CStdString ConstructMultiPath(const std::set<std::string> &setPaths);

private:
  void MergeItems(CFileItemList &items);
  static void AddToMultiPath(CStdString& strMultiPath, const CStdString& strPath);
  CStdString ConstructMultiPath(const CFileItemList& items, const std::vector<int> &stack);
};
}
