/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"

#include <memory>

#include <cdio++/iso9660.hpp>

namespace XFILE
{

class CISO9660File : public IFile
{
public:
  CISO9660File();
  ~CISO9660File() override = default;

  bool Open(const CURL& url) override;
  void Close() override {}

  int Stat(const CURL& url, struct __stat64* buffer) override;

  ssize_t Read(void* buffer, size_t size) override;
  int64_t Seek(int64_t filePosition, int whence) override;

  int64_t GetLength() override;
  int64_t GetPosition() override;

  bool Exists(const CURL& url) override;

  int GetChunkSize() override;

private:
  std::unique_ptr<ISO9660::IFS> m_iso;
  std::unique_ptr<ISO9660::Stat> m_stat;

  int32_t m_start;
  int32_t m_current;
};

} // namespace XFILE
