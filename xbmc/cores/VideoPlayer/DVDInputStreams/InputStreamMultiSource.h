/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  void SetReadRate(unsigned rate) override;

protected:
  IVideoPlayer* m_pPlayer;
  std::vector<std::string> m_filenames;
};
