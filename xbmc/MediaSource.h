/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LockMode.h"
#include "SourceType.h"
#include "media/MediaLockState.h"

#include <string>
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

  void FromNameAndPaths(const std::string &category, const std::string &name, const std::vector<std::string> &paths);
  bool IsWritable() const;
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

  /*!
  \brief The type of Lock UI to show when accessing the media source.

  Value can be:
  - LockMode::EVERYONE \n
  Default value.  No lock UI is shown, user can freely access the source.
  - LockMode::NUMERIC \n
  Lock code is entered via OSD numpad or IrDA remote buttons.
  - LockMode::GAMEPAD \n
  Lock code is entered via XBOX gamepad buttons.
  - LockMode::QWERTY \n
  Lock code is entered via OSD keyboard or PC USB keyboard.
  - LockMode::SAMBA \n
  Lock code is entered via OSD keyboard or PC USB keyboard and passed directly to SMB for authentication.
  - LockMode::EEPROM_PARENTAL \n
  Lock code is retrieved from XBOX EEPROM and entered via XBOX gamepad or remote.
  - LockMode::UNKNOWN \n
  Value is unknown or unspecified.
  */
  LockMode m_iLockMode = LockMode::EVERYONE;
  std::string m_strLockCode;  ///< Input code for Lock UI to verify, can be chosen freely.
  int m_iHasLock = LOCK_STATE_NO_LOCK;
  int m_iBadPwdCount = 0; ///< Number of wrong passwords user has entered since share was last unlocked

  std::string m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  std::vector<std::string> vecPaths;
  bool m_ignore = false; /// <Do not store in xml
  bool m_allowSharing = true; /// <Allow browsing of source from UPnP / WebServer
};

void AddOrReplace(std::vector<CMediaSource>& sources, const std::vector<CMediaSource>& extras);
void AddOrReplace(std::vector<CMediaSource>& sources, const CMediaSource& source);
