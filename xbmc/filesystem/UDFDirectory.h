#ifndef UDF_DIRECTORY_H
#define UDF_DIRECTORY_H
/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
 *
 *      Copyright (C) 2010-present Team Kodi
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
#include "IFileDirectory.h"

namespace XFILE
{
class CUDFDirectory :
      public IFileDirectory
{
public:
  CUDFDirectory(void);
  ~CUDFDirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList &items) override;
  bool Exists(const CURL& url) override;
  bool ContainsFiles(const CURL& url) override { return true; }
};
}

#endif
