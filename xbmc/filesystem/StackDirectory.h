#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

namespace XFILE
{
  class CStackDirectory : public IDirectory
  {
  public:
    CStackDirectory();
    ~CStackDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool IsAllowed(const CStdString &strFile) const { return true; };
    static CStdString GetStackedTitlePath(const CStdString &strPath);
    static CStdString GetStackedTitlePath(const CStdString &strPath, VECCREGEXP& RegExps);
    static CStdString GetFirstStackedFile(const CStdString &strPath);
    static bool GetPaths(const CStdString& strPath, std::vector<CStdString>& vecPaths);
    static CStdString ConstructStackPath(const CFileItemList& items, const std::vector<int> &stack);
    static bool ConstructStackPath(const std::vector<CStdString> &paths, CStdString &stackedPath);
  };
}
