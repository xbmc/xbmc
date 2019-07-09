/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// FileShoutcast.h: interface for the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#include "CurlFile.h"
#include "IFile.h"
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

