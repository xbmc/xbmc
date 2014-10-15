/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
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

// IFile.h: interface for the IFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_)
#define AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_

#pragma once

#include "PlatformDefs.h" // for __stat64, ssize_t

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string>

#if !defined(SIZE_MAX) || !defined(SSIZE_MAX)
#include <limits.h>
#ifndef SIZE_MAX
#define SIZE_MAX UINTPTR_MAX
#endif // ! SIZE_MAX
#ifndef SSIZE_MAX
#define SSIZE_MAX INTPTR_MAX
#endif // ! SSIZE_MAX
#endif // ! SIZE_MAX || ! SSIZE_MAX

#include "IFileTypes.h"

class CURL;

namespace XFILE
{

class IFile
{
public:
  IFile();
  virtual ~IFile();

  virtual bool Open(const CURL& url) = 0;
  virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false) { return false; };
  virtual bool Exists(const CURL& url) = 0;
  /**
   * Fills struct __stat64 with information about file specified by url.
   * For st_mode function will set correctly _S_IFDIR (directory) flag and may set
   * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
   * information is available. Function may set st_size (file size), st_atime,
   * st_mtime, st_ctime (access, modification, creation times).
   * Any other flags and members of __stat64 that didn't updated with actual file
   * information will be set to zero (st_nlink can be set ether to 1 or zero).
   * @param url         specifies requested file
   * @param buffer      pointer to __stat64 buffer to receive information about file
   * @return zero of success, -1 otherwise.
   */
  virtual int Stat(const CURL& url, struct __stat64* buffer) = 0;
  /**
   * Fills struct __stat64 with information about currently open file
   * For st_mode function will set correctly _S_IFDIR (directory) flag and may set
   * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
   * information is available. Function may set st_size (file size), st_atime,
   * st_mtime, st_ctime (access, modification, creation times).
   * Any other flags and members of __stat64 that didn't updated with actual file
   * information will be set to zero (st_nlink can be set ether to 1 or zero).
   * @param buffer      pointer to __stat64 buffer to receive information about file
   * @return zero of success, -1 otherwise.
   */
  virtual int Stat(struct __stat64* buffer);
  /**
   * Attempt to read bufSize bytes from currently opened file into buffer bufPtr.
   * @param bufPtr  pointer to buffer
   * @param bufSize size of the buffer
   * @return number of successfully read bytes if any bytes were read and stored in
   *         buffer, zero if no bytes are available to read (end of file was reached)
   *         or undetectable error occur, -1 in case of any explicit error
   */
  virtual ssize_t Read(void* bufPtr, size_t bufSize) = 0;
  /**
   * Attempt to write bufSize bytes from buffer bufPtr into currently opened file.
   * @param bufPtr  pointer to buffer
   * @param bufSize size of the buffer
   * @return number of successfully written bytes if any bytes were written,
   *         zero if no bytes were written and no detectable error occur,
   *         -1 in case of any explicit error
   */
  virtual ssize_t Write(const void* bufPtr, size_t bufSize) { return -1;}
  virtual bool ReadString(char *szLine, int iLineLength);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) = 0;
  virtual void Close() = 0;
  virtual int64_t GetPosition() = 0;
  virtual int64_t GetLength() = 0;
  virtual void Flush() { }
  virtual int Truncate(int64_t size) { return -1;};

  /* Returns the minium size that can be read from input stream.   *
   * For example cdrom access where access could be sector based.  *
   * This will cause file system to buffer read requests, to       *
   * to meet the requirement of CFile.                             *
   * It can also be used to indicate a file system is non buffered *
   * but accepts any read size, have it return the value 1         */
  virtual int  GetChunkSize() {return 0;}

  virtual bool SkipNext(){return false;}

  virtual bool Delete(const CURL& url) { return false; }
  virtual bool Rename(const CURL& url, const CURL& urlnew) { return false; }
  virtual bool SetHidden(const CURL& url, bool hidden) { return false; }

  virtual int IoControl(EIoControl request, void* param) { return -1; }

  virtual std::string GetContent()                           { return "application/octet-stream"; }
  virtual std::string GetContentCharset(void)                { return ""; }
};

class CRedirectException
{
public:
  IFile *m_pNewFileImp;
  CURL  *m_pNewUrl;

  CRedirectException();
  
  CRedirectException(IFile *pNewFileImp, CURL *pNewUrl=NULL);
};

}

#endif // !defined(AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_)
