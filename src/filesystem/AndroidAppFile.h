#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "system.h"

#if defined(TARGET_ANDROID)
#include "IFile.h"
#include "URL.h"
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
  virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
  virtual void Close();
  virtual int64_t GetLength();
  virtual int64_t Seek(int64_t, int) {return -1;};
  virtual int64_t GetPosition() {return 0;};
  virtual int GetChunkSize();
  virtual int IoControl(EIoControl request, void* param);

  /*! \brief Only valid after GetLength() has been called, usually by Open(). */
  unsigned int GetIconWidth();
  /*! \brief Only valid after GetLength() has been called, usually by Open(). */
  unsigned int GetIconHeight();

protected:
  bool IsValidFile(const CURL& url);

private:
  CURL              m_url;
  std::string       m_appname;
  int               m_iconWidth;
  int               m_iconHeight;
};
}

#endif

