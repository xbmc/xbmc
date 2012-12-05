/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
#define AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

class CFileStreamBuffer;

class CFile
{
public:
  CFile();
  virtual ~CFile();

  bool Open(const CStdString& strFileName, unsigned int flags = 0);
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
