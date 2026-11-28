/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"

#include <string>
#include <vector>

namespace XFILE
{
/*! \brief A folder of this device that the user chose with the browser's folder picker
 *  (File System Access API).
 *
 * device:// lists the chosen folders and device://<id>/<path> is a path in one of them.
 * The browser keeps the access across sessions; access is read-only. Implemented in
 * platform/wasm/device_folder.js.
 */
struct DeviceFolder
{
  std::string path;
  std::string name;
};

class CDeviceFolderDirectory : public IDirectory
{
public:
  //! \return whether the browser has a folder picker
  static bool IsPickerAvailable();
  static std::vector<DeviceFolder> GetFolders();
  /*! \brief Opens the browser's folder picker and blocks until the user has chosen.
   *
   * Must be called while handling a user input and not from the GUI thread.
   */
  static bool PickFolder(DeviceFolder& folder);

  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Exists(const CURL& url) override;
  CacheType GetCacheType(const CURL& url) const override;
};

class CDeviceFolderFile : public IFile
{
public:
  ~CDeviceFolderFile() override;

  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  ssize_t Read(void* bufPtr, size_t bufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  void Close() override;
  int64_t GetPosition() override { return m_position; }
  int64_t GetLength() override { return m_length; }
  int IoControl(IOControl request, void* param) override;

private:
  std::string m_blobUrl;
  int64_t m_length{0};
  int64_t m_position{0};
};
} // namespace XFILE
