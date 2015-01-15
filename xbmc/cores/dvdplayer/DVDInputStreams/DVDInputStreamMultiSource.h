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

#include <string>
#include <vector>

typedef std::shared_ptr<CDVDInputStream> InputStreamPtr;
class IDVDPlayer;

class CDVDInputStreamMultiSource : public CDVDInputStream
{

public:
  CDVDInputStreamMultiSource(IDVDPlayer* pPlayer, const std::vector<std::string>& filenames);
  virtual ~CDVDInputStreamMultiSource();

  virtual void Abort();
  virtual void Close();
  virtual BitstreamStats GetBitstreamStats() const ;
  virtual int GetBlockSize();
  virtual bool GetCacheStatus(XFILE::SCacheStatus *status);
  int64_t GetLength();
  virtual bool IsEOF();
  virtual bool Open(const char* strFile, const std::string &content);
  virtual bool Pause(double dTime) { return false; };
  virtual int Read(uint8_t* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual void SetReadRate(unsigned rate);

protected:
  IDVDPlayer* m_pPlayer;
  std::vector<std::string> m_filenames;
  std::vector<InputStreamPtr> m_pInputStreams;    // input streams for current playing file
};
