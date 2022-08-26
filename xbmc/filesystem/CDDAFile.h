/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"
#include "storage/cdioSupport.h"

namespace XFILE
{
class CFileCDDA : public IFile
{
public:
  CFileCDDA(void);
  ~CFileCDDA(void) override;
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;

  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  void Close() override;
  int64_t GetPosition() override;
  int64_t GetLength() override;
  int GetChunkSize() override;

protected:
  bool IsValidFile(const CURL& url);
  int GetTrackNum(const CURL& url);

protected:
  CdIo_t* m_pCdIo;
  lsn_t m_lsnStart = CDIO_INVALID_LSN; // Start of m_iTrack in logical sector number
  lsn_t m_lsnCurrent = CDIO_INVALID_LSN; // Position inside the track in logical sector number
  lsn_t m_lsnEnd = CDIO_INVALID_LSN; // End of m_iTrack in logical sector number
  int m_iSectorCount; // max number of sectors to read at once
  std::shared_ptr<MEDIA_DETECT::CLibcdio> m_cdio;
};
}
