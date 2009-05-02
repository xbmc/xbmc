/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "MediaSource.h"
#include "Settings.h"
#include "Util.h"
#include "URL.h"
#include "FileSystem/MultiPathDirectory.h"

using namespace std;
using namespace DIRECTORY;

bool CMediaSource::isWritable() const
{
#ifdef _WIN32PC
  if(CUtil::IsDOSPath(strPath) && !CUtil::IsDVD(strPath))
#else
  if (strPath[1] == ':' && (strPath[0] != 'D' && strPath[0] != 'd'))
#endif
    return true; // local disk
  if (strPath.size() > 4)
  {
    if (strPath.substr(0,4) == "smb:")
      return true; // smb path
  }
#ifdef _LINUX
  if (strPath == getenv("HOME"))
    return true;
#endif

  return false;
}

void CMediaSource::FromNameAndPaths(const CStdString &category, const CStdString &name, const vector<CStdString> &paths)
{
  vecPaths = paths;
  if (paths.size() == 0)
  { // no paths - return
    strPath.Empty();
  }
  else if (paths.size() == 1)
  { // only one valid path? make it the strPath
    strPath = paths[0];
  }
  else
  { // multiple valid paths?
    if (g_advancedSettings.m_useMultipaths) // use new multipath:// protocol
      strPath = CMultiPathDirectory::ConstructMultiPath(vecPaths);
    else // use older virtualpath:// protocol
      strPath.Format("virtualpath://%s/%s/", category.c_str(), name.c_str());
  }

  strName = name;
  m_iLockMode = LOCK_MODE_EVERYONE;
  m_strLockCode = "0";
  m_iBadPwdCount = 0;
  m_iHasLock = 0;

  if (CUtil::IsVirtualPath(strPath) || CUtil::IsMultiPath(strPath))
    m_iDriveType = SOURCE_TYPE_VPATH;
  else if (strPath.Left(4).Equals("udf:"))
  {
    m_iDriveType = SOURCE_TYPE_VIRTUAL_DVD;
    strPath = "D:\\";
  }
  else if (CUtil::IsISO9660(strPath))
    m_iDriveType = SOURCE_TYPE_VIRTUAL_DVD;
  else if (CUtil::IsDVD(strPath))
    m_iDriveType = SOURCE_TYPE_DVD;
  else if (CUtil::IsRemote(strPath))
    m_iDriveType = SOURCE_TYPE_REMOTE;
  else if (CUtil::IsHD(strPath))
    m_iDriveType = SOURCE_TYPE_LOCAL;
  else
    m_iDriveType = SOURCE_TYPE_UNKNOWN;
  // check - convert to url and back again to make sure strPath is accurate
  // in terms of what we expect
  CUtil::AddSlashAtEnd(strPath);
  CURL url(strPath);
  url.GetURL(strPath);
}
