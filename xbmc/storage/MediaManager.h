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
  MEDIA_DETECT::CCdInfo* GetCdInfo(const std::string& devicePath="");
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

  struct DiscInfo
  {
    std::string name;
    std::string serial;
    std::string type;

    bool empty()
    {
      return (name.empty() && serial.empty());
    }
  };

  DiscInfo GetDiscInfo(const std::string& mediaPath);
  void RemoveDiscInfo(const std::string& devicePath);
  std::map<std::string, DiscInfo> m_mapDiscInfo;
};
