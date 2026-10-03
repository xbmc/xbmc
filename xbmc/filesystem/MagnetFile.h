/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"
#include "XBDateTime.h"

#include <stdint.h>
#include <string>
#include <utility>

#include <libtorrent/libtorrent.hpp>

class CURL;

namespace XFILE
{

class CMagnetFile : public IFile
{
public:
  CMagnetFile() = default;
  ~CMagnetFile() override = default;

  // Implementation of IFile
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  int Stat(struct __stat64* buffer) override;
  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  int64_t GetPosition() override;
  int64_t GetLength() override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  void Close() override {}

private:
  // Private utility functions
  void OnFileAccess(size_t fileOffset, size_t partLength);
  void SetPiecePriority(int fileIndex, size_t fileOffset, size_t pieceSize, uint8_t priority);

  // libtorrent parameters
  lt::torrent_handle m_torrentHandle;

  // File parameters
  int m_fileIndex{-1};
  int64_t m_fileSize{-1};
  CDateTime m_modifiedTime;
  int64_t m_filePosition{-1};
};

} // namespace XFILE
