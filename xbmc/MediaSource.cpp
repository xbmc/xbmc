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

#include "MediaSource.h"
#include "settings/AdvancedSettings.h"
#include "Util.h"
#include "URL.h"
#include "filesystem/MultiPathDirectory.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace XFILE;

bool CMediaSource::IsWritable() const
{
  return CUtil::SupportsWriteFileOperations(strPath);
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
    strPath = CMultiPathDirectory::ConstructMultiPath(vecPaths);
  }

  strName = name;
  m_iLockMode = LOCK_MODE_EVERYONE;
  m_strLockCode = "0";
  m_iBadPwdCount = 0;
  m_iHasLock = 0;

  if (URIUtils::IsMultiPath(strPath))
    m_iDriveType = SOURCE_TYPE_VPATH;
  else if (strPath.Left(4).Equals("udf:"))
  {
    m_iDriveType = SOURCE_TYPE_VIRTUAL_DVD;
    strPath = "D:\\";
  }
  else if (URIUtils::IsISO9660(strPath))
    m_iDriveType = SOURCE_TYPE_VIRTUAL_DVD;
  else if (URIUtils::IsDVD(strPath))
    m_iDriveType = SOURCE_TYPE_DVD;
  else if (URIUtils::IsRemote(strPath))
    m_iDriveType = SOURCE_TYPE_REMOTE;
  else if (URIUtils::IsHD(strPath))
    m_iDriveType = SOURCE_TYPE_LOCAL;
  else
    m_iDriveType = SOURCE_TYPE_UNKNOWN;
  // check - convert to url and back again to make sure strPath is accurate
  // in terms of what we expect
  URIUtils::AddSlashAtEnd(strPath);
  strPath = CURL(strPath).Get();
}

bool CMediaSource::operator==(const CMediaSource &share) const
{
  // NOTE: we may wish to filter this through CURL to enable better "fuzzy" matching
  if (strPath != share.strPath)
    return false;
  if (strName != share.strName)
    return false;
  return true;
}

void AddOrReplace(VECSOURCES& sources, const VECSOURCES& extras)
{
  unsigned int i;
  for( i=0;i<extras.size();++i )
  {
    unsigned int j;
    for ( j=0;j<sources.size();++j)
    {
      if (sources[j].strPath.Equals(extras[i].strPath))
      {
        sources[j] = extras[i];
        break;
      }
    }
    if (j == sources.size())
      sources.push_back(extras[i]);
  }
}

void AddOrReplace(VECSOURCES& sources, const CMediaSource& source)
{
  unsigned int i;
  for( i=0;i<sources.size();++i )
  {
    if (sources[i].strPath.Equals(source.strPath))
    {
      sources[i] = source;
      break;
    }
  }
  if (i == sources.size())
    sources.push_back(source);
}
