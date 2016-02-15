/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once
#include <string>
#include <vector>

class CInfoScanner
{
public:
  virtual ~CInfoScanner();
  virtual bool DoScan(const std::string& strDirectory) = 0;
  /*! \brief Check if the folder is excluded from scanning process
   \param strDirectory Directory to scan
   \param regexps Regular expression to exclude from the scan
   \return true if there is a .nomedia file or one of the regexps is a match
   */
  bool IsExcluded(const std::string& strDirectory, const std::vector<std::string> &regexps);
private:
  bool HasNoMedia(const std::string& strDirectory) const;
};
