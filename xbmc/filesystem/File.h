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

// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
#define AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_

#pragma once

#include <iostream>
#include <stdio.h>
#include <string>
#include "utils/auto_buffer.h"
#include "IFileTypes.h"
#include "PlatformDefs.h"

class BitstreamStats;
class CURL;

namespace XFILE
{

using ::XUTILS::auto_buffer;
class IFile;

class IFileCallback
{
public:
  virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) = 0;
  virtual ~IFileCallback() {};
};

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED 0x01

/* indicate that that caller support read in the minimum defined chunk size, this disables internal cache then */
#define READ_CHUNKED   0x02

/* use cache to access this file */
#define READ_CACHED     0x04

/* open without caching. regardless to file type. */
#define READ_NO_CACHE  0x08

/* calcuate bitrate for file while reading */
#define READ_BITRATE   0x10

/* indicate the caller will seek between multiple streams in the file frequently */
#define READ_MULTI_STREAM 0x20

class CFileStreamBuffer;

class CFile
{
public:
  CFile();
  ~CFile();

  bool Open(const CURL& file, const unsigned int flags = 0);
  bool OpenForWrite(const CURL& file, bool bOverWrite = false);
  ssize_t LoadFile(const CURL &file, auto_buffer& outputBuffer);

  bool Open(const std::string& strFileName, const unsigned int flags = 0);
  bool OpenForWrite(const std::string& strFileName, bool bOverWrite = false);
  /**
   * Attempt to read bufSize bytes from currently opened file into buffer bufPtr.
   * @param bufPtr  pointer to buffer
   * @param bufSize size of the buffer
   * @return number of successfully read bytes if any bytes were read and stored in
   *         buffer, zero if no bytes are available to read (end of file was reached)
   *         or undetectable error occur, -1 in case of any explicit error
   */
  ssize_t Read(void* bufPtr, size_t bufSize);
  bool ReadString(char *szLine, int iLineLength);
  /**
   * Attempt to write bufSize bytes from buffer bufPtr into currently opened file.
   * @param bufPtr  pointer to buffer
   * @param bufSize size of the buffer
   * @return number of successfully written bytes if any bytes were written,
   *         zero if no bytes were written and no detectable error occur,
   *         -1 in case of any explicit error
   */
  ssize_t Write(const void* bufPtr, size_t bufSize);
  void Flush();
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int Truncate(int64_t iSize);
  int64_t GetPosition() const;
  int64_t GetLength();
  void Close();
  int GetChunkSize();
  std::string GetContentMimeType(void);
  std::string GetContentCharset(void);
  ssize_t LoadFile(const std::string &filename, auto_buffer& outputBuffer);


  // will return a size, that is aligned to chunk size
  // but always greater or equal to the file's chunk size
  static int GetChunkSize(int chunk, int minimum)
  {
    if(chunk)
      return chunk * ((minimum + chunk - 1) / chunk);
    else
      return minimum;
  }

  bool SkipNext();
  BitstreamStats* GetBitstreamStats() { return m_bitStreamStats; }

  int IoControl(EIoControl request, void* param);

  IFile *GetImplemenation() { return m_pFile; }

  // CURL interface
  static bool Exists(const CURL& file, bool bUseCache = true);
  static bool Delete(const CURL& file);
  /**
  * Fills struct __stat64 with information about file specified by filename
  * For st_mode function will set correctly _S_IFDIR (directory) flag and may set
  * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
  * information is available. Function may set st_size (file size), st_atime,
  * st_mtime, st_ctime (access, modification, creation times).
  * Any other flags and members of __stat64 that didn't updated with actual file
  * information will be set to zero (st_nlink can be set ether to 1 or zero).
  * @param file        specifies requested file
  * @param buffer      pointer to __stat64 buffer to receive information about file
  * @return zero of success, -1 otherwise.
  */
  static int  Stat(const CURL& file, struct __stat64* buffer);
  static bool Rename(const CURL& file, const CURL& urlNew);
  static bool Copy(const CURL& file, const CURL& dest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
  static bool SetHidden(const CURL& file, bool hidden);

  // string interface
  static bool Exists(const std::string& strFileName, bool bUseCache = true);
  /**
  * Fills struct __stat64 with information about file specified by filename
  * For st_mode function will set correctly _S_IFDIR (directory) flag and may set 
  * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
  * information is available. Function may set st_size (file size), st_atime,
  * st_mtime, st_ctime (access, modification, creation times).
  * Any other flags and members of __stat64 that didn't updated with actual file 
  * information will be set to zero (st_nlink can be set ether to 1 or zero).
  * @param strFileName specifies requested file
  * @param buffer      pointer to __stat64 buffer to receive information about file
  * @return zero of success, -1 otherwise.
  */
  static int  Stat(const std::string& strFileName, struct __stat64* buffer);
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
  int Stat(struct __stat64 *buffer);
  static bool Delete(const std::string& strFileName);
  static bool Rename(const std::string& strFileName, const std::string& strNewFileName);
  static bool Copy(const std::string& strFileName, const std::string& strDest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
  static bool SetHidden(const std::string& fileName, bool hidden);

private:
  unsigned int m_flags;
  IFile* m_pFile;
  CFileStreamBuffer* m_pBuffer;
  BitstreamStats* m_bitStreamStats;
};

// streambuf for file io, only supports buffered input currently
class CFileStreamBuffer
  : public std::streambuf
{
public:
  ~CFileStreamBuffer();
  CFileStreamBuffer(int backsize = 0);

  void Attach(IFile *file);
  void Detach();

private:
  virtual int_type underflow();
  virtual std::streamsize showmanyc();
  virtual pos_type seekoff(off_type, std::ios_base::seekdir,std::ios_base::openmode = std::ios_base::in | std::ios_base::out);
  virtual pos_type seekpos(pos_type, std::ios_base::openmode = std::ios_base::in | std::ios_base::out);

  IFile* m_file;
  char*  m_buffer;
  int    m_backsize;
  int    m_frontsize;
};

// very basic file input stream
class CFileStream
  : public std::istream
{
public:
  CFileStream(int backsize = 0);
  ~CFileStream();

  bool Open(const std::string& filename);
  bool Open(const CURL& filename);
  void Close();

  int64_t GetLength();
private:
  CFileStreamBuffer m_buffer;
  IFile*            m_file;
};

}
#endif // !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
