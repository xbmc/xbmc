/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSource.h"

#include "URL.h"
#include "Util.h"
#include "filesystem/MultiPathDirectory.h"
#include "media/MediaLockState.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>

using namespace XFILE;

bool CMediaSource::IsWritable() const
{
  return CUtil::SupportsWriteFileOperations(strPath);
}

void CMediaSource::FromNameAndPaths(std::string_view name, const std::vector<std::string>& paths)
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
  m_lockInfo = {};
  m_allowSharing = true;

  if (URIUtils::IsMultiPath(strPath))
    m_iDriveType = SourceType::VPATH;
  else if (StringUtils::StartsWithNoCase(strPath, "udf:"))
  {
    m_iDriveType = SourceType::VIRTUAL_OPTICAL_DISC;
    strPath = "D:\\";
  }
  else if (URIUtils::IsISO9660(strPath))
    m_iDriveType = SourceType::VIRTUAL_OPTICAL_DISC;
  else if (URIUtils::IsDVD(strPath))
    m_iDriveType = SourceType::OPTICAL_DISC;
  else if (URIUtils::IsRemote(strPath))
    m_iDriveType = SourceType::REMOTE;
  else if (URIUtils::IsHD(strPath))
    m_iDriveType = SourceType::LOCAL;
  else
    m_iDriveType = SourceType::UNKNOWN;

  // check - convert to url and back again to make sure strPath is accurate
  // in terms of what we expect
  strPath = CURL(strPath).Get();
}

bool CMediaSource::operator==(const CMediaSource &share) const
{
  // NOTE: we may wish to filter this through CURL to enable better "fuzzy" matching
  return strPath == share.strPath && strName == share.strName;
}

void AddOrReplace(std::vector<CMediaSource>& sources, const std::vector<CMediaSource>& extras)
{
  std::ranges::for_each(extras, [&sources](auto& extra) { AddOrReplace(sources, extra); });
}

void AddOrReplace(std::vector<CMediaSource>& sources, const CMediaSource& source)
{
  auto it = std::ranges::find_if(sources, [&path = source.strPath](const auto& src)
                                 { return StringUtils::EqualsNoCase(src.strPath, path); });
  if (it != sources.end())
    *it = source;
  else
    sources.push_back(source);
}
