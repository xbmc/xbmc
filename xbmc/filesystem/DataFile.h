/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"

#include <cstdint>
#include <string>

namespace XFILE
{

/*!
 * \brief Read-only, memory-backed implementation of RFC 2397 data URLs
 *
 * Supports \c data:[<mediatype>][;base64],<data>. An omitted media type has
 * the RFC default \c text/plain;charset=US-ASCII. Kodi limits the encoded
 * scheme-specific part to 128 MiB and the materialized data to 64 MiB.
 * Open() materializes the data; Exists() and Stat() validate it and calculate
 * its decoded size without allocating the decoded payload.
 *
 * \sa https://www.rfc-editor.org/rfc/rfc2397.html
 */
class CDataFile final : public IFile
{
public:
  // Implementation of IFile
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  int Stat(struct __stat64* buffer) override;
  ssize_t Read(void* buffer, size_t size) override;
  int64_t Seek(int64_t position, int whence = SEEK_SET) override;
  void Close() override;
  int64_t GetPosition() override;
  int64_t GetLength() override;

private:
  std::string m_data;
  int64_t m_position{0};
  bool m_isOpen{false};
};

} // namespace XFILE
