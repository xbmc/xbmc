#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include "IFileDirectory.h"

namespace XFILE
{
  class CAPKDirectory : public IFileDirectory
  {
    public:
    CAPKDirectory() {};
    virtual ~CAPKDirectory() {};
      virtual bool GetDirectory(const  CStdString& strPath, CFileItemList &items);
      virtual bool ContainsFiles(const CStdString& strPath);
      virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;
      virtual bool Exists(const char* strPath);
  };
}
