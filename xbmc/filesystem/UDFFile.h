/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"
#include "filesystem/UDFBlockInput.h"

#include <memory>

class udfread;
typedef struct udfread_file UDFFILE;

namespace XFILE
{

class CUDFFile : public IFile
{
public:
  CUDFFile();
  ~CUDFFile() override = default;

  bool Open(const CURL& url) override;
  void Close() override;

  int Stat(const CURL& url, struct __stat64* buffer) override;

  ssize_t Read(void* buffer, size_t size) override;
  int64_t Seek(int64_t filePosition, int whence) override;

  int64_t GetLength() override;
  int64_t GetPosition() override;

  bool Exists(const CURL& url) override;

private:
  std::unique_ptr<CUDFBlockInput> m_bi{nullptr};

  udfread* m_udf{nullptr};
  UDFFILE* m_file{nullptr};
};

} // namespace XFILE
