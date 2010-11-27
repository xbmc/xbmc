/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// FileShoutcast.h: interface for the CFileShoutcast class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IFile.h"
#include "FileCurl.h"
#include "StdString.h"
#include "MusicInfoTag.h"

namespace XFILE
{
class CFileShoutcast : public IFile
{
public:
  CFileShoutcast();
  virtual ~CFileShoutcast();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
protected:
  void ExtractTagInfo(const char* buf);
  void ReadTruncated(char* buf2, int size);

  unsigned int m_lastTime;
  CFileCurl m_file;
  int m_metaint;
  int m_discarded; // data used for tags
  int m_currint;
  char* m_buffer; // buffer used for tags
  MUSIC_INFO::CMusicInfoTag m_tag;
};
}

