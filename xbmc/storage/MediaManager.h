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
#include "storage/discs/IDiscDriveHandler.h"
#include "threads/CriticalSection.h"
#include "utils/DiscsUtils.h"
#include "utils/Job.h"

#include <map>
#include <memory>
#include <vector>

#include "PlatformDefs.h"

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
  DriveState GetDriveStatus(const std::string& devicePath = "");
#ifdef HAS_OPTICAL_DRIVE
  MEDIA_DETECT::CCdInfo* GetCdInfo(const std::string& devicePath="");
  bool RemoveCdInfo(const std::string& devicePath="");
  std::string GetDiskLabel(const std::string& devicePath="");
  std::string GetDiskUniqueId(const std::string& devicePath="");

  /*! \brief Gets the platform disc drive handler
  * @todo this likely doesn't belong here but in some discsupport component owned by media manager
  * let's keep it here for now
  * \return The platform disc drive handler
  */
  std::shared_ptr<IDiscDriveHandler> GetDiscDriveHandler();
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
#ifdef HAS_OPTICAL_DRIVE
  std::map<std::string,MEDIA_DETECT::CCdInfo*> m_mapCdInfo;
#endif
  bool m_bhasoptical;
  std::string m_strFirstAvailDrive;

private:
  /*! \brief Loads the addon sources for the different supported browsable addon types
   */
  void LoadAddonSources() const;

  /*! \brief Get the addons root source for the given content type
   \param type the type of addon content desired
   \return the given CMediaSource for the addon root directory
   */
  CMediaSource GetRootAddonTypeSource(const std::string& type) const;

  /*! \brief Generate the addons source for the given content type
   \param type the type of addon content desired
   \param label the name of the addons source
   \param thumb image to use as the icon
   \return the given CMediaSource for the addon root directory
   */
  CMediaSource ComputeRootAddonTypeSource(const std::string& type,
                                          const std::string& label,
                                          const std::string& thumb) const;

  std::unique_ptr<IStorageProvider> m_platformStorage;
#ifdef HAS_OPTICAL_DRIVE
  std::shared_ptr<IDiscDriveHandler> m_platformDiscDriveHander;
#endif

  UTILS::DISCS::DiscInfo GetDiscInfo(const std::string& mediaPath);
  void RemoveDiscInfo(const std::string& devicePath);
  std::map<std::string, UTILS::DISCS::DiscInfo> m_mapDiscInfo;
};
