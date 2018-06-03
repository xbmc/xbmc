#pragma once
/*
 *      Copyright (C) 2014-present Team Kodi
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

#include "filesystem/IDirectory.h"

namespace XFILE
{

class CPosixDirectory : public IDirectory
{
public:
  CPosixDirectory(void);
  ~CPosixDirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList &items) override;
  bool Create(const CURL& url) override;
  bool Exists(const CURL& url) override;
  bool Remove(const CURL& url) override;
  bool RemoveRecursive(const CURL& url) override;
private:
  bool Create(std::string path);
};
}
