/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#pragma once
#if defined(TARGET_ANDROID)
#include "IDirectory.h"
#include "FileItem.h"
namespace XFILE
{

class CAndroidAppDirectory :
      public IDirectory
{
public:
  CAndroidAppDirectory(void);
  virtual ~CAndroidAppDirectory(void);
  virtual bool GetDirectory(const CURL& url, CFileItemList &items);
  virtual bool Exists(const CURL& url) { return true; };
  virtual bool AllowAll() const { return true; };
  virtual DIR_CACHE_TYPE GetCacheType(const std::string& strPath) const { return DIR_CACHE_NEVER; }
};
}
#endif

