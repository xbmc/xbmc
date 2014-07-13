#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include "MediaSource.h"
#include "URL.h"

struct afp_file_info;

namespace XFILE
{
class CAFPDirectory : public IDirectory
{
public:
  CAFPDirectory(void);
  virtual ~CAFPDirectory(void);
  virtual bool GetDirectory(const CURL& url, CFileItemList &items);
  virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const { return DIR_CACHE_ONCE; };
  virtual bool Create(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual bool Remove(const CURL& url);

  afp_file_info *Open(const CURL &url);
private:
  afp_file_info *OpenDir(const CURL &url, std::string& strAuth);
  bool ResolveSymlink( const std::string &dirName, const std::string &fileName,
                       struct stat *stat, CURL &resolvedUrl);
};
}
