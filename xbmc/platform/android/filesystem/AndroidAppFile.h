/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"
#include "filesystem/IFile.h"

#include <string.h>

namespace XFILE
{
class CFileAndroidApp : public IFile
{
public:
  /*! \brief Currently only used for retrieving App Icons. */
  CFileAndroidApp(void);
  ~CFileAndroidApp() override;
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;

  /*! \brief Return 32bit rgba raw bitmap. */
  ssize_t Read(void* lpBuf, size_t uiBufSize) override { return 0; }
  void Close() override;
  int64_t GetLength() override { return 0; }
  int64_t Seek(int64_t, int) override { return -1; }
  int64_t GetPosition() override { return 0; }
  int GetChunkSize() override;
  int IoControl(IOControl request, void* param) override;

  virtual unsigned int ReadIcon(unsigned char **lpBuf, unsigned int* width, unsigned int* height);

protected:
  bool IsValidFile(const CURL& url);

private:
  CURL              m_url;
  std::string       m_packageName;
  std::string       m_packageLabel;
  int               m_icon;
  int               m_iconWidth;
  int               m_iconHeight;
};
}
