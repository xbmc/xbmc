#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>
#include "GUIPassword.h"

/*!
\ingroup windows
\brief Represents a share.
\sa VECMediaSource, IVECSOURCES
*/
class CMediaSource
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
  CMediaSource() { m_iDriveType=SOURCE_TYPE_UNKNOWN; m_iLockMode=LOCK_MODE_EVERYONE; m_iBadPwdCount=0; m_iHasLock=0; m_ignore=false; m_allowSharing=true; };
  virtual ~CMediaSource() {};

  bool operator==(const CMediaSource &right) const;

  void FromNameAndPaths(const std::string &category, const std::string &name, const std::vector<std::string> &paths);
  bool IsWritable() const;
  std::string strName; ///< Name of the share, can be choosen freely.
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
  SourceType m_iDriveType;

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
  LockType m_iLockMode;
  std::string m_strLockCode;  ///< Input code for Lock UI to verify, can be chosen freely.
  int m_iHasLock;
  int m_iBadPwdCount; ///< Number of wrong passwords user has entered since share was last unlocked

  std::string m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  std::vector<std::string> vecPaths;
  bool m_ignore; /// <Do not store in xml
  bool m_allowSharing; /// <Allow browsing of source from UPnP / WebServer
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
