/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IEncoder.h"
#include "filesystem/File.h"

#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <string>

#define WRITEBUFFER_SIZE 131072 // 128k buffer

class CEncoder : public IEncoder
{
public:
  CEncoder() = default;
  virtual ~CEncoder();

  bool EncoderInit(const std::string& strFile, int iInChannels, int iInRate, int iInBits);
  int EncoderEncode(int nNumBytesRead, uint8_t* pbtStream);
  bool EncoderClose();

  void SetComment(const std::string& str) { m_strComment = str; }
  void SetArtist(const std::string& str) { m_strArtist = str; }
  void SetTitle(const std::string& str) { m_strTitle = str; }
  void SetAlbum(const std::string& str) { m_strAlbum = str; }
  void SetAlbumArtist(const std::string& str) { m_strAlbumArtist = str; }
  void SetGenre(const std::string& str) { m_strGenre = str; }
  void SetTrack(const std::string& str) { m_strTrack = str; }
  void SetTrackLength(int length) { m_iTrackLength = length; }
  void SetYear(const std::string& str) { m_strYear = str; }

protected:
  virtual int Write(const uint8_t* pBuffer, int iBytes);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence);

private:
  bool FileCreate(const std::string& filename);
  bool FileClose();
  int FileWrite(const void* pBuffer, uint32_t iBytes);
  int FlushStream();

  std::unique_ptr<XFILE::CFile> m_file;

  uint8_t m_btWriteBuffer[WRITEBUFFER_SIZE]; // 128k buffer for writing to disc
  uint32_t m_dwWriteBufferPointer{0};
};
