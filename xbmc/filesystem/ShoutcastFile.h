/*
 *      Copyright (C) 2005-2013 Team XBMC
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

// FileShoutcast.h: interface for the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#include "IFile.h"
#include "CurlFile.h"
#include "music/tags/MusicInfoTag.h"
#include "threads/Thread.h"

namespace XFILE
{

class CFileCache;

class CShoutcastFile : public IFile, public CThread
{
public:
  CShoutcastFile();
  ~CShoutcastFile() override;
  int64_t GetPosition() override;
  int64_t GetLength() override;
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override { return true;};
  int Stat(const CURL& url, struct __stat64* buffer) override { errno = ENOENT; return -1; };
  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  void Close() override;
  int IoControl(EIoControl request, void* param) override;

  void Process() override;
protected:
  bool ExtractTagInfo(const char* buf);
  void ReadTruncated(char* buf2, int size);

  CCurlFile m_file;
  std::string m_fileCharset;
  int m_metaint;
  int m_discarded; // data used for tags
  int m_currint;
  char* m_buffer; // buffer used for tags
  MUSIC_INFO::CMusicInfoTag m_tag;

  CFileCache* m_cacheReader;
  CEvent m_tagChange;
  CCriticalSection m_tagSection;
  int64_t m_tagPos;
};
}

