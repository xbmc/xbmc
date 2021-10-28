/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IEncoder.h"

#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <string>

#define WRITEBUFFER_SIZE 131072 // 128k buffer

namespace XFILE
{
class CFile;
}

class CEncoder
{
public:
  explicit CEncoder(std::shared_ptr<IEncoder> encoder);
  virtual ~CEncoder();
  virtual bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  virtual int Encode(int nNumBytesRead, uint8_t* pbtStream);
  virtual bool CloseEncode();

  void SetComment(const std::string& str) { m_impl->m_strComment = str; }
  void SetArtist(const std::string& str) { m_impl->m_strArtist = str; }
  void SetTitle(const std::string& str) { m_impl->m_strTitle = str; }
  void SetAlbum(const std::string& str) { m_impl->m_strAlbum = str; }
  void SetAlbumArtist(const std::string& str) { m_impl->m_strAlbumArtist = str; }
  void SetGenre(const std::string& str) { m_impl->m_strGenre = str; }
  void SetTrack(const std::string& str) { m_impl->m_strTrack = str; }
  void SetTrackLength(int length) { m_impl->m_iTrackLength = length; }
  void SetYear(const std::string& str) { m_impl->m_strYear = str; }

  bool FileCreate(const char* filename);
  bool FileClose();
  int FileWrite(const void* pBuffer, uint32_t iBytes);
  int64_t FileSeek(int64_t iFilePosition, int iWhence = SEEK_SET);

protected:
  int WriteStream(const void* pBuffer, uint32_t iBytes);
  int FlushStream();

  static int WriteCallback(void* opaque, const uint8_t* data, int size);
  static int64_t SeekCallback(void* opaque, int64_t offset, int whence);

  std::shared_ptr<IEncoder> m_impl;

  XFILE::CFile* m_file;

  uint8_t m_btWriteBuffer[WRITEBUFFER_SIZE]; // 128k buffer for writing to disc
  uint32_t m_dwWriteBufferPointer;
};
