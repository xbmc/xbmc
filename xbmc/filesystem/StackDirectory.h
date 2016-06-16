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

#include "IDirectory.h"
#include "utils/RegExp.h"
#include <string>
#include <vector>

namespace XFILE
{
  class CStackDirectory : public IDirectory
  {
  public:
    CStackDirectory();
    ~CStackDirectory();
    virtual bool GetDirectory(const CURL& url, CFileItemList& items);
    virtual bool AllowAll() const { return true; }
    static std::string GetStackedTitlePath(const std::string &strPath);
    static std::string GetStackedTitlePath(const std::string &strPath, VECCREGEXP& RegExps);
    static std::string GetFirstStackedFile(const std::string &strPath);
    static bool GetPaths(const std::string& strPath, std::vector<std::string>& vecPaths);
    static std::string ConstructStackPath(const CFileItemList& items, const std::vector<int> &stack);
    static bool ConstructStackPath(const std::vector<std::string> &paths, std::string &stackedPath);
  };
}
