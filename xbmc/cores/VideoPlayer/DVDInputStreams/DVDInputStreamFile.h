#pragma once

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

#include "DVDInputStream.h"

class CDVDInputStreamFile : public CDVDInputStream
{
public:
  CDVDInputStreamFile(const CFileItem& fileitem);
  virtual ~CDVDInputStreamFile();
  virtual bool Open() override;
  virtual void Close() override;
  virtual int Read(uint8_t* buf, int buf_size) override;
  virtual int64_t Seek(int64_t offset, int whence) override;
  virtual bool Pause(double dTime) override { return false; };
  virtual bool IsEOF() const override;
  virtual int64_t GetLength() const override;
  virtual BitstreamStats GetBitstreamStats() const override;
  virtual int GetBlockSize() const override;
  virtual void SetReadRate(unsigned rate) override;
  virtual bool GetCacheStatus(XFILE::SCacheStatus *status) const override;

protected:
  XFILE::CFile* m_pFile;
  bool m_eof;
};
