/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
