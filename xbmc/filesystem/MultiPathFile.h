#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
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

#include "filesystem/OverrideFile.h"

namespace XFILE
{
  class CMultiPathFile : public COverrideFile
  {
  public:
    CMultiPathFile(void);
    ~CMultiPathFile(void) override;
    bool Open(const CURL& url) override;
    bool Exists(const CURL& url) override;
    int Stat(const CURL& url, struct __stat64* buffer) override;

  protected:
    std::string TranslatePath(const CURL &url) override;
  };
}
