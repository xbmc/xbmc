/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDInputStream.h"

class CDVDInputStreamMemory : public CDVDInputStream
{
public:
  explicit CDVDInputStreamMemory(CFileItem& fileitem);
  ~CDVDInputStreamMemory() override;
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  bool IsEOF() override;
  int64_t GetLength() override;

protected:
  uint8_t* m_pData;
  int m_iDataSize;
  int m_iDataPos;
};
