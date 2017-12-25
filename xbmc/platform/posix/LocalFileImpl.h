#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
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

#include "URL.h"
#include "platform/Filesystem.h"

namespace KODI
{
namespace PLATFORM
{
namespace DETAILS
{
class CLocalFileImpl
{
public:
  CLocalFileImpl();
  ~CLocalFileImpl();

  bool Open(const CURL &url);
  bool Open(const std::string &url);
  bool OpenForWrite(const CURL &url, bool bOverWrite = false);
  bool OpenForWrite(const std::string &url, bool bOverWrite = false);
  void Close();

  int64_t Read(void *lpBuf, size_t uiBufSize);
  int64_t Write(const void *lpBuf, size_t uiBufSize);
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int Truncate(int64_t toSize);
  int64_t GetPosition();
  int64_t GetLength();
  void Flush();
  int Stat(struct __stat64 *buffer);

  static bool Delete(const CURL &url);
  static bool Delete(const std::string &url);
  static bool Rename(const CURL &urlCurrentName, const CURL &urlNewName);
  static bool Rename(const std::string &urlCurrentName, const std::string &urlNewName);
  static bool SetHidden(const CURL &url, bool hidden);
  static bool SetHidden(const std::string &url, bool hidden);
  static bool Exists(const CURL &url);
  static bool Exists(const std::string &url);
  static int Stat(const CURL &url, struct __stat64 *buffer);
  static int Stat(const std::string &url, struct __stat64 *buffer);

protected:
  int m_fd;
  int64_t m_filePos;
  int64_t m_lastDropPos;
  bool m_allowWrite;
};
}
}
}