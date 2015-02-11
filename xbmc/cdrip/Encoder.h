#ifndef _ENCODER_H
#define _ENCODER_H

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

#include "IEncoder.h"
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <memory>

#define WRITEBUFFER_SIZE 131072 // 128k buffer

namespace XFILE { class CFile; }

class CEncoder
{
public:
  CEncoder(std::shared_ptr<IEncoder> encoder);
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
  int FileWrite(const void *pBuffer, uint32_t iBytes);
  int64_t FileSeek(int64_t iFilePosition, int iWhence = SEEK_SET);
protected:

  int WriteStream(const void *pBuffer, uint32_t iBytes);
  int FlushStream();

  static int WriteCallback(void *opaque, uint8_t *data, int size);
  static int64_t SeekCallback(void *opaque, int64_t offset, int whence);

  std::shared_ptr<IEncoder> m_impl;

  XFILE::CFile *m_file;

  uint8_t m_btWriteBuffer[WRITEBUFFER_SIZE]; // 128k buffer for writing to disc
  uint32_t m_dwWriteBufferPointer;
};

#endif // _ENCODER_H

