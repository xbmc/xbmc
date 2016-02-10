#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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
#include <string>
#include "FileItem.h"

class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item, bool force=false);
  static bool DeleteItem(const std::string &strPath, bool force=false);
  static bool RenameFile(const std::string &strFile);
  static bool RemoteAccessAllowed(const std::string &strPath);
  static unsigned int LoadFile(const std::string &filename, void* &outputBuffer);
  /*! \brief Get the modified date of a file if its invalid it returns the creation date - this behavior changes when you set bUseLatestDate
  \param strFileNameAndPath path to the file
  \param bUseLatestDate use the newer datetime of the files mtime and ctime
  \return Returns the file date, can return a invalid date if problems occure
  */
  static CDateTime GetModificationDate(const std::string& strFileNameAndPath, const bool& bUseLatestDate);
  static bool ZebraListAccessCheck(const std::string &filePath);
};
