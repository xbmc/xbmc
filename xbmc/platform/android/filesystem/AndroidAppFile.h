/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#if defined(TARGET_ANDROID)
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

#endif

