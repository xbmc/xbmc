#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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
#include <memory>
#include <vector>

class CDVDInputStreamStack : public CDVDInputStream
{
public:
  explicit CDVDInputStreamStack(const CFileItem& fileitem);
  ~CDVDInputStreamStack() override;

  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  bool Pause(double dTime) override { return false; };
  bool IsEOF() override;
  int64_t GetLength() override;

protected:

  typedef std::shared_ptr<XFILE::CFile> TFile;

  struct TSeg
  {
    TFile file;
    int64_t length;
  };

  typedef std::vector<TSeg> TSegVec;

  TSegVec m_files;  ///< collection of open ptr's to all files in stack
  TFile m_file;   ///< currently active file
  bool m_eof;
  int64_t m_pos;
  int64_t m_length;
};
