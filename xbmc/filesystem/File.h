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
#include "utils/StdString.h"
#include "IFileTypes.h"
#include "PlatformDefs.h"

class BitstreamStats;
class CURL;

namespace XFILE
{

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

class auto_buffer
{
public:
  auto_buffer(void) : p(NULL), s(0)
  { }
  explicit auto_buffer(size_t size);
  ~auto_buffer();

  auto_buffer& allocate(size_t size);
  auto_buffer& resize(size_t newSize);
  auto_buffer& clear(void);

  inline char* get(void) const { return static_cast<char*>(p); }
  inline size_t size(void) const { return s; }
  inline size_t length(void) const { return s; }

  auto_buffer& attach(void* pointer, size_t size);
  void* detach(void);

private:
  auto_buffer(const auto_buffer& other); // disallow copy constructor
  auto_buffer& operator=(const auto_buffer& other); // disallow assignment

  void* p;
  size_t s;
};

class CFile
{
public:
  CFile();
  ~CFile();

  bool Open(const CStdString& strFileName, const unsigned int flags = 0);
  bool OpenForWrite(const CStdString& strFileName, bool bOverWrite = false);
  unsigned int Read(void* lpBuf, int64_t uiBufSize);
  bool ReadString(char *szLine, int iLineLength);
  int Write(const void* lpBuf, int64_t uiBufSize);
  void Flush();
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int Truncate(int64_t iSize);
  int64_t GetPosition() const;
  int64_t GetLength();
  void Close();
  int GetChunkSize();
  std::string GetContentMimeType(void);
  std::string GetContentCharset(void);
  unsigned int LoadFile(const std::string &filename, auto_buffer& outputBuffer);


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

  static bool Exists(const CStdString& strFileName, bool bUseCache = true);
  static int  Stat(const CStdString& strFileName, struct __stat64* buffer);
  int Stat(struct __stat64 *buffer);
  static bool Delete(const CStdString& strFileName);
  static bool Rename(const CStdString& strFileName, const CStdString& strNewFileName);
  static bool Cache(const CStdString& strFileName, const CStdString& strDest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
  static bool SetHidden(const CStdString& fileName, bool hidden);

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

  bool Open(const CStdString& filename);
  bool Open(const CURL& filename);
  void Close();

  int64_t GetLength();
private:
  CFileStreamBuffer m_buffer;
  IFile*            m_file;
};

}
#endif // !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
