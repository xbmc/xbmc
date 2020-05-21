/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LockType.h"
#include "media/MediaLockState.h"

#include <string>
#include <vector>

/*!
\ingroup windows
\brief Represents a share.
\sa VECMediaSource, IVECSOURCES
*/
class CMediaSource final
{
public:
  enum SourceType
  {
    SOURCE_TYPE_UNKNOWN      = 0,
    SOURCE_TYPE_LOCAL        = 1,
    SOURCE_TYPE_DVD          = 2,
    SOURCE_TYPE_VIRTUAL_DVD  = 3,
    SOURCE_TYPE_REMOTE       = 4,
    SOURCE_TYPE_VPATH        = 5,
    SOURCE_TYPE_REMOVABLE    = 6
  };

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
  - SOURCE_TYPE_DVD \n
  DVD-ROM source of the build in drive, strPath may vary.
  - SOURCE_TYPE_VIRTUAL_DVD \n
  DVD-ROM source, strPath is fix.
  - SOURCE_TYPE_REMOTE \n
  Network source.
  */
  SourceType m_iDriveType = SOURCE_TYPE_UNKNOWN;

  /*!
  \brief The type of Lock UI to show when accessing the media source.

  Value can be:
  - CMediaSource::LOCK_MODE_EVERYONE \n
  Default value.  No lock UI is shown, user can freely access the source.
  - LOCK_MODE_NUMERIC \n
  Lock code is entered via OSD numpad or IrDA remote buttons.
  - LOCK_MODE_GAMEPAD \n
  Lock code is entered via XBOX gamepad buttons.
  - LOCK_MODE_QWERTY \n
  Lock code is entered via OSD keyboard or PC USB keyboard.
  - LOCK_MODE_SAMBA \n
  Lock code is entered via OSD keyboard or PC USB keyboard and passed directly to SMB for authentication.
  - LOCK_MODE_EEPROM_PARENTAL \n
  Lock code is retrieved from XBOX EEPROM and entered via XBOX gamepad or remote.
  - LOCK_MODE_UNKNOWN \n
  Value is unknown or unspecified.
  */
  LockType m_iLockMode = LOCK_MODE_EVERYONE;
  std::string m_strLockCode;  ///< Input code for Lock UI to verify, can be chosen freely.
  int m_iHasLock = LOCK_STATE_NO_LOCK;
  int m_iBadPwdCount = 0; ///< Number of wrong passwords user has entered since share was last unlocked

  std::string m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  std::vector<std::string> vecPaths;
  bool m_ignore = false; /// <Do not store in xml
  bool m_allowSharing = true; /// <Allow browsing of source from UPnP / WebServer
};

/*!
\ingroup windows
\brief A vector to hold CMediaSource objects.
\sa CMediaSource, IVECSOURCES
*/
typedef std::vector<CMediaSource> VECSOURCES;

/*!
\ingroup windows
\brief Iterator of VECSOURCES.
\sa CMediaSource, VECSOURCES
*/
typedef std::vector<CMediaSource>::iterator IVECSOURCES;
typedef std::vector<CMediaSource>::const_iterator CIVECSOURCES;

void AddOrReplace(VECSOURCES& sources, const VECSOURCES& extras);
void AddOrReplace(VECSOURCES& sources, const CMediaSource& source);
