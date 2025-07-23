/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SourceType.h"
#include "utils/LockInfo.h"

#include <string>
#include <string_view>
#include <vector>

/*!
\ingroup windows
\brief Represents a share.
\sa VECMediaSource, std::vector<CMediaSource>::iterator
*/
class CMediaSource final
{
public:
  bool operator==(const CMediaSource &right) const;

  void FromNameAndPaths(std::string_view name, const std::vector<std::string>& paths);
  bool IsWritable() const;

  KODI::UTILS::CLockInfo& GetLockInfo() { return m_lockInfo; }
  const KODI::UTILS::CLockInfo& GetLockInfo() const { return m_lockInfo; }

  std::string strName; ///< Name of the share, can be chosen freely.
  std::string strStatus; ///< Status of the share (eg has disk etc.)
  std::string strDiskUniqueId; ///< removable:// + DVD Label + DVD ID for resume point storage, if available
  std::string strPath; ///< Path of the share, eg. iso9660:// or F:

  /*!
  \brief The type of the media source.

  Value can be:
  - SOURCE_TYPE_UNKNOWN \n
  Unknown source, maybe a wrong path.
  - SOURCE_TYPE_LOCAL \n
  Harddisk source.
  - SOURCE_TYPE_OPTICAL_DISC \n
  DVD-ROM source of the build in drive, strPath may vary.
  - SOURCE_TYPE_VIRTUAL_OPTICAL_DISC \n
  DVD-ROM source, strPath is fix.
  - SOURCE_TYPE_REMOTE \n
  Network source.
  */
  SourceType m_iDriveType = SourceType::UNKNOWN;

  KODI::UTILS::CLockInfo m_lockInfo;

  std::string
      m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  std::vector<std::string> vecPaths;
  bool m_ignore = false; /// <Do not store in xml
  bool m_allowSharing = true; /// <Allow browsing of source from UPnP / WebServer
};

void AddOrReplace(std::vector<CMediaSource>& sources, const std::vector<CMediaSource>& extras);
void AddOrReplace(std::vector<CMediaSource>& sources, const CMediaSource& source);
