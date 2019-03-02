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

// FileISO.h: interface for the CISOFile class.
//
//////////////////////////////////////////////////////////////////////

#include "IFile.h"
#include "utils/RingBuffer.h"

namespace XFILE
{

class CISOFile : public IFile
{
public:
  CISOFile();
  ~CISOFile() override;
  int64_t GetPosition() override;
  int64_t GetLength() override;
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  void Close() override;
protected:
  bool m_bOpened = false;
  HANDLE m_hFile;
  CRingBuffer m_cache;
};
}
