/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/wasm/DeviceFolder.h"
#include "storage/IStorageProvider.h"
#include "threads/CriticalSection.h"

#include <string>
#include <vector>

class CWasmStorageProvider : public IStorageProvider
{
public:
  CWasmStorageProvider() = default;
  ~CWasmStorageProvider() override = default;

  void Initialize() override;
  void Stop() override {}

  void GetLocalDrives(std::vector<CMediaSource>& localDrives) override;
  void GetRemovableDrives(std::vector<CMediaSource>& removableDrives) override;

  bool Eject(const std::string& mountpath) override { return false; }

  std::vector<std::string> GetDiskUsage() override;

  bool PumpDriveChangeEvents(IStorageEventsCallback* callback) override;

private:
  // Asked for often, and listing the storages is a round trip to the browser thread
  std::vector<XFILE::DeviceFolder> m_storages;
  CCriticalSection m_storagesLock;
};
