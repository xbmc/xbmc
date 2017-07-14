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
  ~CInputStreamMultiSource() override;

  void Abort() override;
  void Close() override;
  BitstreamStats GetBitstreamStats() const override ;
  int GetBlockSize() override;
  bool GetCacheStatus(XFILE::SCacheStatus *status) override;
  int64_t GetLength() override;
  bool IsEOF() override;
  CDVDInputStream::ENextStream NextStream() override;
  bool Open() override;
  bool Pause(double dTime)override { return false; };
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  void SetReadRate(unsigned rate) override;

protected:
  IVideoPlayer* m_pPlayer;
  std::vector<std::string> m_filenames;
};
