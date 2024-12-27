/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// IFile.h: interface for the IFile class.
//
//////////////////////////////////////////////////////////////////////

#include "PlatformDefs.h" // for __stat64, ssize_t

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string>
#include <vector>

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
  virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false) { return false; }
  virtual bool ReOpen(const CURL& url) { return false; }
  virtual bool Exists(const CURL& url) = 0;
  /*!
   * \brief Fills struct __stat64 with information about file specified by url.
   *
   * For st_mode function will set correctly _S_IFDIR (directory) flag and may set
   * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
   * information is available. Function may set st_size (file size), st_atime,
   * st_mtime, st_ctime (access, modification, creation times).
   * Any other flags and members of __stat64 that didn't updated with actual file
   * information will be set to zero (st_nlink can be set ether to 1 or zero).
   *
   * \param[in] url specifies requested file. Ends with a directory separator for directories.
   * \param[out] buffer pointer to __stat64 buffer to receive information about file
   * \return zero for success, -1 otherwise.
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
  struct ReadLineResult
  {
    enum class ResultCode
    {
      FAILURE, ///< Recondition violated, e.g. buffer is nullptr, file not open, EOF already reached, ...
      TRUNCATED, ///< The line is longer than the supplied buffer. The next read continues reading of the line
      OK, ///< Read a line
    };
    using enum ResultCode;

    ResultCode code; ///< The result of the read operation
    std::size_t length; ///< length of the read string without null terminator
  };
  /**
   * Reads a line from a file into \p buffer. Reads at most \p bufferLength - 1 bytes from the file.
   * \p buffer is unchanged in case the returned result code is FAILURE. The read line can contain '\0' characters
   * @param buffer The buffer into which the line is wrote
   * @param bufferSize The size of \p buffer
   * @return See \ref ReadLineResult
   */
  virtual ReadLineResult ReadLine(char* buffer, std::size_t bufferSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) = 0;
  virtual void Close() = 0;
  virtual int64_t GetPosition() = 0;
  virtual int64_t GetLength() = 0;
  virtual void Flush() { }
  virtual int Truncate(int64_t size) { return -1; }

  /* Returns the minimum size that can be read from input stream.  *
   * For example cdrom access where access could be sector based.  *
   * This will cause file system to buffer read requests, to       *
   * to meet the requirement of CFile.                             *
   * It can also be used to indicate a file system is non buffered *
   * but accepts any read size, have it return the value 1         */
  virtual int  GetChunkSize() {return 0;}
  virtual double GetDownloadSpeed() { return 0.0; }

  virtual bool Delete(const CURL& url) { return false; }
  virtual bool Rename(const CURL& url, const CURL& urlnew) { return false; }
  virtual bool SetHidden(const CURL& url, bool hidden) { return false; }

  virtual int IoControl(IOControl request, void* param) { return -1; }

  virtual const std::string GetProperty(XFILE::FileProperty type, const std::string &name = "") const
  {
    return type == XFILE::FileProperty::CONTENT_TYPE ? "application/octet-stream" : "";
  };

  virtual const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type, const std::string &name = "") const
  {
    std::vector<std::string> values;
    std::string value = GetProperty(type, name);
    if (!value.empty())
    {
      values.emplace_back(value);
    }
    return values;
  }
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
