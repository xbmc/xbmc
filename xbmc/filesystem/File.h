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

// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#include "IFileTypes.h"
#include "URL.h"

#include <iostream>
#include <memory>
#include <stdio.h>
#include <string>
#include <vector>

#include "PlatformDefs.h"

class BitstreamStats;

namespace XFILE
{

class IFile;

class CFileStreamBuffer;

class CFile
{
public:
  CFile();
  ~CFile();

  bool CURLCreate(const std::string &url);
  bool CURLAddOption(XFILE::CURLOPTIONTYPE type, const char* name, const char * value);
  bool CURLOpen(unsigned int flags);

  /**
  * Attempt to open an IFile instance.
  * @param file reference to CCurl file description
  * @param flags see IFileTypes.h
  * @return true on success, false otherwise
  *
  * Remarks: Open can only be called once. Calling
  * Open() on an already opened file will fail
  * except if flag READ_REOPEN is set and the underlying
  * file has an implementation of ReOpen().
  */
  bool Open(const CURL& file, const unsigned int flags = 0);
  bool Open(const std::string& strFileName, const unsigned int flags = 0);

  bool OpenForWrite(const CURL& file, bool bOverWrite = false);
  bool OpenForWrite(const std::string& strFileName, bool bOverWrite = false);

  ssize_t LoadFile(const CURL& file, std::vector<uint8_t>& outputBuffer);

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
  const std::string GetProperty(XFILE::FileProperty type, const std::string &name = "") const;
  const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type, const std::string &name = "") const;
  ssize_t LoadFile(const std::string& filename, std::vector<uint8_t>& outputBuffer);

  static int DetermineChunkSize(const int srcChunkSize, const int reqChunkSize);

  const std::unique_ptr<BitstreamStats>& GetBitstreamStats() const { return m_bitStreamStats; }

  int IoControl(EIoControl request, void* param);

  IFile* GetImplementation() const { return m_pFile.get(); }

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
  double GetDownloadSpeed();

private:
  /*!
   * \brief Determines if CFileStreamBuffer should be used to read a file.
   *
   * In general, should be used for ALL media files (only when is not used FileCache)
   * and NOT used for non-media files e.g. small local files as config/settings xml files.
   * Enables basic buffer that allows read sources with 64K chunk size even if FFmpeg only reads
   * data with small 4K chunks or Blu-Ray sector size (6144 bytes):
   *
   * [FFmpeg] <-----4K chunks----- [CFileStreamBuffer] <-----64K chunks----- [Source file / Network]
   *
   * NOTE: in case of SMB / NFS default 64K chunk size is replaced with value configured in
   * settings for the protocol.
   * This improves performance when reads big files through Network.
   * \param url Source file info as CULR class.
   */
  bool ShouldUseStreamBuffer(const CURL& url);

  unsigned int m_flags = 0;
  CURL                m_curl;
  std::unique_ptr<IFile> m_pFile;
  std::unique_ptr<CFileStreamBuffer> m_pBuffer;
  std::unique_ptr<BitstreamStats> m_bitStreamStats;
};

// streambuf for file io, only supports buffered input currently
class CFileStreamBuffer
  : public std::streambuf
{
public:
  ~CFileStreamBuffer() override;
  explicit CFileStreamBuffer(int backsize = 0);

  void Attach(IFile *file);
  void Detach();

private:
  int_type underflow() override;
  std::streamsize showmanyc() override;
  pos_type seekoff(off_type, std::ios_base::seekdir,std::ios_base::openmode = std::ios_base::in | std::ios_base::out) override;
  pos_type seekpos(pos_type, std::ios_base::openmode = std::ios_base::in | std::ios_base::out) override;

  IFile* m_file;
  char*  m_buffer;
  int    m_backsize;
  int    m_frontsize = 0;
};

// very basic file input stream
class CFileStream
  : public std::istream
{
public:
  explicit CFileStream(int backsize = 0);
  ~CFileStream() override;

  bool Open(const std::string& filename);
  bool Open(const CURL& filename);
  void Close();

  int64_t GetLength();
private:
  CFileStreamBuffer m_buffer;
  std::unique_ptr<IFile> m_file;
};

}
