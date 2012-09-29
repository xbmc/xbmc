#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "IDirectory.h"
#include "SmbFile.h"
#include "MediaSource.h"

namespace XFILE
{
class CSMBDirectory : public IDirectory
{
public:
  CSMBDirectory(void);
  virtual ~CSMBDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString &strPath) const { return DIR_CACHE_ONCE; };
  virtual bool Create(const char* strPath);
  virtual bool Exists(const char* strPath);
  virtual bool Remove(const char* strPath);

  int Open(const CURL &url);

  //MountShare will try to mount the smb share and return the path to the mount point (or empty string if failed)
  static CStdString MountShare(const CStdString &smbPath, const CStdString &strType, const CStdString &strName,
    const CStdString &strUser, const CStdString &strPass);

  static void UnMountShare(const CStdString &strType, const CStdString &strName);
  static CStdString GetMountPoint(const CStdString &strType, const CStdString &strName);

  static bool MountShare(const CStdString &strType, CMediaSource &share);

private:
  int OpenDir(const CURL &url, CStdString& strAuth);
};
}
