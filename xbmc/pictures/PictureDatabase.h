/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include "dbwrappers/DynamicDatabase.h"

#include <string>
#include <set>

class CPictureInfoTag;
class CFileItem;
class CGUIDialogProgress;

class CPictureDatabase : public CDynamicDatabase
{
public:
  CPictureDatabase();
  virtual ~CPictureDatabase() { }

  virtual bool Open();

  bool HasPictures() { return Count() != 0; }

  bool GetPaths(std::set<std::string> &paths);
  bool GetPathHash(const std::string &strDirectory, std::string &dbHash);
  bool SetPathHash(const std::string &strDirectory, const std::string &dbHash);
  bool HasPath(const std::string &strPath);

  bool GetPicturesByPath(const std::string &path, std::vector<CPictureInfoTag> &pictures);
  bool DeletePicturesByPath(const std::string &path, bool recurseive, CGUIDialogProgress* pDialogProgress = NULL);

protected:
  virtual int GetMinVersion() const { return 2; }
  virtual const char *GetBaseDBName() const { return "MyPhotos"; }
  
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

  /*!
   * Uniqueness is quantified by file name and path.
   * @throw dbiplus::DbErrors
   */
  virtual bool Exists(const bson *object, int &idObject);
  virtual bool IsValid(const bson *object) const;

  virtual CFileItem *CreateFileItem(const std::string &strBson, int id) const;
};
