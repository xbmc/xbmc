/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSource.h"

#include "URL.h"
#include "Util.h"
#include "filesystem/MultiPathDirectory.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;

bool CMediaSource::IsWritable() const
{
  return CUtil::SupportsWriteFileOperations(strPath);
}

void CMediaSource::FromNameAndPaths(const std::string &category, const std::string &name, const std::vector<std::string> &paths)
{
  vecPaths = paths;
  if (paths.empty())
  { // no paths - return
    strPath.clear();
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
  m_allowSharing = true;

  if (URIUtils::IsMultiPath(strPath))
    m_iDriveType = SOURCE_TYPE_VPATH;
  else if (StringUtils::StartsWithNoCase(strPath, "udf:"))
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
      if (StringUtils::EqualsNoCase(sources[j].strPath, extras[i].strPath))
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
    if (StringUtils::EqualsNoCase(sources[i].strPath, source.strPath))
    {
      sources[i] = source;
      break;
    }
  }
  if (i == sources.size())
    sources.push_back(source);
}
