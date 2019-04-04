/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IFile.h"
#include "URL.h"
#include "string.h"

namespace XFILE
{
class CFileAndroidApp : public IFile
{
public:
  /*! \brief Currently only used for retrieving App Icons. */
  CFileAndroidApp(void);
  virtual ~CFileAndroidApp(void);
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);

  /*! \brief Return 32bit rgba raw bitmap. */
  virtual ssize_t Read(void* lpBuf, size_t uiBufSize) {return 0;}
  virtual void Close();
  virtual int64_t GetLength()  {return 0;}
  virtual int64_t Seek(int64_t, int) {return -1;}
  virtual int64_t GetPosition() {return 0;}
  virtual int GetChunkSize();
  virtual int IoControl(EIoControl request, void* param);

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
