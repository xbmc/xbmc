/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDInputStream.h"

class CDVDInputStreamFile : public CDVDInputStream
{
public:
  explicit CDVDInputStreamFile(const CFileItem& fileitem, unsigned int flags);
  ~CDVDInputStreamFile() override;
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  bool IsEOF() override;
  int64_t GetLength() override;
  BitstreamStats GetBitstreamStats() const override ;
  int GetBlockSize() override;
  void SetReadRate(uint32_t rate) override;
  bool GetCacheStatus(XFILE::SCacheStatus *status) override;

protected:
  XFILE::CFile* m_pFile = nullptr;
  bool m_eof = false;
  unsigned int m_flags = 0;
};
