#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "IDirectory.h"

class CPVRSession;

namespace XFILE {

class CPVRDirectory
  : public IDirectory
{
public:
  CPVRDirectory();
  ~CPVRDirectory() override;

  bool GetDirectory(const CURL& url, CFileItemList &items) override;
  bool AllowAll() const override { return true; }
  DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_NEVER; };
  bool Exists(const CURL& url) override;

  static bool SupportsWriteFileOperations(const std::string& strPath);
  static bool IsLiveTV(const std::string& strPath);
  static bool HasTVRecordings();
  static bool HasDeletedTVRecordings();
  static bool HasRadioRecordings();
  static bool HasDeletedRadioRecordings();
};

}
