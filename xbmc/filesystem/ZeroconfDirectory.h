/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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

#pragma once
#include "IDirectory.h"
//txt-records as of http://www.dns-sd.org/ServiceTypes.html
#define TXT_RECORD_PATH_KEY     "path"
#define TXT_RECORD_USERNAME_KEY "u"
#define TXT_RECORD_PASSWORD_KEY "p"

namespace XFILE
{
  class CZeroconfDirectory : public IDirectory
  {
    public:
      CZeroconfDirectory(void);
      ~CZeroconfDirectory(void) override;
      bool GetDirectory(const CURL& url, CFileItemList &items) override;
      DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_NEVER; };
  };
}

