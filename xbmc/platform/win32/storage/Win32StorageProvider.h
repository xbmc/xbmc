/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "storage/IStorageProvider.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <memory>
#include <vector>

#include <Cfgmgr32.h>

enum Drive_Types
{
  ALL_DRIVES = 0,
  LOCAL_DRIVES,
  REMOVABLE_DRIVES,
  DVD_DRIVES
};

class CWin32StorageProvider : public IStorageProvider
{
public:
  virtual ~CWin32StorageProvider() { }

  virtual void Initialize();
  virtual void Stop() { }

  virtual void ScanForPresentMedia();
  virtual void StopScanForPresentMedia();

  // Reads the discs the drives hold at startup. Runs on m_presentMediaScanner, and so touches
  // nothing of this object's own.
  static void ScanDrivesForPresentMedia();

  virtual void GetLocalDrives(std::vector<CMediaSource>& localDrives);
  virtual void GetRemovableDrives(std::vector<CMediaSource>& removableDrives);
  virtual std::string GetFirstOpticalDeviceFileName();

  virtual bool Eject(const std::string& mountpath);

  virtual std::vector<std::string> GetDiskUsage();

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback* callback);

  // Translated from Windows messages
  enum class StorageEventType
  {
    ADDED,
    SAFELY_REMOVED,
    UNSAFELY_REMOVED,
    //! A disc that was already in the drive when Kodi started
    PRESENT_AT_STARTUP,
  };

  // Build a StorageDevice descriptor from a drive root (eg. "D:")
  static MEDIA_DETECT::STORAGE::StorageDevice GetStorageDevice(const std::string& drive);

  // Called from the windowing thread to queue a change for the next pump
  static void QueueStorageEvent(StorageEventType type,
                                const MEDIA_DETECT::STORAGE::StorageDevice& device);

  static void SetEvent() { xbevent = true; }
  static bool xbevent;

private:
  static void GetDrivesByType(std::vector<CMediaSource>& localDrives,
                              Drive_Types eDriveType = ALL_DRIVES,
                              bool bonlywithmedia = false);
  static DEVINST GetDrivesDevInstByDiskNumber(long DiskNumber);

  struct StorageEvent
  {
    StorageEventType type;
    MEDIA_DETECT::STORAGE::StorageDevice device;
  };

  inline static std::vector<StorageEvent> m_events;
  inline static CCriticalSection m_eventsSection;

  std::unique_ptr<CThread> m_presentMediaScanner;
};
