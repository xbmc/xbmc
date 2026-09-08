/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IStorageProvider.h"
#include "MediaSource.h" // for std::vector<CMediaSource>
#include "jobs/IJobCallback.h"
#include "storage/discs/IDiscDriveHandler.h"
#include "threads/CriticalSection.h"
#include "utils/DiscsUtils.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

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
  enum class HasBlurayPlaylist : uint8_t
  {
    YES,
    NO,
    UNKNOWN
  };

  CMediaManager();

  void Initialize();
  void Stop();

  void LoadSources();
  bool SaveSources();

  void GetLocalDrives(std::vector<CMediaSource>& localDrives, bool includeQ = true);
  void GetRemovableDrives(std::vector<CMediaSource>& removableDrives);
  void GetNetworkLocations(std::vector<CMediaSource>& locations, bool autolocations = true);

  bool AddNetworkLocation(const std::string &path);
  bool HasLocation(const std::string& path) const;
  bool RemoveLocation(const std::string& path);
  bool SetLocationPath(const std::string& oldPath, const std::string& newPath);

  void AddAutoSource(const CMediaSource &share, bool bAutorun=false);
  void RemoveAutoSource(const CMediaSource &share);

#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  /*! \brief Build and register an optical-disc auto source for the given device path.
   * Adds the source WITHOUT triggering autorun.
   * \param devicePath The optical drive path (e.g. "D:")
   */
  void AddOpticalSource(const std::string& devicePath);
#endif

  bool IsDiscInDrive(const std::string& devicePath="");
  /*! \brief Whether the disc in a drive is an audio CD
   * \param allowCachedFailure Allow GUI polling to reuse a recent failed TOC read on Windows.
   *        Explicit actions such as ripping must leave this false so they read the disc again.
   */
  bool IsAudio(const std::string& devicePath = "", bool allowCachedFailure = false);
  bool HasOpticalDrive();
  std::string TranslateDevicePath(const std::string& devicePath, bool bReturnAsDevice=false);
  DriveState GetDriveStatus(const std::string& devicePath = "");

  /*! \brief Discard everything cached about a drive, so the next query asks the hardware again
   * \param devicePath The optical drive path, empty for every drive
   */
  void ResetDriveCaches(const std::string& devicePath = "");
#ifdef HAS_OPTICAL_DRIVE
  /*! \brief Get the disc TOC, reusing successful reads.
   * \param allowCachedFailure Allow GUI polling to reuse a recent failed read on Windows.
   */
  MEDIA_DETECT::CCdInfo* GetCdInfo(const std::string& devicePath = "",
                                   bool allowCachedFailure = false);
  bool RemoveCdInfo(const std::string& devicePath = "");
  std::string GetDiskLabel(const std::string& devicePath = "");
  std::string GetDiskUniqueId(const std::string& devicePath="");
  bool HasMediaBlurayPlaylist(const std::string& devicePath = "");

  /*! \brief Reset flag for removable bluray playlist status
   * This is needed as HasMediaBlurayPlaylist() is called every screen refresh when
   * the disc node is highlighted.
   * It needs to be reset whenever a disc is ejected or played (as a playlist may have been selected).
  */
  void ResetBlurayPlaylistStatus();

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

  UTILS::DISCS::DiscInfo GetDiscInfo(const std::string& mediaPath);

protected:
  std::vector<CNetworkLocation> m_locations;

  CCriticalSection m_muAutoSource, m_CritSecStorageProvider;
#ifdef HAS_OPTICAL_DRIVE
  std::map<std::string,MEDIA_DETECT::CCdInfo*> m_mapCdInfo;
#endif
  bool m_bOpticalDrivePresent;
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

  void RemoveDiscInfo(const std::string& devicePath);

  bool IsOpticalDrivePresent();

  struct DiscInfoCacheEntry
  {
    /*! What the disc reported about itself, exactly as GetDiscInfo() returned it */
    UTILS::DISCS::DiscInfo info;
    /*! The name to show for the disc - info.name, or the volume label if the disc gave none */
    std::string label;
    /*! When the entry must be read again. max() for a disc that did identify itself */
    std::chrono::steady_clock::time_point expires{std::chrono::steady_clock::time_point::max()};
  };

  /*! \brief Read the disc in a drive, answering from the cache whenever it can
   * Reading a disc is slow - it can spin the drive up and, for a Blu-ray, load libaacs - so
   * everything that needs to identify a disc shares the one read.
   * \param mediaPath The drive holding the disc (eg. "D:")
   * \return What the disc reported, and the name to show for it
   */
  DiscInfoCacheEntry GetCachedDiscInfo(const std::string& mediaPath);
  /*! Disc identity per drive, read from the disc itself - see GetDiskLabel */
  std::map<std::string, DiscInfoCacheEntry> m_mapDiscInfo;
  uint64_t m_discInfoGeneration{0};
  CCriticalSection m_discInfoSection;
#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  friend class TestMediaManager;

  /*! Reads the TOC of the disc in a drive. Replaceable so the cache can be tested without one */
  std::function<std::unique_ptr<MEDIA_DETECT::CCdInfo>(const std::string& devicePath)> m_readToc;
  std::unique_ptr<MEDIA_DETECT::CCdInfo> ReadToc(const std::string& devicePath);
  MEDIA_DETECT::CCdInfo* CacheCdInfo(const std::string& devicePath,
                                     std::unique_ptr<MEDIA_DETECT::CCdInfo> info,
                                     uint64_t generation);
  void CacheDiscInfo(const std::string& mediaPath, DiscInfoCacheEntry& entry, uint64_t generation);

  struct DriveStatusCacheEntry
  {
    DriveState state{DriveState::NOT_READY};
    /*! When the entry must be re-probed. Soon for a state that may recover on its own, later for
        a stable one, in case the event that should have announced a change never arrived */
    std::chrono::steady_clock::time_point expires{std::chrono::steady_clock::time_point::max()};
  };
  /*! Last known state of each optical drive, to keep the GUI off the hardware - see GetDriveStatus */
  std::map<std::string, DriveStatusCacheEntry> m_driveStatusCache;
  /*! Last state logged per drive. Deliberately survives ResetDriveCaches() so a re-probe that
      lands on the same state stays quiet - only transitions are logged */
  std::map<std::string, DriveState> m_driveStatusLogged;
  /*! Bumped by every ResetDriveCaches() so a probe that was invalidated while it was in
      flight can discard its now stale result instead of caching it - see GetDriveStatus */
  uint64_t m_driveStatusGeneration{0};
  CCriticalSection m_driveStatusSection;
  /*! Drives whose disc yielded no CdInfo, and when to try reading it again. Guarded by
      m_muAutoSource like m_mapCdInfo - see GetCdInfo */
  std::map<std::string, std::chrono::steady_clock::time_point> m_cdInfoUnavailable;
  uint64_t m_cdInfoGeneration{0};
#endif
#ifdef HAVE_LIBBLURAY
  HasBlurayPlaylist m_hasBlurayPlaylist{HasBlurayPlaylist::UNKNOWN};
#endif
};
