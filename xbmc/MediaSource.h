#pragma once

#include "StdString.h"
#include <vector>

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
    SOURCE_TYPE_VPATH        = 5
  };
  enum LockType
  {
    LOCK_MODE_UNKNOWN            = -1,
    LOCK_MODE_EVERYONE           =  0,
    LOCK_MODE_NUMERIC            =  1,
    LOCK_MODE_GAMEPAD            =  2,
    LOCK_MODE_QWERTY             =  3,
    LOCK_MODE_SAMBA              =  4,
    LOCK_MODE_EEPROM_PARENTAL    =  5
  };
  CMediaSource() { m_iDriveType=SOURCE_TYPE_UNKNOWN; m_iLockMode=CMediaSource::LOCK_MODE_EVERYONE; m_iBadPwdCount=0; m_iHasLock=0; m_ignore=false; };
  virtual ~CMediaSource() {};

  void FromNameAndPaths(const CStdString &category, const CStdString &name, const std::vector<CStdString> &paths);
  bool isWritable() const;
  CStdString strName; ///< Name of the share, can be choosen freely.
  CStdString strStatus; ///< Status of the share (eg has disk etc.)
  CStdString strPath; ///< Path of the share, eg. iso9660:// or F:

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
  int m_iLockMode;
  CStdString m_strLockCode;  ///< Input code for Lock UI to verify, can be chosen freely.
  int m_iHasLock;
  int m_iBadPwdCount; ///< Number of wrong passwords user has entered since share was last unlocked

  CStdString m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  std::vector<CStdString> vecPaths;
  bool m_ignore; /// <Do not store in xml
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
