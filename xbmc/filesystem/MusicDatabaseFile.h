/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "File.h"
#include "IFile.h"

namespace XFILE
{
class CMusicDatabaseFile : public IFile
{
public:
  CMusicDatabaseFile(void);
  ~CMusicDatabaseFile(void) override;
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;

  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  void Close() override;
  int64_t GetPosition() override;
  int64_t GetLength() override;

  static std::string TranslateUrl(const CURL& url);
protected:
  CFile m_file;
};
}
