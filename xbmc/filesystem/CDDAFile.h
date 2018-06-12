/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
  lsn_t m_lsnStart;  // Start of m_iTrack in logical sector number
  lsn_t m_lsnCurrent; // Position inside the track in logical sector number
  lsn_t m_lsnEnd;   // End of m_iTrack in logical sector number
  int m_iSectorCount; // max number of sectors to read at once
  std::shared_ptr<MEDIA_DETECT::CLibcdio> m_cdio;
};
}
