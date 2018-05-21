#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "DVDInputStream.h"

class CDVDInputStreamFile : public CDVDInputStream
{
public:
  explicit CDVDInputStreamFile(const CFileItem& fileitem);
  ~CDVDInputStreamFile() override;
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  bool Pause(double dTime) override { return false; };
  bool IsEOF() override;
  int64_t GetLength() override;
  BitstreamStats GetBitstreamStats() const override ;
  int GetBlockSize() override;
  void SetReadRate(unsigned rate) override;
  bool GetCacheStatus(XFILE::SCacheStatus *status) override;

protected:
  XFILE::CFile* m_pFile;
  bool m_eof;
};
