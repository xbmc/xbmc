/*
 *      Copyright (C) 2005-2013 Team XBMC
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

// FileShoutcast.h: interface for the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef FILESYSTEM_IFILE_H_INCLUDED
#define FILESYSTEM_IFILE_H_INCLUDED
#include "IFile.h"
#endif

#ifndef FILESYSTEM_CURLFILE_H_INCLUDED
#define FILESYSTEM_CURLFILE_H_INCLUDED
#include "CurlFile.h"
#endif

#ifndef FILESYSTEM_UTILS_STDSTRING_H_INCLUDED
#define FILESYSTEM_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef FILESYSTEM_MUSIC_TAGS_MUSICINFOTAG_H_INCLUDED
#define FILESYSTEM_MUSIC_TAGS_MUSICINFOTAG_H_INCLUDED
#include "music/tags/MusicInfoTag.h"
#endif

#ifndef FILESYSTEM_THREADS_THREAD_H_INCLUDED
#define FILESYSTEM_THREADS_THREAD_H_INCLUDED
#include "threads/Thread.h"
#endif


namespace XFILE
{

class CFileCache;

class CShoutcastFile : public IFile, public CThread
{
public:
  CShoutcastFile();
  virtual ~CShoutcastFile();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  int IoControl(EIoControl request, void* param);

  void Process();
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
  int64_t m_tagPos;
};
}

