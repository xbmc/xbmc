/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IStorageProvider.h"
#include "MediaSource.h" // for VECSOURCES
#include "threads/CriticalSection.h"
#include "utils/DiscsUtils.h"
#include "utils/Job.h"

#include <map>
#include <vector>

#include "PlatformDefs.h"

#define TRAY_OPEN     16
#define TRAY_CLOSED_NO_MEDIA  64
#define TRAY_CLOSED_MEDIA_PRESENT 96

#define DRIVE_OPEN      0 // Open...
#define DRIVE_NOT_READY     1 // Opening.. Closing...
#define DRIVE_READY      2
#define DRIVE_CLOSED_NO_MEDIA   3 // CLOSED...but no media in drive
#define DRIVE_CLOSED_MEDIA_PRESENT  4 // Will be send once when the drive just have closed
#define DRIVE_NONE  5 // system doesn't have an optical drive


/*! \brief Wait modes for acessing MediaManager info */
enum class WaitMode
{
  WAIT, /*!< When this mode is specified the caller is blocked if some other component is trying to access the same info, and the info is returned */
  LAZY /*!< When this mode is specified the caller is not blocked when some other component is trying to access the same info and the info might not be returned  */
};

class CFileItem;

class CNetworkLocation
{
public:
  CNetworkLocation() { id = 0; }
  int id;
  std::string path;
};

class CMediaManager : public IStorageEventsCallback, public IJobCallback
{
public:
  CMediaManager();

  void Initialize();
  void Stop();

  bool LoadSources();
  bool SaveSources();

  void GetLocalDrives(VECSOURCES &localDrives, bool includeQ = true);
  void GetRemovableDrives(VECSOURCES &removableDrives);
  void GetNetworkLocations(VECSOURCES &locations, bool autolocations = true);

  bool AddNetworkLocation(const std::string &path);
  bool HasLocation(const std::string& path) const;
  bool RemoveLocation(const std::string& path);
  bool SetLocationPath(const std::string& oldPath, const std::string& newPath);

  void AddAutoSource(const CMediaSource &share, bool bAutorun=false);
  void RemoveAutoSource(const CMediaSource &share);
  bool IsDiscInDrive(const std::string& devicePath="");
  bool IsAudio(const std::string& devicePath="");
  bool HasOpticalDrive();
  std::string TranslateDevicePath(const std::string& devicePath, bool bReturnAsDevice=false);
  DWORD GetDriveStatus(const std::string& devicePath="");
#ifdef HAS_DVD_DRIVE
  /*! \brief Gets the CDInfo of the current inserted optical disc
    \param devicePath The disc mediapath (e.g. /dev/cdrom, D\://, etc), default empty
    \param waitMode Getting the cd info is a blocking operation. waitMode specifies if the
    caller must wait for the result (WaitMode::WAIT) or get the info in a lazy/best effort way (WaitMode::LAZY).
    If WaitMode::WAIT is specified nullptr will be returned if the drive is currently being accessed
    by some other resource.
    \return a pointer to the CCdInfo of nullptr if it doesn't exist
*/
  MEDIA_DETECT::CCdInfo* GetCdInfo(const std::string& devicePath = "",
                                   WaitMode waitMode = WaitMode::WAIT);
  bool RemoveCdInfo(const std::string& devicePath="");
  std::string GetDiskLabel(const std::string& devicePath="");
  std::string GetDiskUniqueId(const std::string& devicePath="");
#endif
  std::string GetDiscPath();
  void SetHasOpticalDrive(bool bstatus);

  bool Eject(const std::string& mountpath);
  void EjectTray( const bool bEject=true, const char cDriveLetter='\0' );
  void CloseTray(const char cDriveLetter='\0');
  void ToggleTray(const char cDriveLetter='\0');

  void ProcessEvents();

  std::vector<std::string> GetDiskUsage();

  /*! \brief Callback executed when a new storage device is added
    * \sa IStorageEventsCallback
    * @param device the storage device
  */
  void OnStorageAdded(const MEDIA_DETECT::STORAGE::StorageDevice& device) override;

  /*! \brief Callback executed when a new storage device is safely removed
    * \sa IStorageEventsCallback
    * @param device the storage device
  */
  void OnStorageSafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device) override;

  /*! \brief Callback executed when a new storage device is unsafely removed
    * \sa IStorageEventsCallback
    * @param device the storage device
  */
  void OnStorageUnsafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device) override;

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override { }

  bool playStubFile(const CFileItem& item);

protected:
  std::vector<CNetworkLocation> m_locations;

  CCriticalSection m_muAutoSource, m_CritSecStorageProvider;
#ifdef HAS_DVD_DRIVE
  std::map<std::string,MEDIA_DETECT::CCdInfo*> m_mapCdInfo;
#endif
  bool m_bhasoptical;
  std::string m_strFirstAvailDrive;

private:
  IStorageProvider *m_platformStorage;

  UTILS::DISCS::DiscInfo GetDiscInfo(const std::string& mediaPath);
  void RemoveDiscInfo(const std::string& devicePath);
  std::map<std::string, UTILS::DISCS::DiscInfo> m_mapDiscInfo;
};
