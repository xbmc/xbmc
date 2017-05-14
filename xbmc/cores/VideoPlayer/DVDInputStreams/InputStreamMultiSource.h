#pragma once

/*
 *      Copyright (C) 2005-2015 Team XBMC
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
#include "InputStreamMultiStreams.h"

#include <string>
#include <vector>

class IVideoPlayer;

class CInputStreamMultiSource : public InputStreamMultiStreams
{

public:
  CInputStreamMultiSource(IVideoPlayer* pPlayer, const CFileItem& fileitem, const std::vector<std::string>& filenames);
  virtual ~CInputStreamMultiSource();

  virtual void Abort() override;
  virtual void Close() override;
  virtual BitstreamStats GetBitstreamStats() const override;
  virtual int GetBlockSize() override;
  virtual bool GetCacheStatus(XFILE::SCacheStatus *status) override;
  int64_t GetLength() override;
  virtual bool IsEOF() override;
  virtual CDVDInputStream::ENextStream NextStream() override;
  virtual bool Open() override;
  virtual bool Pause(double dTime)override { return false; };
  virtual int Read(uint8_t* buf, int buf_size) override;
  virtual int64_t Seek(int64_t offset, int whence) override;
  virtual void SetReadRate(unsigned rate) override;

protected:
  IVideoPlayer* m_pPlayer;
  std::vector<std::string> m_filenames;
};
