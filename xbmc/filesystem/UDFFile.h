/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"

typedef struct udf_s udf_t;
typedef struct udf_dirent_s udf_dirent_t;

namespace XFILE
{

class CUDFFile : public IFile
{
public:
  CUDFFile() = default;
  ~CUDFFile() override = default;

  bool Open(const CURL& url) override;
  void Close() override;

  int Stat(const CURL& url, struct __stat64* buffer) override;

  ssize_t Read(void* buffer, size_t size) override;
  int64_t Seek(int64_t filePosition, int whence) override;

  int64_t GetLength() override;
  int64_t GetPosition() override;

  bool Exists(const CURL& url) override;

  int GetChunkSize() override;

private:
  udf_t* m_udf{nullptr};
  udf_dirent_t* m_path{nullptr};

  uint32_t m_current;
};

} // namespace XFILE
