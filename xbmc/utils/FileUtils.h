#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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
#include <string>
#include "FileItem.h"

class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item);
  static bool DeleteItem(const std::string &strPath);
  static bool RenameFile(const std::string &strFile);
  static bool RemoteAccessAllowed(const std::string &strPath);
  static unsigned int LoadFile(const std::string &filename, void* &outputBuffer);
  /*! \brief Get the modified date of a file if its invalid it returns the creation date - this behavior changes when you set bUseLatestDate
  \param strFileNameAndPath path to the file
  \param bUseLatestDate use the newer datetime of the files mtime and ctime
  \return Returns the file date, can return a invalid date if problems occur
  */
  static CDateTime GetModificationDate(const std::string& strFileNameAndPath, const bool& bUseLatestDate);
};
